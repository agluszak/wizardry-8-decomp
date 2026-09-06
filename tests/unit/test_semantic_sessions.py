"""Per-function decompiler result reuse inside one query batch."""

from __future__ import annotations

import sys
from types import ModuleType, SimpleNamespace

from wiz8decomp.ghidra import semantic


class _Program:
    def getUniqueProgramID(self) -> int:
        return 7

    def getDomainFile(self):
        raise RuntimeError("transient")

    def getName(self) -> str:
        return "fixture"


class _Function:
    def getEntryPoint(self) -> str:
        return "00401000"


class _Result:
    def __init__(self, high: object) -> None:
        self._high = high

    def getHighFunction(self) -> object:
        return self._high

    def getErrorMessage(self) -> str:
        return ""


class _Interface:
    def __init__(self) -> None:
        self.calls = 0

    def decompileFunction(self, _function, _timeout, _monitor) -> _Result:
        self.calls += 1
        return _Result(object())


def test_high_function_is_decompiled_once_per_function_and_style(monkeypatch) -> None:
    ghidra = ModuleType("ghidra")
    util = ModuleType("ghidra.util")
    task = ModuleType("ghidra.util.task")
    task.TaskMonitor = SimpleNamespace(DUMMY=object())
    monkeypatch.setitem(sys.modules, "ghidra", ghidra)
    monkeypatch.setitem(sys.modules, "ghidra.util", util)
    monkeypatch.setitem(sys.modules, "ghidra.util.task", task)

    interface = _Interface()
    monkeypatch.setattr(semantic, "_session", lambda *_args, **_kwargs: interface)
    semantic._results.clear()
    try:
        program = _Program()
        function = _Function()

        first = semantic._high_function(program, function, "normalize")
        second = semantic._high_function(program, function, "normalize")

        assert first is second
        assert interface.calls == 1
    finally:
        semantic._results.clear()
