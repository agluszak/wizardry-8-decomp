from __future__ import annotations

from pathlib import Path

from wiz8decomp.extract.variants import _apply_overlay


def test_overlay_uses_windows_case_insensitive_paths(tmp_path: Path) -> None:
    base = tmp_path / "base"
    patch = tmp_path / "patch"
    base.mkdir()
    patch.mkdir()
    (base / "Wiz8.exe").write_bytes(b"old")
    (patch / "Wiz8.EXE").write_bytes(b"new")
    changed = _apply_overlay(patch, base)
    assert changed == ["Wiz8.exe"]
    assert (base / "Wiz8.exe").read_bytes() == b"new"
    assert not (base / "Wiz8.EXE").exists()
