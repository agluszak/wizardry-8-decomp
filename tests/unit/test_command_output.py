from __future__ import annotations

import json

from wiz8decomp import command_support
from wiz8decomp.subprocesses import actionable_diagnostics


def test_structured_result_is_the_only_output(capsys) -> None:
    command_support.emit({"context": {"calls": [1]}})
    assert json.loads(capsys.readouterr().out) == {"context": {"calls": [1]}}


def test_failed_build_diagnostics_strip_jom_unwinding() -> None:
    output = "jom: stopping\nunit.cpp(3): error C2065: missing\njom: Error 2\n"
    assert actionable_diagnostics(output, "") == ["unit.cpp(3): error C2065: missing"]


def test_link_failure_keeps_the_unresolved_symbol() -> None:
    symbol = "sr.exp : error LNK2001: unresolved external symbol Quantizer copy constructor"
    summary = "sr.dll : fatal error LNK1120: 1 unresolved externals"
    output = f"{symbol}\n{summary}\njom: Error 1120\n"
    assert actionable_diagnostics(output, "") == [symbol, summary]
