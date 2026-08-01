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
    marker_span,
    splice_lines,
    split_export_blocks,
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
