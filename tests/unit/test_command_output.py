from __future__ import annotations

import json

import pytest
import typer
from wiz8decomp import command_support
from wiz8decomp.subprocesses import actionable_diagnostics


def test_structured_result_is_the_only_output(capsys) -> None:
    command_support.emit({"context": {"calls": [1]}})
    assert json.loads(capsys.readouterr().out) == {"context": {"calls": [1]}}


def test_failure_uses_the_same_structured_boundary(capsys) -> None:
    def fail() -> None:
        raise ValueError("bad selector")

    with pytest.raises(typer.Exit):
        command_support.run_action(fail)
    assert json.loads(capsys.readouterr().out) == {
        "ok": False,
        "error": {"type": "ValueError", "message": "bad selector"},
    }


def test_failed_build_diagnostics_strip_jom_unwinding() -> None:
    output = "jom: stopping\nunit.cpp(3): error C2065: missing\njom: Error 2\n"
    assert actionable_diagnostics(output, "") == ["unit.cpp(3): error C2065: missing"]
