from pathlib import Path

import pytest
from wiz8decomp.markers import (
    MarkerHygieneError,
    check_marker_hygiene,
    following_declaration_offset,
)

REPOSITORY = Path(__file__).resolve().parents[2]


def _source(tmp_path: Path, text: str) -> Path:
    path = tmp_path / "unit.cpp"
    path.write_text(text, encoding="utf-8")
    return path


def test_a_marker_against_its_declaration_passes(tmp_path: Path) -> None:
    _source(
        tmp_path,
        "/* prose belongs here */\n// FUNCTION: WIZ8 0x00401000\nvoid f(void)\n{\n}\n",
    )
    result = check_marker_hygiene([tmp_path], tmp_path)

    assert result["function_markers"] == 1
    assert result["function_addresses"] == 1


def test_a_comment_between_marker_and_declaration_is_refused(tmp_path: Path) -> None:
    _source(
        tmp_path,
        "// FUNCTION: WIZ8 0x00401000\n/* explanation */\nvoid f(void)\n{\n}\n",
    )
    with pytest.raises(MarkerHygieneError, match="separated from its declaration by a comment"):
        check_marker_hygiene([tmp_path], tmp_path)


def test_a_blank_line_between_marker_and_declaration_is_refused(tmp_path: Path) -> None:
    _source(tmp_path, "// FUNCTION: WIZ8 0x00401000\n\nvoid f(void)\n{\n}\n")
    with pytest.raises(MarkerHygieneError, match="blank line"):
        check_marker_hygiene([tmp_path], tmp_path)


def test_a_block_comment_is_skipped_whole_rather_than_line_by_line(tmp_path: Path) -> None:
    # The continuation line starts with a letter, not an asterisk. Testing lines
    # individually reads it as the declaration and reports the marker as clean,
    # and any fix acting on that reading splices the marker into the comment.
    text = (
        "// FUNCTION: WIZ8 0x00401000\n"
        "/* first line\n"
        "   never see a live entry, and this continuation starts with a letter */\n"
        "void f(void)\n{\n}\n"
    )
    lines = text.splitlines(True)
    offset, separator = following_declaration_offset(lines, 0)

    assert lines[offset].startswith("void f(void)")
    assert separator == "comment"
    with pytest.raises(MarkerHygieneError, match="comment"):
        check_marker_hygiene([_source(tmp_path, text).parent], tmp_path)


def test_one_address_may_own_only_one_marker_within_a_module(tmp_path: Path) -> None:
    (tmp_path / "a.cpp").write_text("// FUNCTION: WIZ8 0x00401000\nvoid a(void) {}\n")
    (tmp_path / "b.cpp").write_text("// FUNCTION: WIZ8 0x401000\nvoid b(void) {}\n")

    with pytest.raises(MarkerHygieneError, match="claimed by 2 FUNCTION markers"):
        check_marker_hygiene([tmp_path], tmp_path)


def test_the_same_offset_in_two_modules_is_not_a_duplicate(tmp_path: Path) -> None:
    # WIZ8 and the srEXT DLLs are separate images and legitimately reuse
    # offsets, unlike the single-module tree this check was ported from.
    (tmp_path / "a.cpp").write_text("// FUNCTION: SREXT_UNZIP 0x10001000\nvoid a(void) {}\n")
    (tmp_path / "b.cpp").write_text("// FUNCTION: SREXT_JPEGIMPORTER 0x10001000\nvoid b(void) {}\n")

    assert check_marker_hygiene([tmp_path], tmp_path)["function_addresses"] == 2


def test_library_markers_may_be_followed_by_the_symbol_name(tmp_path: Path) -> None:
    # They name linked code with no owned definition to sit against, which is
    # the established convention in vc6_runtime.cpp.
    _source(tmp_path, "// LIBRARY: WIZ8 0x005E1C10\n// operator delete\n")

    assert check_marker_hygiene([tmp_path], tmp_path)["library_markers"] == 1


def test_the_tree_itself_is_clean() -> None:
    result = check_marker_hygiene(
        [REPOSITORY / "src", REPOSITORY / "include"], REPOSITORY
    )

    assert result["function_markers"] == result["function_addresses"]
    assert result["function_markers"] > 100
