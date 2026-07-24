from __future__ import annotations

import shutil
import struct
import tarfile
import urllib.request
from pathlib import Path
from typing import Any

import yaml
from pydantic import BaseModel, ConfigDict, Field, field_validator

from ..binary.rich_header import parse_rich_header
from ..config import Settings
from ..paths import atomic_json, atomic_write, ensure_safe_generated_target, sha256_file, tree_hash, tree_manifest
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


class Toolchain(BaseModel):
    id: str
    family: str
    repository: str
    commit: str
    image: str
    status: str

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


class ToolchainEvidence(BaseModel):
    selection_confidence: str
    evidence: list[str]
    uncertainty: list[str]


class StaticLibrariesConfig(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    schema_name: str = Field(alias="schema")
    format_version: int
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


def load_static_libraries(settings: Settings) -> StaticLibrariesConfig:
    path = settings.repo_dir / "config" / "static-libraries.yml"
    return StaticLibrariesConfig.model_validate(yaml.safe_load(path.read_text(encoding="utf-8")))


def select_toolchains(config: StaticLibrariesConfig, ids: list[str] | None = None) -> list[Toolchain]:
    by_id = {toolchain.id: toolchain for toolchain in config.toolchains}
    if not ids:
        return sorted(config.toolchains, key=lambda item: item.id)
    unknown = sorted(set(ids) - set(by_id))
    if unknown:
        raise RuntimeError(f"unknown toolchain id(s): {', '.join(unknown)}")
    return [by_id[item] for item in dict.fromkeys(ids)]


def _fid_root(settings: Settings) -> Path:
    root = settings.work_dir / "fid"
    ensure_safe_generated_target(root, settings.work_dir)
    return root


def static_inventory(settings: Settings) -> dict[str, Any]:
    config = load_static_libraries(settings)
    result = {
        "schema": "wiz8.static-library-inventory",
        "format_version": 1,
        "toolchains": [toolchain.model_dump(mode="json") for toolchain in config.toolchains],
        "toolchain_evidence": config.toolchain_evidence.model_dump(mode="json"),
        "libraries": [library.model_dump(mode="json") for library in config.libraries],
        "seedable_now": [
            library.id
            for library in config.libraries
            if library.source is not None
            and library.source.sha256 is not None
            and library.seed_variants
            and library.compilation_units
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
            if not (
                library.source is not None
                and library.source.sha256 is not None
                and library.seed_variants
                and library.compilation_units
            )
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
            f"{'ready' if library.id in seedable else 'blocked'} |"
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
            if __import__("hashlib").sha256(payload).hexdigest() != source.sha256:
                raise RuntimeError(f"download hash mismatch for {library.id}")
            atomic_write(archive, payload)
        destination = unpacked / library.id
        expected_root = destination / source.archive_root
        if not expected_root.is_dir():
            if destination.exists():
                shutil.rmtree(destination)
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
    result = {"schema": "wiz8.fid-source-fetch", "format_version": 1, "sources": records}
    atomic_json(settings.build_dir / "manifests" / "fid-sources.json", result)
    return result


def build_toolchain_images(settings: Settings, toolchain_ids: list[str] | None = None) -> dict[str, Any]:
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
        "format_version": 1,
        "docker": docker,
        "toolchains": records,
    }
    atomic_json(settings.build_dir / "manifests" / "fid-toolchain-image.json", summary)
    return summary


def _docker_run(
    settings: Settings,
    toolchain: Toolchain,
    arguments: list[str],
    *,
    sources: Path,
    output: Path,
    log_name: str,
) -> None:
    docker = tool_version("docker", ("--version",))
    if docker["executable"] is None:
        raise RuntimeError("docker is required to build MSVC600 FID seeds")
    output.mkdir(parents=True, exist_ok=True)
    run(
        [
            docker["executable"],
            "run",
            "--rm",
            "--network",
            "none",
            "--volume",
            f"{sources.resolve()}:/src:ro",
            "--volume",
            f"{output.resolve()}:/out",
            toolchain.image,
            *arguments,
        ],
        cwd=settings.repo_dir,
        log_path=settings.build_dir / "logs" / "fid" / f"{log_name}.json",
    )


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


def build_seed_objects(settings: Settings, toolchain_ids: list[str] | None = None) -> dict[str, Any]:
    fetch_seed_sources(settings)
    config = load_static_libraries(settings)
    source_root = _fid_root(settings) / "sources" / "unpacked"
    output_root = _fid_root(settings) / "objects"
    records = []
    for toolchain in select_toolchains(config, toolchain_ids):
        for library in config.libraries:
            if library.source is None or library.source.sha256 is None or not library.compilation_units:
                continue
            pristine = source_root / library.id / library.source.archive_root
            source = _prepared_source(settings, library, pristine)
            for variant in library.seed_variants:
                output = output_root / toolchain.id / library.id / variant.id
                output.mkdir(parents=True, exist_ok=True)
                for unit in library.compilation_units:
                    object_path = output / (Path(unit).stem + ".obj")
                    windows_unit = unit.replace("/", "\\")
                    arguments = [
                        "CL.EXE",
                        *variant.flags,
                        f"/FoZ:\\out\\{object_path.name}",
                        f"Z:\\src\\{windows_unit}",
                    ]
                    _docker_run(
                        settings,
                        toolchain,
                        arguments,
                        sources=source,
                        output=output,
                        log_name=f"compile-{toolchain.id}-{library.id}-{variant.id}-{Path(unit).stem}",
                    )
                    if not object_path.is_file():
                        raise RuntimeError(f"compiler did not produce {object_path}")
                    _normalize_coff_timestamp(object_path)
                manifest = tree_manifest(output)
                records.append(
                    {
                        "toolchain": toolchain.id,
                        "toolchain_commit": toolchain.commit,
                        "library": library.id,
                        "version": library.version,
                        "variant": variant.id,
                        "flags": variant.flags,
                        "object_count": len(manifest),
                        "tree_hash": tree_hash(manifest),
                        "objects": manifest,
                    }
                )
    result = {
        "schema": "wiz8.fid-seed-objects",
        "format_version": 1,
        "toolchains": [item.model_dump(mode="json") for item in select_toolchains(config, toolchain_ids)],
        "libraries": records,
    }
    atomic_json(settings.build_dir / "manifests" / "fid-seed-objects.json", result)
    return result


def probe_toolchains(settings: Settings, toolchain_ids: list[str] | None = None) -> dict[str, Any]:
    config = load_static_libraries(settings)
    probe = settings.repo_dir / "docker" / "msvc600" / "probes"
    arguments = [
        "CL.EXE",
        "/nologo",
        "/O2",
        "/MD",
        "/FeZ:\\out\\rich_probe.exe",
        "Z:\\src\\rich_probe.cpp",
    ]
    records = []
    for toolchain in select_toolchains(config, toolchain_ids):
        output = _fid_root(settings) / "toolchain-probes" / toolchain.id
        _docker_run(
            settings,
            toolchain,
            arguments,
            sources=probe,
            output=output,
            log_name=f"toolchain-probe-{toolchain.id}",
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
        "format_version": 1,
        "toolchains": records,
    }
    atomic_json(settings.build_dir / "reports" / "fid-toolchain-probe.json", result)
    return result
