from __future__ import annotations

from pathlib import Path

from wiz8decomp.paths import tree_hash, tree_manifest


def test_tree_manifest_is_sorted_and_mtime_independent(tmp_path: Path) -> None:
    (tmp_path / "z.txt").write_text("z")
    (tmp_path / "a.txt").write_text("a")
    first = tree_manifest(tmp_path)
    (tmp_path / "a.txt").touch()
    second = tree_manifest(tmp_path)
    assert [item["path"] for item in first] == ["a.txt", "z.txt"]
    assert first == second
    assert tree_hash(first) == tree_hash(second)

