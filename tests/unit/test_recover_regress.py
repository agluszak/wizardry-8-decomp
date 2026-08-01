"""Unit tests for the recover-regress splicing logic and CLI shape.

The harness itself needs the live project and the VC6 toolchain; these tests
cover the pure text machinery: export-block splitting, marker span selection,
and line splicing with restoration semantics.
"""

from __future__ import annotations

import pytest
from typer.testing import CliRunner
from wiz8decomp import command_support
from wiz8decomp.cli import app
from wiz8decomp.recover import (
    compile_diagnostics,
    constructor_member_names,
    constructor_store_alternative,
    insert_lines,
    marker_span,
    mismatch_alternatives,
    place_address,
    splice_lines,
    split_export_blocks,
    suggest_includes,
)

EXPORT_TEXT = """\
// FUNCTION: WIZ8 0x004a5e50

W8GrCycle::W8GrCycle()

{
  m_plsLights = 0;
}

// SYNTHETIC: WIZ8 0x004a5f00
// W8GrCycle::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004a6e20
void W8GrCycle::TickAnimation(float scale)
{
}
"""


def test_split_export_blocks_keys_blocks_by_address() -> None:
    blocks = split_export_blocks(EXPORT_TEXT)
    assert sorted(blocks) == [0x4A5E50, 0x4A5F00, 0x4A6E20]
    assert blocks[0x4A5E50].startswith("// FUNCTION: WIZ8 0x004a5e50\n")
    assert blocks[0x4A5E50].endswith("}\n")
    assert "scalar deleting destructor" in blocks[0x4A5F00]
    assert blocks[0x4A6E20].endswith("{\n}\n")


def test_split_export_blocks_drops_trailing_blank_lines() -> None:
    blocks = split_export_blocks("// FUNCTION: WIZ8 0x00000010\nbody\n\n\n")
    assert blocks[0x10] == "// FUNCTION: WIZ8 0x00000010\nbody\n"


def test_marker_span_covers_marker_line_through_end_line() -> None:
    marker = {
        "address": 0x4A6E20,
        "line": 348,
        "source_file": "src/wiz8/engine_code/GrCycle.cpp",
        "declaration": {"end_line": 413},
    }
    assert marker_span(marker) == ("src/wiz8/engine_code/GrCycle.cpp", 347, 413)


def test_marker_span_declines_without_a_declaration_end() -> None:
    assert marker_span({"line": 10, "source_file": "a.cpp", "declaration": {}}) is None
    assert marker_span({"line": 10, "source_file": "a.cpp", "declaration": {"end_line": 9}}) is None


def test_splice_lines_replaces_the_span_and_round_trips() -> None:
    original = "a\nb\nc\nd\ne\n"
    spliced = splice_lines(original, 2, 4, "X\nY")
    assert spliced == "a\nX\nY\ne\n"
    with pytest.raises(ValueError):
        splice_lines(original, 0, 2, "X")
    with pytest.raises(ValueError):
        splice_lines(original, 4, 99, "X")


def test_compile_diagnostics_extracts_compiler_errors() -> None:
    output = (
        "cl something\n"
        "GrCycle.cpp(363) : error C2065: 'code' : undeclared identifier\n"
        "GrCycle.cpp(363) : error C2065: 'code' : undeclared identifier\n"
        "jom: Makefile [WIZ8] Error 2\n"
    )
    assert compile_diagnostics(output) == [
        "GrCycle.cpp(363) : error C2065: 'code' : undeclared identifier"
    ]
    assert compile_diagnostics("no compiler lines at all") == ["no compiler lines at all"]


_MARKERS = [
    {
        "address": 0x100,
        "marker_kind": "FUNCTION",
        "source_file": "src/a.cpp",
        "line": 10,
        "declaration": {"end_line": 20},
    },
    {
        "address": 0x200,
        "marker_kind": "SYNTHETIC",
        "source_file": "src/a.cpp",
        "line": 30,
        "declaration": None,
    },
    {
        "address": 0x300,
        "marker_kind": "FUNCTION",
        "source_file": "src/b.cpp",
        "line": 5,
        "declaration": {"end_line": 9},
    },
]


def test_place_address_chooses_the_shared_file_after_the_earlier_block() -> None:
    placement = place_address(_MARKERS, 0x180)
    assert placement == {
        "status": "placed",
        "source_file": "src/a.cpp",
        "after_line": 20,
    }


def test_place_address_refuses_between_translation_units_and_outside_ranges() -> None:
    between = place_address(_MARKERS, 0x280)
    assert between["status"] == "unplaced"
    assert "src/a.cpp" in between["reason"] and "src/b.cpp" in between["reason"]
    assert place_address(_MARKERS, 0x50)["status"] == "unplaced"
    assert place_address(_MARKERS, 0x400)["status"] == "unplaced"


def test_place_address_counts_the_synthetic_symbol_comment_line() -> None:
    placement = place_address(_MARKERS, 0x250)
    assert placement["status"] == "unplaced"  # between units
    inside = place_address(
        [_MARKERS[0], _MARKERS[1], {**_MARKERS[2], "source_file": "src/a.cpp"}], 0x250
    )
    assert inside == {"status": "placed", "source_file": "src/a.cpp", "after_line": 31}


def test_insert_lines_adds_a_separating_blank_line() -> None:
    original = "a\nb\nc\n"
    assert insert_lines(original, 2, "X\n") == "a\nb\n\nX\nc\n"
    with pytest.raises(ValueError):
        insert_lines(original, 99, "X\n")


def test_constructor_store_alternative_moves_member_initializers() -> None:
    block = (
        "// FUNCTION: WIZ8 0x004aded0\n"
        "W8CameraShakeEffect::W8CameraShakeEffect(float duration, char preset)\n"
        "    : W8Base(other), flags_00(0),\n"
        "      timer_18(duration, 0), value_48(0)\n"
        "\n"
        "{\n"
        "  cycle_3c = 0;\n"
        "}\n"
    )
    alternative = constructor_store_alternative(
        block, member_names={"flags_00", "timer_18", "value_48"}
    )
    assert alternative is not None
    assert "    : W8Base(other), timer_18(duration, 0)" in alternative
    assert "  flags_00 = 0;\n  value_48 = 0;\n  cycle_3c = 0;" in alternative


def test_constructor_store_alternative_declines_without_movable_members() -> None:
    assert (
        constructor_store_alternative(
            "// FUNCTION: WIZ8 0x1\nvoid f()\n{\n}\n", member_names={"value"}
        )
        is None
    )
    base_only = "X::X()\n    : W8Base(other)\n\n{\n}\n"
    assert constructor_store_alternative(base_only, member_names={"value"}) is None


def test_constructor_store_alternative_never_guesses_lowercase_bases() -> None:
    block = "X::X()\n    : srNode(0), value(1)\n\n{\n}\n"
    alternative = constructor_store_alternative(block, member_names={"value"})
    assert alternative is not None
    assert "    : srNode(0)" in alternative
    assert "  value = 1;" in alternative
    assert "srNode =" not in alternative


def test_constructor_member_names_uses_the_source_class_inventory() -> None:
    classes = [
        {
            "qualified_name": "stSurface2D",
            "fields": [{"name": "state"}, {"name": "source_surface"}],
        }
    ]
    block = "stSurface2D::stSurface2D()\n    : srNode(0), state(1)\n\n{\n}\n"
    assert constructor_member_names(block, classes) == {"state", "source_surface"}


def test_return_width_candidates_are_bounded_and_structurally_driven() -> None:
    block = "// FUNCTION: WIZ8 0x00000001\nint f(int value)\n{\n  return value;\n}\n"
    finding = {"difference": {"kind": "return_value"}}
    alternatives = mismatch_alternatives(block, finding)
    assert [name for name, _ in alternatives] == ["return-unsigned-char", "return-unsigned-int"]
    assert alternatives[0][1].startswith("// FUNCTION: WIZ8 0x00000001\nunsigned char f(int value)")
    assert mismatch_alternatives(block, {"difference": {"kind": "branch_condition"}}) == []


def test_suggest_includes_names_declaring_headers(tmp_path) -> None:
    header = tmp_path / "include" / "wiz8" / "chunk.h"
    header.parent.mkdir(parents=True)
    header.write_text("#pragma once\nclass W8Chunk {\n};\n", encoding="utf-8")
    diagnostics = [
        "GrCycle.cpp(10) : error C2065: 'W8Chunk' : undeclared identifier",
        "GrCycle.cpp(11) : error C2065: 'SBORROW4' : undeclared identifier",
    ]
    suggestions = suggest_includes(tmp_path, diagnostics)
    assert suggestions == {"W8Chunk": ["include/wiz8/chunk.h"]}


def test_recover_function_command_previews(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(command_support, "settings", lambda: object())

    def fake_recover(settings, address, *, apply, target, program_selector):
        assert address == "0x004a6970"
        assert apply is False
        return {"address": "0x004a6970", "status": "previewed", "chosen": "as-exported"}

    monkeypatch.setattr("wiz8decomp.recover.recover_function", fake_recover)
    result = CliRunner().invoke(app, ["recover", "function", "0x004a6970"])
    assert result.exit_code == 0
    assert "previewed" in result.stdout


def test_regress_command_reports_the_summary(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(command_support, "settings", lambda: object())

    def fake_regress(settings, selections, *, target, program_selector):
        assert selections == ["0x004a5e50"]
        assert target == "WIZ8"
        assert program_selector == "wiz8"
        return {"selected": 1, "exact": 1, "effective": 0, "compile_failed": 0}

    monkeypatch.setattr("wiz8decomp.recover.regress", fake_regress)
    result = CliRunner().invoke(app, ["recover", "regress", "0x004a5e50"])
    assert result.exit_code == 0
    assert "exact" in result.stdout
