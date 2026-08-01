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
    graft_source_signature,
    insert_lines,
    marker_span,
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


def test_graft_keeps_the_source_declaration_and_takes_the_exported_body() -> None:
    source = (
        "// FUNCTION: WIZ8 0x004a6e20\n"
        "/* prose the source owns */\n"
        "void W8GrCycle::TickAnimation(const W8Timer& timer)\n"
        "{\n"
        "    old_body();\n"
        "}\n"
    )
    exported = (
        "// FUNCTION: WIZ8 0x004a6e20\n"
        "void __thiscall W8GrCycle::TickAnimation(W8GrCycle *this, W8Timer *param_1)\n"
        "{\n"
        "  new_body();\n"
        "}\n"
    )
    grafted = graft_source_signature(source, exported)
    assert "const W8Timer& timer" in grafted
    assert "__thiscall" not in grafted
    assert "new_body();" in grafted
    assert "old_body();" not in grafted


def test_graft_takes_the_exported_initializer_list() -> None:
    source = (
        "// FUNCTION: WIZ8 0x004ae000\n"
        "W8Effect::W8Effect(const W8Effect& other)\n"
        "    : old_init(other.a),\n"
        "      old_more(other.b)\n"
        "{\n"
        "    old_body();\n"
        "}\n"
    )
    exported = (
        "// FUNCTION: WIZ8 0x004ae000\n"
        "W8Effect::W8Effect(W8Effect *param_1)\n"
        "    : new_init(param_1->a)\n"
        "{\n"
        "  new_body();\n"
        "}\n"
    )
    grafted = graft_source_signature(source, exported)
    assert "const W8Effect& other" in grafted
    assert "new_init(param_1->a)" in grafted
    assert "old_init" not in grafted
    assert "old_more" not in grafted


def test_graft_falls_back_when_a_block_lacks_the_body_brace() -> None:
    exported = "// FUNCTION: WIZ8 0x1\nvoid f()\n{\n}\n"
    assert graft_source_signature("// FUNCTION: WIZ8 0x1\nno brace here\n", exported) == exported
    one_line = "// FUNCTION: WIZ8 0x1\nvoid f() { body(); }\n"
    assert graft_source_signature("// FUNCTION: WIZ8 0x1\nvoid f()\n{\n}\n", one_line) == one_line


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


def test_verify_marker_adjacency_proves_the_span_start() -> None:
    from wiz8decomp.recover import verify_marker_adjacency

    original = "prose\n// FUNCTION: WIZ8 0x004a5e50\nvoid f()\n{\n}\n"
    assert verify_marker_adjacency(original, 2, 0x4A5E50)
    assert not verify_marker_adjacency(original, 1, 0x4A5E50)
    assert not verify_marker_adjacency(original, 99, 0x4A5E50)


def test_exporter_defects_extracts_flagged_lines() -> None:
    from wiz8decomp.recover import exporter_defects

    block = (
        "// exporter-defect: call.virtual: java.lang.NullPointerException\n"
        "// FUNCTION: WIZ8 0x00000010\nvoid f()\n{\n}\n"
    )
    assert exporter_defects(block) == ["call.virtual: java.lang.NullPointerException"]
    assert exporter_defects("// FUNCTION: WIZ8 0x00000010\nvoid f()\n{\n}\n") == []


def test_candidate_rank_prefers_status_then_divergence_position() -> None:
    from wiz8decomp.recover import _candidate_rank

    exact = {"status": "exact", "raw_matching": 0.5}
    late = {
        "status": "mismatch",
        "raw_matching": 0.7,
        "first_divergence": {"difference": {"orig": {"instruction_index": 40}}},
    }
    early_high_raw = {
        "status": "mismatch",
        "raw_matching": 0.99,
        "first_divergence": {"difference": {"orig": {"instruction_index": 3}}},
    }
    ranked = sorted([early_high_raw, late, exact], key=_candidate_rank, reverse=True)
    assert ranked[0] is exact
    assert ranked[1] is late  # later first divergence beats higher raw score


def test_splice_unit_applies_many_spans_and_reports_ranges() -> None:
    from wiz8decomp.recover import splice_unit

    original = "l1\nl2\nl3\nl4\nl5\nl6\n"
    text, ranges = splice_unit(
        original,
        [(2, 3, 0x10, "A1\nA2\nA3\n"), (5, 5, 0x20, "B1\n")],
    )
    assert text == "l1\nA1\nA2\nA3\nl4\nB1\nl6\n"
    assert ranges == {0x10: (2, 4), 0x20: (6, 6)}
    import pytest as _pytest

    with _pytest.raises(ValueError):
        splice_unit(original, [(2, 4, 0x10, "X\n"), (3, 5, 0x20, "Y\n")])


def test_attribute_diagnostics_maps_lines_to_blocks() -> None:
    from wiz8decomp.recover import attribute_diagnostics

    ranges = {"src/wiz8/engine_code/GrCycle.cpp": {0x10: (100, 120), 0x20: (200, 210)}}
    per_address, unattributed = attribute_diagnostics(
        [
            "Z:\\repo\\src\\wiz8\\engine_code\\GrCycle.cpp(105) : error C2065: 'x'",
            "Z:\\repo\\src\\wiz8\\engine_code\\GrCycle.cpp(300) : error C2065: 'y'",
            "no location at all",
        ],
        ranges,
    )
    assert set(per_address) == {0x10}
    assert len(per_address[0x10]) == 1
    assert len(unattributed) == 2


def test_categorize_failure_orders_by_evidence_strength() -> None:
    from wiz8decomp.recover import categorize_failure

    assert (
        categorize_failure(["a.cpp(1) : error C2065: 'SBORROW4' : undeclared identifier"])
        == "unsupported-intrinsic"
    )
    assert (
        categorize_failure(["a.cpp(1) : error C2065: 'DAT_0065be2c' : undeclared identifier"])
        == "unresolved-identity"
    )
    assert (
        categorize_failure(["a.cpp(1) : error C2511: 'W8GrCycle::W8GrCycle' : overloaded member"])
        == "declaration-or-type"
    )
    assert categorize_failure(["a.cpp(1) : fatal error C1004: unexpected"]) == (
        "other-compile-failure"
    )
