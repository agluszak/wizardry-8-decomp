import io
import tarfile
from pathlib import Path

from wiz8decomp.config import Settings
from wiz8decomp.runtime_toolchain import (
    RuntimeArchive,
    _materialize_archive,
    ge_proton_path,
    umu_run_path,
)


def _settings(tmp_path: Path) -> Settings:
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": tmp_path / "ghidra",
            "WIZ8_INPUT_DIR": tmp_path / "inputs",
            "WIZ8_WORK_DIR": tmp_path / "work",
            "repo_dir": tmp_path / "repo",
        }
    )


def _tar_with_file(path: Path, root: str, relative: str, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = io.BytesIO(data)
    info = tarfile.TarInfo(f"{root}/{relative}")
    info.size = len(data)
    info.mode = 0o755
    with tarfile.open(path, "w") as bundle:
        bundle.addfile(info, payload)


def test_runtime_toolchain_paths_are_checkout_local(tmp_path: Path) -> None:
    settings = _settings(tmp_path)

    assert umu_run_path(settings) == (
        settings.work_dir / "runtime-toolchain/umu-launcher-1.4.4/umu-run"
    )
    assert ge_proton_path(settings) == settings.work_dir / "runtime-toolchain/GE-Proton11-7-x86_64"


def test_materialize_archive_uses_verified_cached_archive(
    tmp_path: Path, monkeypatch
) -> None:
    settings = _settings(tmp_path)
    source = tmp_path / "bundle.tar"
    _tar_with_file(source, "payload", "launcher", b"#!/bin/sh\n")

    import hashlib

    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    archive = RuntimeArchive(
        name="bundle.tar",
        url="https://example.invalid/bundle.tar",
        sha256=digest,
        archive_root=None,
        destination="runtime",
        sentinel="launcher",
    )
    downloads = settings.runtime_toolchain_dir / "downloads"
    downloads.mkdir(parents=True)
    cached = downloads / archive.name
    cached.write_bytes(source.read_bytes())

    monkeypatch.setattr(
        "wiz8decomp.runtime_toolchain.urllib.request.urlopen",
        lambda *args, **kwargs: (_ for _ in ()).throw(AssertionError("unexpected download")),
    )

    destination = _materialize_archive(settings, archive)

    assert (destination / "launcher").is_file()
    assert (destination / ".wiz8-source-sha256").read_text().strip() == digest
    assert (destination / "launcher").stat().st_mode & 0o111


def test_materialize_archive_reuses_matching_tree(tmp_path: Path, monkeypatch) -> None:
    settings = _settings(tmp_path)
    archive = RuntimeArchive(
        name="unused.tar",
        url="https://example.invalid/unused.tar",
        sha256="a" * 64,
        archive_root="payload",
        destination="runtime",
        sentinel="launcher",
    )
    destination = settings.runtime_toolchain_dir / archive.destination
    destination.mkdir(parents=True)
    launcher = destination / "launcher"
    launcher.write_text("#!/bin/sh\n")
    launcher.chmod(0o755)
    (destination / ".wiz8-source-sha256").write_text(archive.sha256 + "\n")

    monkeypatch.setattr(
        "wiz8decomp.runtime_toolchain._download_verified",
        lambda *args, **kwargs: (_ for _ in ()).throw(AssertionError("unexpected download")),
    )

    assert _materialize_archive(settings, archive) == destination
