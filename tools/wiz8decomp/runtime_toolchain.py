from __future__ import annotations

import hashlib
import os
import shutil
import tarfile
import tempfile
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .config import Settings
from .paths import build_directory_atomically, sha256_file


@dataclass(frozen=True)
class RuntimeArchive:
    name: str
    url: str
    sha256: str
    archive_root: str | None
    destination: str
    sentinel: str


UMU_VERSION = "1.4.4"
GE_PROTON_VERSION = "GE-Proton11-7-x86_64"

UMU_ARCHIVE = RuntimeArchive(
    name=f"umu-launcher-{UMU_VERSION}-zipapp.tar",
    url=(
        "https://github.com/Open-Wine-Components/umu-launcher/releases/download/"
        f"{UMU_VERSION}/umu-launcher-{UMU_VERSION}-zipapp.tar"
    ),
    sha256="eb590691841f7fad3fc3ad8fd5db4ccb87849fe7948e62b28ece7a4ee48cc851",
    archive_root="umu",
    destination=f"umu-launcher-{UMU_VERSION}",
    sentinel="umu-run",
)

GE_PROTON_ARCHIVE = RuntimeArchive(
    name=f"{GE_PROTON_VERSION}.tar.gz",
    url=(
        "https://github.com/GloriousEggroll/proton-ge-custom/releases/download/"
        f"GE-Proton11-7/{GE_PROTON_VERSION}.tar.gz"
    ),
    sha256="c5448b76a230384e2d7bc6beb5ccb97bafb7e2c3b6c527cb03a1a546bbcb00a0",
    archive_root=None,
    destination=GE_PROTON_VERSION,
    sentinel="proton",
)


def umu_run_path(settings: Settings) -> Path:
    return settings.runtime_toolchain_dir / UMU_ARCHIVE.destination / UMU_ARCHIVE.sentinel


def ge_proton_path(settings: Settings) -> Path:
    return settings.runtime_toolchain_dir / GE_PROTON_ARCHIVE.destination


def _download_verified(settings: Settings, archive: RuntimeArchive) -> Path:
    cache = settings.runtime_toolchain_dir / "downloads"
    cache.mkdir(parents=True, exist_ok=True)
    destination = cache / archive.name
    if destination.is_file():
        if sha256_file(destination) == archive.sha256:
            return destination
        destination.unlink()

    request = urllib.request.Request(
        archive.url,
        headers={"User-Agent": "wizardry-8-decomp/runtime-toolchain"},
    )
    fd, temporary_name = tempfile.mkstemp(prefix=f".{archive.name}.", dir=cache)
    os.close(fd)
    temporary = Path(temporary_name)
    digest = hashlib.sha256()
    try:
        with (
            urllib.request.urlopen(request, timeout=60) as response,
            temporary.open("wb") as stream,
        ):
            while chunk := response.read(4 * 1024 * 1024):
                digest.update(chunk)
                stream.write(chunk)
        actual = digest.hexdigest()
        if actual != archive.sha256:
            raise RuntimeError(
                f"{archive.name} SHA-256 mismatch: expected {archive.sha256}, got {actual}"
            )
        os.replace(temporary, destination)
    finally:
        temporary.unlink(missing_ok=True)
    return destination


def _materialize_archive(settings: Settings, archive: RuntimeArchive) -> Path:
    destination = settings.runtime_toolchain_dir / archive.destination
    marker = destination / ".wiz8-source-sha256"
    sentinel = destination / archive.sentinel
    if (
        sentinel.is_file()
        and marker.is_file()
        and marker.read_text(encoding="ascii").strip() == archive.sha256
    ):
        return destination

    if destination.exists():
        shutil.rmtree(destination)
    source = _download_verified(settings, archive)

    def build(candidate: Path) -> None:
        unpacked = candidate.parent / "unpacked"
        unpacked.mkdir()
        with tarfile.open(source) as bundle:
            bundle.extractall(unpacked, filter="data")
        if archive.archive_root is not None:
            root = unpacked / archive.archive_root
            if not root.is_dir():
                raise RuntimeError(
                    f"{archive.name} did not contain expected root {archive.archive_root}"
                )
        else:
            entries = list(unpacked.iterdir())
            roots = [entry for entry in entries if entry.is_dir()]
            if len(entries) != 1 or len(roots) != 1:
                raise RuntimeError(f"{archive.name} did not contain one top-level directory")
            root = roots[0]
        root.replace(candidate)
        (candidate / ".wiz8-source-sha256").write_text(archive.sha256 + "\n", encoding="ascii")

    build_directory_atomically(destination, settings.work_dir, build)
    if not sentinel.is_file():
        raise RuntimeError(f"{archive.name} did not provide expected {archive.sentinel}")
    return destination


def prepare_runtime_toolchain(settings: Settings) -> dict[str, Any]:
    """Materialize the pinned interactive/runtime-test launcher stack."""

    umu = _materialize_archive(settings, UMU_ARCHIVE)
    proton = _materialize_archive(settings, GE_PROTON_ARCHIVE)
    state = settings.runtime_toolchain_dir / "state"
    cache = settings.runtime_toolchain_dir / "cache"
    state.mkdir(parents=True, exist_ok=True)
    cache.mkdir(parents=True, exist_ok=True)
    return {
        "umu": str(umu / UMU_ARCHIVE.sentinel),
        "proton": str(proton),
        "state": str(state),
        "cache": str(cache),
    }
