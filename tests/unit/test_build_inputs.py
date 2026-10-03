from __future__ import annotations

import zipfile
from pathlib import Path

import pytest
from wiz8decomp.build_inputs import (
    _download_verified,
    _safe_extract_zip,
)


def test_safe_zip_extraction_rejects_parent_traversal(tmp_path: Path) -> None:
    archive = tmp_path / "bad.zip"
    with zipfile.ZipFile(archive, "w") as stream:
        stream.writestr("../escaped.txt", "bad")
    with pytest.raises(RuntimeError, match="escapes extraction root"):
        _safe_extract_zip(archive, tmp_path / "output")
    assert not (tmp_path / "escaped.txt").exists()


def test_safe_zip_extraction_accepts_normal_tree(tmp_path: Path) -> None:
    archive = tmp_path / "good.zip"
    with zipfile.ZipFile(archive, "w") as stream:
        stream.writestr("source/unit.c", "int unit(void) { return 1; }")
    output = tmp_path / "output"
    _safe_extract_zip(archive, output)
    assert (output / "source" / "unit.c").is_file()


def _pin(payload: bytes) -> str:
    import hashlib

    return hashlib.sha256(payload).hexdigest()


def test_download_verified_falls_through_after_network_failure(monkeypatch, capfd) -> None:
    payload = b"reviewed archive bytes"

    def fake_download(url: str, **kwargs) -> bytes:
        if "dead.example" in url:
            raise OSError("connection refused")
        return payload

    monkeypatch.setattr("wiz8decomp.build_inputs._download", fake_download)

    result = _download_verified(
        ["https://dead.example/a.tar.gz", "https://live.example/a.tar.gz"],
        _pin(payload),
        label="zlib-1.0.4",
    )

    assert result == payload
    assert "dead.example" in capfd.readouterr().err


def test_download_verified_rejects_mutated_mirror_and_uses_next(monkeypatch, capfd) -> None:
    payload = b"reviewed archive bytes"

    def fake_download(url: str, **kwargs) -> bytes:
        return b"mutated mirror bytes" if "mutated" in url else payload

    monkeypatch.setattr("wiz8decomp.build_inputs._download", fake_download)

    result = _download_verified(
        ["https://mutated.example/a.tar.gz", "https://good.example/a.tar.gz"],
        _pin(payload),
        label="jpeg-6",
    )

    assert result == payload
    assert "hash mismatch" in capfd.readouterr().err


def test_download_verified_exhaustion_reports_each_source(monkeypatch) -> None:
    payload = b"reviewed archive bytes"

    def fake_download(url: str, **kwargs) -> bytes:
        if "dead" in url:
            raise OSError("gone")
        return b"different bytes"

    monkeypatch.setattr("wiz8decomp.build_inputs._download", fake_download)

    with pytest.raises(RuntimeError, match="all reviewed sources exhausted") as error:
        _download_verified(
            ["https://dead.example/a.zip", "https://mutated.example/a.zip"],
            _pin(payload),
            label="unzip540",
        )

    message = str(error.value)
    assert "dead.example/a.zip: network error" in message
    assert "mutated.example/a.zip: hash mismatch" in message


@pytest.mark.parametrize("emulate", ["0", "1"])
def test_toolchain_build_forwards_qemu_and_cloud_trust(tmp_path, monkeypatch, emulate) -> None:
    from types import SimpleNamespace

    from wiz8decomp import build_inputs
    from wiz8decomp.config import Settings

    settings = Settings.model_construct(
        repo_dir=tmp_path,
        work_dir=tmp_path / "work",
        input_dir=tmp_path / "input",
        ghidra_install_dir=tmp_path / "ghidra",
    )
    toolchain = SimpleNamespace(
        id="vc6-sp5", image="vc6:sp5", repository="https://example.test/vc6", commit="a" * 40
    )
    monkeypatch.setattr(
        build_inputs, "load_build_inputs", lambda _: SimpleNamespace(toolchains=[toolchain])
    )
    monkeypatch.setattr(build_inputs, "tool_version", lambda *_: {"executable": "docker"})
    monkeypatch.setenv("WIZ8_EMULATE_I386", emulate)
    monkeypatch.setenv("CODEX_PROXY_CERT", str(tmp_path / "ca.crt"))
    commands = []

    def run(command, **kwargs):
        commands.append(command)
        return SimpleNamespace(command="docker build")

    monkeypatch.setattr(build_inputs, "run", run)
    build_inputs.build_toolchain_images(settings, ["vc6-sp5"])

    assert len(commands) == 2
    assert [command[command.index("--target") + 1] for command in commands] == [
        "product",
        "analysis",
    ]
    for command in commands:
        assert f"WIZ8_EMULATE_I386={emulate}" in command
        assert command[command.index("--secret") + 1] == f"id=proxy_ca,src={tmp_path / 'ca.crt'}"
        assert "--network" not in command
