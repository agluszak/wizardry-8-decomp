from pathlib import Path

import pytest
from wiz8decomp.reccmp_lint import ReccmpLintError, validate_reccmp_annotations


def test_decomplint_covers_every_configured_source_target(tmp_path: Path) -> None:
    (tmp_path / "reccmp-project.yml").write_text(
        """targets:
  WIZ8:
    filename: Wiz8.exe
    source-root: src/wiz8
    hash:
      sha256: "0000000000000000000000000000000000000000000000000000000000000000"
  SURRENDER:
    filename: sr.dll
    source-root:
      - src/surrender
      - include/surrender
    hash:
      sha256: "1111111111111111111111111111111111111111111111111111111111111111"
""",
        encoding="utf-8",
    )
    wiz8 = tmp_path / "src/wiz8"
    surrender = tmp_path / "src/surrender"
    surrender_include = tmp_path / "include/surrender"
    wiz8.mkdir(parents=True)
    surrender.mkdir(parents=True)
    surrender_include.mkdir(parents=True)
    (wiz8 / "game.cpp").write_text(
        "// FUNCTION: WIZ8 0x00401000\nvoid GameFunction() {}\n", encoding="utf-8"
    )
    (surrender / "core.cpp").write_text(
        "// FUNCTION: SURRENDER 0x10001000\nvoid RenderFunction() {}\n", encoding="utf-8"
    )

    result = validate_reccmp_annotations(tmp_path)

    assert set(result["targets"]) == {"WIZ8", "SURRENDER"}


def test_folded_source_marker_is_rejected(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "// FUNCTION: WIZ8 0x00401000 FOLDED\nvoid Example() {}\n",
        encoding="utf-8",
    )

    with pytest.raises(ReccmpLintError, match="FOLDED source markers are forbidden"):
        validate_reccmp_annotations(tmp_path)


def test_identity_alias_source_annotation_is_rejected(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "// identity-alias: old fold escape hatch\nvoid Example() {}\n",
        encoding="utf-8",
    )

    with pytest.raises(ReccmpLintError, match="identity-alias source annotations are forbidden"):
        validate_reccmp_annotations(tmp_path)
