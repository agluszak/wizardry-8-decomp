from __future__ import annotations

import zipfile
from pathlib import Path

import pytest
from wiz8decomp.ghidra.fid_seeds import _safe_extract_zip


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
