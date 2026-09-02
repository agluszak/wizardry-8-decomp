from __future__ import annotations

import json

from wiz8decomp import command_support
from wiz8decomp.subprocesses import actionable_diagnostics


def test_human_result_hides_complete_subprocess_payload(capsys) -> None:
    command_support.set_json_output(False)
    command_support.emit(
        command_support.human(
            "WIZ8 built successfully\nlog: build/logs/product-build.json", {"stdout": "huge"}
        )
    )
    output = capsys.readouterr().out
    assert "built successfully" in output
    assert "huge" not in output


def test_json_result_contains_complete_structured_payload(capsys) -> None:
    command_support.set_json_output(True)
    command_support.emit(command_support.human("short", {"context": {"calls": [1]}}))
    assert json.loads(capsys.readouterr().out) == {"context": {"calls": [1]}}
    command_support.set_json_output(False)


def test_failed_build_diagnostics_strip_jom_unwinding() -> None:
    output = "jom: stopping\nunit.cpp(3): error C2065: missing\njom: Error 2\n"
    assert actionable_diagnostics(output, "") == ["unit.cpp(3): error C2065: missing"]
