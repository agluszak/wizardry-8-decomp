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
    block_end_line,
    compile_diagnostics,
    exported_blocks,
    exported_declines,
    graft_source_signature,
    insert_lines,
    marker_span,
    place_address,
    project_source_forms,
    recover_candidates,
    resolve_source_placement,
    splice_lines,
    suggest_includes,
    verify_marker_adjacency,
)


def test_recover_candidates_preserves_source_and_writes_each_candidate(
    tmp_path, monkeypatch: pytest.MonkeyPatch
) -> None:
    from types import SimpleNamespace

    from wiz8decomp import recover

    source = tmp_path / "unit.cpp"
    source.write_text("int existing;\n", encoding="utf-8")
    settings = SimpleNamespace(repo_dir=tmp_path, build_dir=tmp_path / "build")
    monkeypatch.setattr(
        "wiz8decomp.source_index.load_source_index",
        lambda *_args: {"markers": []},
    )
    monkeypatch.setattr(
        "wiz8decomp.ghidra.recovery.recover_functions",
        lambda *_args, **_kwargs: {
            "exports": [
                {
                    "entry": "0x00401000",
                    "generated_code": "void candidate() {}\n",
                    "recovery": {"passes": [], "defects": []},
                }
            ]
        },
    )
    monkeypatch.setattr(
        recover,
        "resolve_source_placement",
        lambda *_args: {"status": "placed", "source_file": "unit.cpp", "after_line": 1},
    )

    result = recover_candidates(settings, ["0x00401000"])

    assert source.read_text(encoding="utf-8") == "int existing;\n"
    assert result["functions"][0]["candidate"] == "build/recover/candidates/00401000.cpp"
    assert (tmp_path / result["functions"][0]["candidate"]).read_text(encoding="utf-8") == (
        "void candidate() {}\n"
    )


def test_exported_blocks_uses_structured_per_entry_results() -> None:
    blocks = exported_blocks(
        {
            "exports": [
                {"entry": "0x004a5e50", "generated_code": "constructor\n"},
                {"entry": "0x004a5f00", "generated_code": "destructor\n"},
                {"entry": "0x004a6e20", "generated_code": "method\n"},
            ]
        }
    )
    assert sorted(blocks) == [0x4A5E50, 0x4A5F00, 0x4A6E20]
    assert blocks[0x4A5E50] == "constructor\n"
    assert blocks[0x4A5F00] == "destructor\n"
    assert blocks[0x4A6E20] == "method\n"


def test_exported_blocks_does_not_parse_or_normalize_cpp_text() -> None:
    text = "body\n\n\n"
    assert exported_blocks({"exports": [{"entry": "0x10", "generated_code": text}]}) == {0x10: text}


def test_exported_declines_blocks_unresolved_formal_prototypes() -> None:
    result = {
        "exports": [
            {
                "entry": "0x0046a490",
                "recovery": {
                    "passes": [
                        {
                            "status": "declined",
                            "pass": "signature.prototype",
                            "detail": "formal signature contains an unresolved ABI type",
                        },
                        {"status": "declined", "pass": "other", "detail": "not a blocker"},
                    ]
                },
            }
        ]
    }
    assert exported_declines(result) == {
        0x46A490: [
            {
                "pass": "signature.prototype",
                "detail": "formal signature contains an unresolved ABI type",
            }
        ]
    }


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


def test_marker_adjacency_accepts_canonical_uppercase_address_digits() -> None:
    assert verify_marker_adjacency("// FUNCTION: WIZ8 0x004B7BA0\nvoid body();\n", 1, 0x004B7BA0)


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


def test_synthetic_insertion_never_splits_the_following_destructor() -> None:
    marker = {
        "address": 0x44F3D0,
        "marker_kind": "SYNTHETIC",
        "source_file": "src/a.cpp",
        "line": 2,
        # The source index may bind the following declaration's signature to
        # the emission marker. It must not become this marker's insertion end.
        "declaration": {"end_line": 4},
    }
    assert block_end_line(marker) == 3
    original = (
        "before\n// SYNTHETIC: WIZ8 0x0044F3D0\n// C::`scalar deleting destructor'\nC::~C()\n{\n}\n"
    )
    inserted = insert_lines(original, block_end_line(marker), "void recovered() {}\n")
    assert "void recovered() {}\nC::~C()\n{\n}" in inserted


def test_insert_lines_adds_a_separating_blank_line() -> None:
    original = "a\nb\nc\n"
    assert insert_lines(original, 2, "X\n") == "a\nb\n\nX\nc\n"
    with pytest.raises(ValueError):
        insert_lines(original, 99, "X\n")


def test_graft_keeps_the_source_declaration_and_takes_the_exported_body() -> None:
    marker = {
        "address": 0x4A6E20,
        "declaration": {
            "source_signature": (
                "void W8GrCycle::TickAnimation(const W8Object* const* objects, "
                "const W8Timer& timer)"
            )
        },
    }
    grafted = graft_source_signature(marker, "{\n  new_body();\n}\n")
    assert grafted is not None
    assert "const W8Object* const* objects" in grafted
    assert "const W8Timer& timer" in grafted
    assert "__thiscall" not in grafted
    assert "new_body();" in grafted


def test_graft_takes_the_exported_initializer_list() -> None:
    marker = {
        "address": 0x4AE000,
        "declaration": {"source_signature": "W8Effect::W8Effect(const W8Effect& other)"},
    }
    grafted = graft_source_signature(marker, "    : new_init(param_1->a)\n{\n  new_body();\n}\n")
    assert grafted is not None
    assert "const W8Effect& other" in grafted
    assert "new_init(param_1->a)" in grafted


def test_graft_declines_without_compiler_indexed_signature() -> None:
    assert graft_source_signature({"address": 1, "declaration": {}}, "{\n}\n") is None


def test_source_forms_project_unique_address_names_and_authored_casts() -> None:
    generated = (
        "{\n"
        "  short sVar1;\n"
        "  span = value * _DAT_005ec344;\n"
        "  sVar1 = ftol();\n"
        "  cell_count_024 = sVar1 + 1;\n"
        "}\n"
    )
    source_file = "extern float g_path_span_scale_005ec344;\n"
    source_block = "cell_count_024 = (short)(int)span_020 + 1;\n"
    projected, facts, blockers = project_source_forms(generated, source_file, source_block)
    assert "g_path_span_scale_005ec344" in projected
    assert "ftol" not in projected
    assert "sVar1" not in projected
    assert "cell_count_024 = (short)(int)span_020 + 1;" in projected
    assert {fact["kind"] for fact in facts} == {"identifier", "source-cast"}
    assert blockers == []


def test_source_forms_decline_ambiguous_or_unowned_spellings() -> None:
    generated = "value = _DAT_005ec344;\nresult = ftol();\n"
    source = "float first_005ec344; float second_005ec344;\n"
    projected, facts, blockers = project_source_forms(generated, source)
    assert projected == generated
    assert facts == []
    assert blockers == ["005ec344", "ftol"]


def test_recovery_placement_uses_assertion_backed_unit_owner(tmp_path) -> None:
    assertion_path = tmp_path / "evidence/observations/wiz8/assertions.csv"
    assertion_path.parent.mkdir(parents=True)
    assertion_path.write_text(
        "source_path,containing_function\n"
        "C:\\Projects\\Wizardry 8\\Engine Code\\stCube.cpp,0048d080\n"
        "C:\\Projects\\Wizardry 8\\Engine Code\\stCube.cpp,0048e7b0\n",
        encoding="utf-8",
    )
    placement = resolve_source_placement(tmp_path, [], 0x48DCA0)
    assert placement["status"] == "unplaced"
    assert placement["source_path"] == "Engine Code\\stCube.cpp"
    assert placement["attribution"] == "interval-inference"
    assert "no recovered physical source file" in placement["reason"]


def test_suggest_includes_names_declaring_headers(tmp_path) -> None:
    import json

    index = tmp_path / "build" / "source-index.json"
    index.parent.mkdir(parents=True)
    index.write_text(
        json.dumps(
            {
                "classes": [
                    {
                        "qualified_name": "W8Chunk",
                        "source_file": "include/wiz8/chunk.h",
                    }
                ],
                "declarations": [],
            }
        ),
        encoding="utf-8",
    )
    diagnostics = [
        "GrCycle.cpp(10) : error C2065: 'W8Chunk' : undeclared identifier",
        "GrCycle.cpp(11) : error C2065: 'SBORROW4' : undeclared identifier",
    ]
    suggestions = suggest_includes(tmp_path, diagnostics)
    assert suggestions == {"W8Chunk": ["include/wiz8/chunk.h"]}


def test_recover_function_command_generates_a_batch(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(command_support, "settings", lambda: object())

    def fake_recover(settings, selectors, *, program_selector):
        assert selectors == ["0x004a6970", "0x004a6980"]
        return {"schema": "wiz8.recovery-candidates", "functions": []}

    monkeypatch.setattr("wiz8decomp.recover.recover_candidates", fake_recover)
    result = CliRunner().invoke(app, ["recover", "function", "0x004a6970", "0x004a6980"])
    assert result.exit_code == 0
    assert "wiz8.recovery-candidates" in result.stdout


def test_verify_marker_adjacency_proves_the_span_start() -> None:
    from wiz8decomp.recover import verify_marker_adjacency

    original = "prose\n// FUNCTION: WIZ8 0x004a5e50\nvoid f()\n{\n}\n"
    assert verify_marker_adjacency(original, 2, 0x4A5E50)
    assert not verify_marker_adjacency(original, 1, 0x4A5E50)
    assert not verify_marker_adjacency(original, 99, 0x4A5E50)


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
