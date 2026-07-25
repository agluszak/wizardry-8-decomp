from __future__ import annotations

import hashlib
import json
import re
import shutil
import struct
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path
from typing import Any, Literal

import yaml
from pydantic import BaseModel, ConfigDict, Field, field_validator, model_validator

from ..binary.coff_archive import coff_member_kind, read_coff_archive
from ..binary.rich_header import parse_rich_header
from ..config import Settings
from ..paths import (
    atomic_json,
    atomic_write,
    ensure_safe_generated_target,
    sha256_file,
    tree_hash,
    tree_manifest,
)
from ..subprocesses import run, tool_version


class SourceArchive(BaseModel):
    url: str
    sha256: str | None
    archive_root: str


class SeedVariant(BaseModel):
    id: str
    flags: list[str]


class StaticLibrary(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    id: str
    family: str
    version: str
    status: str
    scope: list[str]
    evidence: list[str]
    source: SourceArchive | None = None
    source_overlay: str | None = None
    seed_variants: list[SeedVariant] = Field(default_factory=list)
    compilation_units: list[str] = Field(default_factory=list)


class PrecompiledArchive(BaseModel):
    id: str
    library: str
    variant: str
    path: str
    sha256: str
    seed: bool = True

    @field_validator("sha256")
    @classmethod
    def full_sha256(cls, value: str) -> str:
        if len(value) != 64 or any(char not in "0123456789abcdef" for char in value):
            raise ValueError("precompiled archive SHA-256 must be a full lowercase digest")
        return value


class Toolchain(BaseModel):
    id: str
    family: str
    repository: str
    commit: str
    image: str
    status: str
    capabilities: list[str]
    precompiled_archives: list[PrecompiledArchive] = Field(default_factory=list)

    @field_validator("id")
    @classmethod
    def stable_id(cls, value: str) -> str:
        if not value or any(char not in "abcdefghijklmnopqrstuvwxyz0123456789-" for char in value):
            raise ValueError("toolchain id must be a lowercase slug")
        return value

    @field_validator("commit")
    @classmethod
    def full_git_commit(cls, value: str) -> str:
        if len(value) != 40 or any(char not in "0123456789abcdef" for char in value):
            raise ValueError("toolchain commit must be a full lowercase Git object ID")
        return value

    @field_validator("capabilities")
    @classmethod
    def known_capabilities(cls, value: list[str]) -> list[str]:
        unknown = sorted(set(value) - {"compiler", "precompiled-libraries"})
        if unknown:
            raise ValueError(f"unknown toolchain capabilities: {', '.join(unknown)}")
        if not value:
            raise ValueError("toolchain must expose at least one capability")
        return list(dict.fromkeys(value))

    @field_validator("precompiled_archives")
    @classmethod
    def unique_archive_ids(cls, value: list[PrecompiledArchive]) -> list[PrecompiledArchive]:
        ids = [archive.id for archive in value]
        if len(ids) != len(set(ids)):
            raise ValueError("precompiled archive ids must be unique per toolchain")
        return value


class ToolchainEvidence(BaseModel):
    selection_confidence: str
    evidence: list[str]
    uncertainty: list[str]


class StaticLibrariesConfig(BaseModel):
    model_config = ConfigDict(extra="forbid", populate_by_name=True)

    schema_name: Literal["wiz8.static-libraries"] = Field(alias="schema")
    toolchains: list[Toolchain]
    toolchain_evidence: ToolchainEvidence
    libraries: list[StaticLibrary]

    @field_validator("toolchains")
    @classmethod
    def unique_toolchains(cls, value: list[Toolchain]) -> list[Toolchain]:
        ids = [toolchain.id for toolchain in value]
        if len(ids) != len(set(ids)):
            raise ValueError("toolchain ids must be unique")
        return value

    @field_validator("libraries")
    @classmethod
    def unique_libraries(cls, value: list[StaticLibrary]) -> list[StaticLibrary]:
        ids = [library.id for library in value]
        if len(ids) != len(set(ids)):
            raise ValueError("static-library ids must be unique")
        return value

    @model_validator(mode="after")
    def valid_archive_references(self) -> StaticLibrariesConfig:
        library_ids = {library.id for library in self.libraries}
        for toolchain in self.toolchains:
            has_archives = bool(toolchain.precompiled_archives)
            if has_archives != ("precompiled-libraries" in toolchain.capabilities):
                raise ValueError(
                    f"{toolchain.id} must declare both precompiled-libraries capability and archives"
                )
            unknown = sorted(
                {archive.library for archive in toolchain.precompiled_archives} - library_ids
            )
            if unknown:
                raise ValueError(
                    f"{toolchain.id} precompiled archives reference unknown libraries: {', '.join(unknown)}"
                )
        return self


def load_static_libraries(settings: Settings) -> StaticLibrariesConfig:
    path = settings.repo_dir / "config" / "static-libraries.yml"
    return StaticLibrariesConfig.model_validate(yaml.safe_load(path.read_text(encoding="utf-8")))


def select_toolchains(
    config: StaticLibrariesConfig,
    ids: list[str] | None = None,
    *,
    capability: str | None = None,
) -> list[Toolchain]:
    by_id = {toolchain.id: toolchain for toolchain in config.toolchains}
    if not ids:
        selected = sorted(config.toolchains, key=lambda item: item.id)
        return [item for item in selected if capability is None or capability in item.capabilities]
    unknown = sorted(set(ids) - set(by_id))
    if unknown:
        raise RuntimeError(f"unknown toolchain id(s): {', '.join(unknown)}")
    selected = [by_id[item] for item in dict.fromkeys(ids)]
    incompatible = [
        item.id
        for item in selected
        if capability is not None and capability not in item.capabilities
    ]
    if incompatible:
        raise RuntimeError(f"toolchain(s) do not provide {capability}: {', '.join(incompatible)}")
    return selected


def select_libraries(
    config: StaticLibrariesConfig, ids: list[str] | None = None
) -> list[StaticLibrary]:
    by_id = {library.id: library for library in config.libraries}
    if not ids:
        return sorted(config.libraries, key=lambda item: item.id)
    unknown = sorted(set(ids) - set(by_id))
    if unknown:
        raise RuntimeError(f"unknown static-library id(s): {', '.join(unknown)}")
    return [by_id[item] for item in dict.fromkeys(ids)]


def _fid_root(settings: Settings) -> Path:
    root = settings.work_dir / "fid"
    ensure_safe_generated_target(root, settings.work_dir)
    return root


def static_inventory(settings: Settings) -> dict[str, Any]:
    config = load_static_libraries(settings)
    precompiled = {
        archive.library
        for toolchain in config.toolchains
        for archive in toolchain.precompiled_archives
    }
    source_seedable = {
        library.id
        for library in config.libraries
        if library.source is not None
        and library.source.sha256 is not None
        and library.seed_variants
        and library.compilation_units
    }
    result = {
        "schema": "wiz8.static-library-inventory",
        "toolchains": [toolchain.model_dump(mode="json") for toolchain in config.toolchains],
        "toolchain_evidence": config.toolchain_evidence.model_dump(mode="json"),
        "libraries": [library.model_dump(mode="json") for library in config.libraries],
        "seedable_now": [
            library.id
            for library in config.libraries
            if library.id in source_seedable or library.id in precompiled
        ],
        "blocked": [
            {
                "library": library.id,
                "reason": (
                    "source archive is not pinned"
                    if library.source is not None and library.source.sha256 is None
                    else "no reproducible source/build recipe is configured"
                ),
            }
            for library in config.libraries
            if library.id not in source_seedable and library.id not in precompiled
        ],
    }
    atomic_json(settings.build_dir / "reports" / "static-libraries.json", result)
    lines = [
        "# Statically linked library inventory",
        "",
        "| Library | Version | Status | Scope | FID seed |",
        "|---|---|---|---|---|",
    ]
    seedable = set(result["seedable_now"])
    for library in config.libraries:
        lines.append(
            f"| {library.family} | {library.version} | {library.status} | "
            f"{', '.join(f'`{item}`' for item in library.scope)} | "
            f"{'exact archive' if library.id in precompiled else 'source build' if library.id in seedable else 'blocked'} |"
        )
    lines.extend(
        [
            "",
            "The inventory distinguishes binary-confirmed versions from build recipes. A library is only seedable when its source archive, hash, compiler and flags are pinned.",
            "",
        ]
    )
    atomic_write(settings.build_dir / "reports" / "static-libraries.md", "\n".join(lines))
    return result


def _download(url: str) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": "wizardry8-decomp/0.1"})
    with urllib.request.urlopen(request, timeout=120) as response:
        return response.read()


def _safe_extract_tar(archive: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    root = destination.resolve()
    with tarfile.open(archive, mode="r:*") as stream:
        members = stream.getmembers()
        for member in members:
            target = (destination / member.name).resolve()
            if target != root and root not in target.parents:
                raise RuntimeError(f"archive member escapes extraction root: {member.name}")
            if member.issym() or member.islnk() or member.isdev():
                raise RuntimeError(f"unsupported archive member type: {member.name}")
        stream.extractall(destination, members=members)


def _safe_extract_zip(archive: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    root = destination.resolve()
    with zipfile.ZipFile(archive) as stream:
        members = stream.infolist()
        for member in members:
            target = (destination / member.filename).resolve()
            if target != root and root not in target.parents:
                raise RuntimeError(f"archive member escapes extraction root: {member.filename}")
            mode = member.external_attr >> 16
            if mode & 0o170000 == 0o120000:
                raise RuntimeError(f"unsupported archive symlink: {member.filename}")
        stream.extractall(destination, members=members)


def fetch_seed_sources(settings: Settings) -> dict[str, Any]:
    config = load_static_libraries(settings)
    root = _fid_root(settings) / "sources"
    archives = root / "archives"
    unpacked = root / "unpacked"
    records = []
    for library in config.libraries:
        source = library.source
        if source is None or source.sha256 is None:
            records.append({"library": library.id, "status": "skipped-unpinned"})
            continue
        suffix = ".tar.gz" if source.url.endswith((".tar.gz", ".tgz")) else Path(source.url).suffix
        archive = archives / f"{library.id}{suffix}"
        if not archive.is_file() or sha256_file(archive) != source.sha256:
            payload = _download(source.url)
            if hashlib.sha256(payload).hexdigest() != source.sha256:
                raise RuntimeError(f"download hash mismatch for {library.id}")
            atomic_write(archive, payload)
        destination = unpacked / library.id
        expected_root = destination / source.archive_root
        if not expected_root.is_dir():
            if destination.exists():
                shutil.rmtree(destination)
            if zipfile.is_zipfile(archive):
                _safe_extract_zip(archive, destination)
            else:
                _safe_extract_tar(archive, destination)
        if not expected_root.is_dir():
            raise RuntimeError(f"{library.id} archive did not contain {source.archive_root}")
        manifest = tree_manifest(expected_root)
        records.append(
            {
                "library": library.id,
                "status": "ready",
                "url": source.url,
                "archive_sha256": source.sha256,
                "source_tree_hash": tree_hash(manifest),
                "file_count": len(manifest),
            }
        )
    result = {"schema": "wiz8.fid-source-fetch", "sources": records}
    atomic_json(settings.build_dir / "manifests" / "fid-sources.json", result)
    return result


def build_toolchain_images(
    settings: Settings, toolchain_ids: list[str] | None = None
) -> dict[str, Any]:
    config = load_static_libraries(settings)
    docker = tool_version("docker", ("--version",))
    if docker["executable"] is None:
        raise RuntimeError("docker is required to build MSVC600 FID seeds")
    records = []
    context = settings.repo_dir / "docker" / "msvc600"
    for toolchain in select_toolchains(config, toolchain_ids):
        result = run(
            [
                docker["executable"],
                "build",
                "--pull",
                "--network",
                "host",
                "--build-arg",
                f"MSVC_REPOSITORY={toolchain.repository}",
                "--build-arg",
                f"MSVC_REF={toolchain.commit}",
                "--tag",
                toolchain.image,
                ".",
            ],
            cwd=context,
            log_path=settings.build_dir / "logs" / "fid" / f"docker-build-{toolchain.id}.json",
        )
        records.append(
            {
                "id": toolchain.id,
                "image": toolchain.image,
                "repository": toolchain.repository,
                "commit": toolchain.commit,
                "command": result.command,
            }
        )
    summary = {
        "schema": "wiz8.fid-toolchain-image",
        "docker": docker,
        "toolchains": records,
    }
    atomic_json(settings.build_dir / "manifests" / "fid-toolchain-image.json", summary)
    return summary


def _docker_cmake_build(
    settings: Settings,
    toolchain: Toolchain,
    *,
    output: Path,
    target: str,
    definitions: dict[str, str],
    source_mounts: dict[str, Path] | None = None,
    log_name: str,
) -> None:
    docker = tool_version("docker", ("--version",))
    if docker["executable"] is None:
        raise RuntimeError("docker is required to build MSVC600 FID seeds")
    output.mkdir(parents=True, exist_ok=True)
    (output / "tmp").mkdir(exist_ok=True)
    volumes = [
        "--volume",
        f"{settings.repo_dir.resolve()}:/repo:ro",
        "--volume",
        f"{output.resolve()}:/out",
    ]
    for name, source in sorted((source_mounts or {}).items()):
        volumes.extend(["--volume", f"{source.resolve()}:/sources/{name}:ro"])
    run(
        [
            docker["executable"],
            "run",
            "--rm",
            "--network",
            "none",
            *volumes,
            toolchain.image,
            r"C:\cmake\bin\cmake.exe",
            "-S",
            "Z:/repo",
            "-B",
            "Z:/out",
            "-G",
            "NMake Makefiles",
            "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
            *[f"-D{key}={value}" for key, value in sorted(definitions.items())],
        ],
        cwd=settings.repo_dir,
        log_path=settings.build_dir / "logs" / "fid" / f"{log_name}-configure.json",
    )
    run(
        [
            docker["executable"],
            "run",
            "--rm",
            "--network",
            "none",
            *volumes,
            toolchain.image,
            r"C:\cmake\bin\cmake.exe",
            "--build",
            "Z:/out",
            "--target",
            target,
        ],
        cwd=settings.repo_dir,
        log_path=settings.build_dir / "logs" / "fid" / f"{log_name}-build.json",
    )


def _cmake_seed_target(library: str, variant: str) -> str:
    normalize = lambda value: re.sub(r"[^a-z0-9]+", "_", value.casefold()).strip("_")
    return f"fid_{normalize(library)}_{normalize(variant)}"


def _publish_cmake_objects(source: Path, output: Path) -> list[dict[str, Any]]:
    objects = sorted(source.rglob("*.obj"), key=lambda path: path.as_posix().casefold())
    if not objects:
        raise RuntimeError(f"CMake target produced no COFF objects under {source}")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(tempfile.mkdtemp(prefix=f".{output.name}.", dir=output.parent))
    backup = output.with_name(f".{output.name}.previous")
    try:
        for index, source_object in enumerate(objects):
            stem = re.sub(r"[^A-Za-z0-9_.-]+", "-", source_object.stem).strip("-.")
            destination = temporary / f"{index:04d}--{stem or 'object'}.obj"
            shutil.copyfile(source_object, destination)
            _normalize_coff_timestamp(destination)
        manifest = tree_manifest(temporary)
        if backup.exists():
            shutil.rmtree(backup)
        if output.exists():
            output.replace(backup)
        temporary.replace(output)
        shutil.rmtree(backup, ignore_errors=True)
        return manifest
    except Exception:
        shutil.rmtree(temporary, ignore_errors=True)
        if backup.exists() and not output.exists():
            backup.replace(output)
        raise


def _docker_copy_from_image(
    settings: Settings,
    toolchain: Toolchain,
    image_path: str,
    output: Path,
    *,
    log_name: str,
) -> None:
    docker = tool_version("docker", ("--version",))
    if docker["executable"] is None:
        raise RuntimeError("docker is required to extract pinned MSVC600 library snapshots")
    output.parent.mkdir(parents=True, exist_ok=True)
    container_path = "/root/.wine/drive_c/msvc/" + image_path.replace("\\", "/").lstrip("/")
    run(
        [
            docker["executable"],
            "run",
            "--rm",
            "--network",
            "none",
            "--volume",
            f"{output.parent.resolve()}:/out",
            "--entrypoint",
            "/bin/cp",
            toolchain.image,
            container_path,
            f"/out/{output.name}",
        ],
        cwd=settings.repo_dir,
        log_path=settings.build_dir / "logs" / "fid" / f"{log_name}.json",
    )
    if not output.is_file():
        raise RuntimeError(
            f"{toolchain.image} did not contain {image_path}; "
            f"build the pinned image with 'wiz8 ghidra fid build-image --toolchain {toolchain.id}'"
        )


def _configured_seed_keys(config: StaticLibrariesConfig) -> set[tuple[str, str, str]]:
    compiled = {
        (toolchain.id, library.id, variant.id)
        for toolchain in config.toolchains
        if "compiler" in toolchain.capabilities
        for library in config.libraries
        for variant in library.seed_variants
        if library.source is not None
        and library.source.sha256 is not None
        and library.compilation_units
    }
    precompiled = {
        (toolchain.id, archive.library, archive.variant)
        for toolchain in config.toolchains
        for archive in toolchain.precompiled_archives
        if archive.seed
    }
    return compiled | precompiled


def _write_archive_members(
    archive_path: Path, output: Path
) -> tuple[list[dict[str, Any]], dict[str, int]]:
    members = read_coff_archive(archive_path)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(tempfile.mkdtemp(prefix=f".{output.name}.", dir=output.parent))
    backup = output.with_name(f".{output.name}.previous")
    records: list[dict[str, Any]] = []
    counts: dict[str, int] = {}
    try:
        for member in members:
            kind = coff_member_kind(member.data)
            counts[kind] = counts.get(kind, 0) + 1
            if kind != "coff-i386":
                continue
            digest = hashlib.sha256(member.data).hexdigest()
            basename = Path(member.name.replace("\\", "/")).stem
            slug = re.sub(r"[^A-Za-z0-9_.-]+", "-", basename).strip("-.") or "member"
            filename = f"{member.index:04d}--{slug[:64]}--{digest[:12]}.obj"
            atomic_write(temporary / filename, member.data)
            records.append(
                {
                    "path": filename,
                    "size": len(member.data),
                    "sha256": digest,
                    "archive_member": member.name,
                    "archive_member_index": member.index,
                    "kind": kind,
                }
            )
        if backup.exists():
            shutil.rmtree(backup)
        if output.exists():
            output.replace(backup)
        temporary.replace(output)
        if backup.exists():
            shutil.rmtree(backup)
    except Exception:
        shutil.rmtree(temporary, ignore_errors=True)
        if backup.exists() and not output.exists():
            backup.replace(output)
        raise
    return records, counts


def extract_precompiled_objects(
    settings: Settings,
    toolchain_ids: list[str] | None = None,
) -> dict[str, Any]:
    """Extract exact i386 COFF members from pinned VC6 library snapshots."""
    config = load_static_libraries(settings)
    toolchains = select_toolchains(config, toolchain_ids, capability="precompiled-libraries")
    library_ids = {library.id for library in config.libraries}
    records: list[dict[str, Any]] = []
    selected_keys: set[tuple[str, str, str]] = set()
    archive_root = _fid_root(settings) / "precompiled-archives"
    object_root = _fid_root(settings) / "objects"
    for toolchain in toolchains:
        for archive in sorted(toolchain.precompiled_archives, key=lambda item: item.id):
            if not archive.seed:
                continue
            if archive.library not in library_ids:
                raise RuntimeError(
                    f"{toolchain.id}/{archive.id} references unknown library {archive.library}"
                )
            selected_keys.add((toolchain.id, archive.library, archive.variant))
            archive_path = archive_root / toolchain.id / f"{archive.id}.lib"
            if not archive_path.is_file() or sha256_file(archive_path) != archive.sha256:
                _docker_copy_from_image(
                    settings,
                    toolchain,
                    archive.path,
                    archive_path,
                    log_name=f"copy-{toolchain.id}-{archive.id}",
                )
            actual_hash = sha256_file(archive_path)
            if actual_hash != archive.sha256:
                raise RuntimeError(
                    f"archive hash mismatch for {toolchain.id}/{archive.id}: "
                    f"expected {archive.sha256}, got {actual_hash}"
                )
            output = object_root / toolchain.id / archive.library / archive.variant
            output.parent.mkdir(parents=True, exist_ok=True)
            objects, member_counts = _write_archive_members(archive_path, output)
            records.append(
                {
                    "toolchain": toolchain.id,
                    "toolchain_commit": toolchain.commit,
                    "library": archive.library,
                    "version": next(
                        item.version for item in config.libraries if item.id == archive.library
                    ),
                    "variant": archive.variant,
                    "flags": [],
                    "source_kind": "precompiled-archive",
                    "archive": {
                        "id": archive.id,
                        "path": archive.path,
                        "sha256": archive.sha256,
                        "member_counts": dict(sorted(member_counts.items())),
                    },
                    "object_count": len(objects),
                    "tree_hash": tree_hash(objects),
                    "objects": objects,
                }
            )

    manifest_path = settings.build_dir / "manifests" / "fid-seed-objects.json"
    preserved: list[dict[str, Any]] = []
    configured_keys = _configured_seed_keys(config)
    if manifest_path.is_file():
        previous = json.loads(manifest_path.read_text(encoding="utf-8"))
        for record in previous.get("libraries", []):
            key = (record["toolchain"], record["library"], record["variant"])
            if key in configured_keys and key not in selected_keys:
                preserved.append(record)
    combined = sorted(
        preserved + records, key=lambda item: (item["toolchain"], item["library"], item["variant"])
    )
    result = {
        "schema": "wiz8.fid-seed-objects",
        "toolchains": [
            item.model_dump(mode="json")
            for item in sorted(config.toolchains, key=lambda item: item.id)
        ],
        "libraries": combined,
    }
    atomic_json(manifest_path, result)
    return result


def _normalize_coff_timestamp(path: Path) -> None:
    data = bytearray(path.read_bytes())
    if len(data) < 20:
        raise RuntimeError(f"COFF object is too short: {path}")
    struct.pack_into("<I", data, 4, 0)
    atomic_write(path, bytes(data))


def _prepared_source(settings: Settings, library: StaticLibrary, pristine: Path) -> Path:
    if library.source_overlay is None:
        return pristine
    overlay = (settings.repo_dir / library.source_overlay).resolve()
    if settings.repo_dir.resolve() not in overlay.parents or not overlay.is_dir():
        raise RuntimeError(f"invalid source overlay for {library.id}: {library.source_overlay}")
    prepared = _fid_root(settings) / "sources" / "prepared" / library.id
    if prepared.exists():
        shutil.rmtree(prepared)
    shutil.copytree(pristine, prepared)
    for source in sorted(overlay.rglob("*")):
        if source.is_file():
            relative = source.relative_to(overlay)
            destination = prepared / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
    return prepared


def build_seed_objects(
    settings: Settings,
    toolchain_ids: list[str] | None = None,
    library_ids: list[str] | None = None,
) -> dict[str, Any]:
    fetch_seed_sources(settings)
    config = load_static_libraries(settings)
    source_root = _fid_root(settings) / "sources" / "unpacked"
    output_root = _fid_root(settings) / "objects"
    records: list[dict[str, Any]] = []
    selected_toolchains = select_toolchains(config, toolchain_ids, capability="compiler")
    selected_libraries = select_libraries(config, library_ids)
    prepared_sources: dict[str, Path] = {}
    cmake_variables = {
        "zlib-1.0.4": "FID_ZLIB_SOURCE",
        "ijg-jpeg-6": "FID_JPEG_SOURCE",
        "infozip-unzip-5.4": "FID_INFOZIP_SOURCE",
    }
    for library in selected_libraries:
        if library.source is None or library.source.sha256 is None or not library.compilation_units:
            continue
        pristine = source_root / library.id / library.source.archive_root
        prepared_sources[library.id] = _prepared_source(settings, library, pristine)

    for toolchain in selected_toolchains:
        temporary = Path(tempfile.mkdtemp(prefix=f"cmake-{toolchain.id}-", dir=_fid_root(settings)))
        try:
            definitions = {
                "WIZ8_BUILD_DECOMP": "OFF",
                "WIZ8_BUILD_FID_SEEDS": "ON",
            }
            mounts: dict[str, Path] = {}
            for index, (library_id, source) in enumerate(sorted(prepared_sources.items())):
                variable = cmake_variables.get(library_id)
                if variable is None:
                    raise RuntimeError(f"no CMake FID target is defined for {library_id}")
                mount_name = f"source-{index}"
                mounts[mount_name] = source
                definitions[variable] = f"Z:/sources/{mount_name}"
            _docker_cmake_build(
                settings,
                toolchain,
                output=temporary,
                target="fid-seeds",
                definitions=definitions,
                source_mounts=mounts,
                log_name=f"cmake-seeds-{toolchain.id}",
            )

            for library in selected_libraries:
                if library.id not in prepared_sources:
                    continue
                for variant in library.seed_variants:
                    target = _cmake_seed_target(library.id, variant.id)
                    cmake_objects = temporary / "CMakeFiles" / f"{target}.dir"
                    output = output_root / toolchain.id / library.id / variant.id
                    manifest = _publish_cmake_objects(cmake_objects, output)
                    records.append(
                        {
                            "toolchain": toolchain.id,
                            "toolchain_commit": toolchain.commit,
                            "library": library.id,
                            "version": library.version,
                            "variant": variant.id,
                            "flags": variant.flags,
                            "source_kind": "cmake-object-library",
                            "cmake_target": target,
                            "object_count": len(manifest),
                            "tree_hash": tree_hash(manifest),
                            "objects": manifest,
                        }
                    )
        finally:
            shutil.rmtree(temporary, ignore_errors=True)
    manifest_path = settings.build_dir / "manifests" / "fid-seed-objects.json"
    selected_pairs = {
        (toolchain.id, library.id)
        for toolchain in selected_toolchains
        for library in selected_libraries
    }
    configured_keys = _configured_seed_keys(config)
    preserved = []
    if manifest_path.is_file() and (toolchain_ids or library_ids):
        previous = json.loads(manifest_path.read_text(encoding="utf-8"))
        for record in previous.get("libraries", []):
            key = (record["toolchain"], record["library"], record["variant"])
            if (
                key in configured_keys
                and (record["toolchain"], record["library"]) not in selected_pairs
            ):
                preserved.append(record)
    combined = sorted(
        preserved + records,
        key=lambda item: (item["toolchain"], item["library"], item["variant"]),
    )
    result = {
        "schema": "wiz8.fid-seed-objects",
        "toolchains": [
            item.model_dump(mode="json")
            for item in sorted(config.toolchains, key=lambda item: item.id)
        ],
        "libraries": combined,
    }
    atomic_json(manifest_path, result)
    return result


def probe_toolchains(settings: Settings, toolchain_ids: list[str] | None = None) -> dict[str, Any]:
    config = load_static_libraries(settings)
    records = []
    for toolchain in select_toolchains(config, toolchain_ids, capability="compiler"):
        output = _fid_root(settings) / "toolchain-probes" / toolchain.id
        if output.exists():
            shutil.rmtree(output)
        _docker_cmake_build(
            settings,
            toolchain,
            output=output,
            target="fid-probe",
            definitions={
                "WIZ8_BUILD_DECOMP": "OFF",
                "WIZ8_BUILD_TOOLCHAIN_PROBE": "ON",
            },
            log_name=f"cmake-toolchain-probe-{toolchain.id}",
        )
        executable = output / "rich_probe.exe"
        if not executable.is_file():
            raise RuntimeError(f"{toolchain.id} did not produce rich_probe.exe")
        records.append(
            {
                "id": toolchain.id,
                "image": toolchain.image,
                "repository": toolchain.repository,
                "commit": toolchain.commit,
                "sha256": sha256_file(executable),
                "rich_header": parse_rich_header(executable),
            }
        )
    result = {
        "schema": "wiz8.fid-toolchain-probe",
        "toolchains": records,
    }
    atomic_json(settings.build_dir / "reports" / "fid-toolchain-probe.json", result)
    return result
