"""Header-includer discovery tolerates sources the change deletes."""

from __future__ import annotations

from pathlib import Path

from wiz8decomp.commands.reports import _includes_directly


def test_includes_directly_matches_the_include_line(tmp_path: Path) -> None:
    source = tmp_path / "unit.cpp"
    source.write_text('#include "surrender/srMath.h"\n', encoding="utf-8")
    assert _includes_directly(source, "include/surrender/srMath.h")
    assert not _includes_directly(source, "include/surrender/srArray.h")


def test_deleted_source_includes_nothing(tmp_path: Path) -> None:
    assert not _includes_directly(tmp_path / "gone.cpp", "include/surrender/srMath.h")
