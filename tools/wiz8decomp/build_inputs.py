from __future__ import annotations

import hashlib
import os
import shutil
import sys
import tarfile
import tempfile
import time
import urllib.request
import zipfile
from pathlib import Path
from typing import Any, Literal

import yaml
from pydantic import BaseModel, ConfigDict, Field, field_validator

from .config import Settings
from .paths import (
    atomic_json,
    atomic_write,
    ensure_safe_generated_target,
    sha256_file,
    tree_hash,
    tree_manifest,
)
from .subprocesses import run, tool_version


class SourceArchive(BaseModel):
    url: str
    sha256: str
    archive_root: str
    # Additional reviewed mirrors for the identical pinned archive. Each is
    # tried in order after `url`; a payload is accepted only when it matches
    # `sha256`, so a mirror can never substitute different bytes.
    mirrors: list[str] = Field(default_factory=list)


class SourceOverlayArchive(SourceArchive):
    members: list[str]


class SourceLibrary(BaseModel):
    model_config = ConfigDict(populate_by_name=True)

    id: str
    family: str
    version: str
    status: str
    scope: list[str]
    evidence: list[str]
    source: SourceArchive
    source_overlay_archives: list[SourceOverlayArchive] = Field(default_factory=list)


class Toolchain(BaseModel):
    id: str
    family: str
    repository: str
    commit: str
    image: str

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


class BuildInputsConfig(BaseModel):
    model_config = ConfigDict(extra="forbid", populate_by_name=True)

    schema_name: Literal["wiz8.build-inputs"] = Field(alias="schema")
    toolchains: list[Toolchain]
    libraries: list[SourceLibrary]

    @field_validator("toolchains")
    @classmethod
    def unique_toolchains(cls, value: list[Toolchain]) -> list[Toolchain]:
        ids = [toolchain.id for toolchain in value]
        if len(ids) != len(set(ids)):
            raise ValueError("toolchain ids must be unique")
        return value

    @field_validator("libraries")
    @classmethod
    def unique_libraries(cls, value: list[SourceLibrary]) -> list[SourceLibrary]:
        ids = [library.id for library in value]
        if len(ids) != len(set(ids)):
            raise ValueError("static-library ids must be unique")
        return value


def load_build_inputs(settings: Settings) -> BuildInputsConfig:
    path = settings.repo_dir / "config" / "build-inputs.yml"
    return BuildInputsConfig.model_validate(yaml.safe_load(path.read_text(encoding="utf-8")))


def select_toolchains(
    config: BuildInputsConfig,
    ids: list[str] | None = None,
) -> list[Toolchain]:
    by_id = {toolchain.id: toolchain for toolchain in config.toolchains}
    if not ids:
        selected = sorted(config.toolchains, key=lambda item: item.id)
        return selected
    unknown = sorted(set(ids) - set(by_id))
    if unknown:
        raise RuntimeError(f"unknown toolchain id(s): {', '.join(unknown)}")
    selected = [by_id[item] for item in dict.fromkeys(ids)]
    return selected


def _download(url: str, *, attempts: int = 3) -> bytes:
    """Fetch ``url`` with a few retries for transient CDN/hash flakes."""

    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        try:
            request = urllib.request.Request(url, headers={"User-Agent": "wizardry8-decomp/0.1"})
            with urllib.request.urlopen(request, timeout=120) as response:
                return response.read()
        except Exception as exc:  # noqa: BLE001 — retry transient network failures
            last_error = exc
            if attempt < attempts:
                time.sleep(min(2**attempt, 8))
    assert last_error is not None
    raise last_error


def _download_verified(urls: list[str], expected_sha256: str, *, label: str) -> bytes:
    """Download the pinned archive from the first reviewed source that matches.

    Sources are tried in order. A network failure or a hash mismatch falls
    through to the next reviewed mirror; a payload is accepted only when its
    SHA-256 equals ``expected_sha256``. The exhaustion error lists each
    source's outcome so logs distinguish dead mirrors from mutated archives.
    """

    failures: list[str] = []
    for url in urls:
        try:
            payload = _download(url)
        except Exception as exc:  # noqa: BLE001 — fall through to the next mirror
            print(
                f"download {label}: source {url} failed (network): {exc}",
                file=sys.stderr,
                flush=True,
            )
            failures.append(f"{url}: network error: {exc}")
            continue
        digest = hashlib.sha256(payload).hexdigest()
        if digest == expected_sha256:
            return payload
        print(
            f"download {label}: source {url} hash mismatch: got {digest}, "
            f"expected {expected_sha256}",
            file=sys.stderr,
            flush=True,
        )
        failures.append(f"{url}: hash mismatch: got {digest}")
    raise RuntimeError(
        f"all reviewed sources exhausted for {label} "
        f"(expected sha256 {expected_sha256}): " + "; ".join(failures)
    )


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


def fetch_sources(settings: Settings) -> dict[str, Any]:
    config = load_build_inputs(settings)
    root = settings.work_dir / "sources"
    ensure_safe_generated_target(root, settings.work_dir)
    archives = root / "archives"
    unpacked = root / "unpacked"
    records = []
    for library in config.libraries:
        source = library.source
        suffix = ".tar.gz" if source.url.endswith((".tar.gz", ".tgz")) else Path(source.url).suffix
        archive = archives / f"{library.id}{suffix}"
        if not archive.is_file() or sha256_file(archive) != source.sha256:
            payload = _download_verified(
                [source.url, *source.mirrors], source.sha256, label=library.id
            )
            atomic_write(archive, payload)
        destination = unpacked / library.id
        destination.parent.mkdir(parents=True, exist_ok=True)
        temporary = Path(tempfile.mkdtemp(prefix=f".{library.id}.", dir=destination.parent))
        candidate = temporary / "tree"
        backup = destination.with_name(f".{destination.name}.previous")
        overlay_records = []
        try:
            if zipfile.is_zipfile(archive):
                _safe_extract_zip(archive, candidate)
            else:
                _safe_extract_tar(archive, candidate)
            expected_root = candidate / source.archive_root
            if not expected_root.is_dir():
                raise RuntimeError(f"{library.id} archive did not contain {source.archive_root}")
            for index, overlay in enumerate(library.source_overlay_archives):
                suffix = (
                    ".tar.gz"
                    if overlay.url.endswith((".tar.gz", ".tgz"))
                    else Path(overlay.url).suffix
                )
                overlay_archive = archives / f"{library.id}-overlay-{index}{suffix}"
                if not overlay_archive.is_file() or sha256_file(overlay_archive) != overlay.sha256:
                    payload = _download_verified(
                        [overlay.url, *overlay.mirrors],
                        overlay.sha256,
                        label=f"{library.id} overlay {index}",
                    )
                    atomic_write(overlay_archive, payload)
                with tempfile.TemporaryDirectory(dir=root) as overlay_temporary:
                    overlay_destination = Path(overlay_temporary)
                    if zipfile.is_zipfile(overlay_archive):
                        _safe_extract_zip(overlay_archive, overlay_destination)
                    else:
                        _safe_extract_tar(overlay_archive, overlay_destination)
                    overlay_root = (overlay_destination / overlay.archive_root).resolve()
                    for member in overlay.members:
                        source_path = (overlay_root / member).resolve()
                        if source_path != overlay_root and overlay_root not in source_path.parents:
                            raise RuntimeError(
                                f"{library.id} overlay member escapes source root: {member}"
                            )
                        if not source_path.is_file():
                            raise RuntimeError(
                                f"{library.id} overlay archive did not contain {member}"
                            )
                        destination_path = expected_root / member
                        destination_path.parent.mkdir(parents=True, exist_ok=True)
                        atomic_write(destination_path, source_path.read_bytes())
                overlay_records.append(
                    {
                        "url": overlay.url,
                        "archive_sha256": overlay.sha256,
                        "members": overlay.members,
                    }
                )
            manifest = tree_manifest(expected_root)
            if backup.exists():
                shutil.rmtree(backup)
            if destination.exists():
                destination.replace(backup)
            candidate.replace(destination)
            shutil.rmtree(backup, ignore_errors=True)
        except Exception:
            if backup.exists() and not destination.exists():
                backup.replace(destination)
            raise
        finally:
            shutil.rmtree(temporary, ignore_errors=True)
        records.append(
            {
                "library": library.id,
                "status": "ready",
                "url": source.url,
                "archive_sha256": source.sha256,
                "source_tree_hash": tree_hash(manifest),
                "file_count": len(manifest),
                "overlays": overlay_records,
            }
        )
    result = {"schema": "wiz8.source-fetch", "sources": records}
    atomic_json(settings.build_dir / "manifests" / "sources.json", result)
    return result


def product_image_name(image: str) -> str:
    """Return the product-only sibling tag for one analysis image tag."""
    return f"{image}-product"


def build_toolchain_images(
    settings: Settings, toolchain_ids: list[str] | None = None
) -> dict[str, Any]:
    config = load_build_inputs(settings)
    docker = tool_version("docker", ("--version",))
    if docker["executable"] is None:
        raise RuntimeError("docker is required to build the VC6 toolchain")
    records = []
    build_options = ["--build-arg", f"WIZ8_EMULATE_I386={os.environ.get('WIZ8_EMULATE_I386', '0')}"]
    if proxy_ca := os.environ.get("CODEX_PROXY_CERT"):
        build_options.extend(("--secret", f"id=proxy_ca,src={proxy_ca}"))
    context = settings.repo_dir / "docker" / "msvc600"
    for toolchain in select_toolchains(config, toolchain_ids):
        commands: dict[str, str] = {}
        for stage, image in (
            ("product", product_image_name(toolchain.image)),
            ("analysis", toolchain.image),
        ):
            result = run(
                [
                    docker["executable"],
                    "build",
                    "--pull",
                    *build_options,
                    "--build-arg",
                    f"MSVC_REPOSITORY={toolchain.repository}",
                    "--build-arg",
                    f"MSVC_REF={toolchain.commit}",
                    "--target",
                    stage,
                    "--tag",
                    image,
                    ".",
                ],
                cwd=context,
                log_path=settings.build_dir
                / "logs"
                / "toolchain"
                / f"docker-build-{toolchain.id}-{stage}.json",
            )
            commands[stage] = result.command
        records.append(
            {
                "id": toolchain.id,
                "image": toolchain.image,
                "product_image": product_image_name(toolchain.image),
                "repository": toolchain.repository,
                "commit": toolchain.commit,
                "commands": commands,
            }
        )
    summary = {
        "schema": "wiz8.toolchain-image",
        "docker": docker,
        "toolchains": records,
    }
    atomic_json(settings.build_dir / "manifests" / "toolchain-image.json", summary)
    return summary
