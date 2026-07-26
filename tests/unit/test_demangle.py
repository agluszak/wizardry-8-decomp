from __future__ import annotations

import shutil
import subprocess
from typing import Any

import pytest
from wiz8decomp.binary import demangle as demangle_module
from wiz8decomp.binary.demangle import DemanglerMissing, demangle

_HAS_TOOL = shutil.which("llvm-undname") is not None


def _fake_run(stdout: str) -> Any:
    def run(*_args: Any, **_kwargs: Any) -> Any:
        return subprocess.CompletedProcess(args=[], returncode=1, stdout=stdout, stderr="")

    return run


def test_failed_names_use_a_two_line_group_and_do_not_desynchronise(monkeypatch: Any) -> None:
    """A rejected name emits no signature line, so the stride is not constant.

    Assuming three lines per name attaches each later signature to the wrong
    symbol, which is silent and wrong rather than loud and wrong.
    """
    monkeypatch.setattr(demangle_module, "tool_path", lambda: "llvm-undname")
    monkeypatch.setattr(
        demangle_module.subprocess,
        "run",
        _fake_run("?a@C@@QAEXXZ\npublic: void __thiscall C::a(void)\n\n?bad@@\n\n?z@D@@QAEXXZ\npublic: void __thiscall D::z(void)\n\n"),
    )

    result = demangle(["?a@C@@QAEXXZ", "?bad@@", "?z@D@@QAEXXZ"])

    assert result["?a@C@@QAEXXZ"] == "public: void __thiscall C::a(void)"
    assert result["?bad@@"] == ""
    assert result["?z@D@@QAEXXZ"] == "public: void __thiscall D::z(void)"


def test_unexpected_echo_is_an_error_rather_than_a_guessed_alignment(monkeypatch: Any) -> None:
    monkeypatch.setattr(demangle_module, "tool_path", lambda: "llvm-undname")
    monkeypatch.setattr(demangle_module.subprocess, "run", _fake_run("?other@@\nsomething\n\n"))

    with pytest.raises(RuntimeError, match="did not echo"):
        demangle(["?a@C@@QAEXXZ"])


def test_undecorated_names_are_never_sent(monkeypatch: Any) -> None:
    captured: dict[str, str] = {}

    def run(*_args: Any, **kwargs: Any) -> Any:
        captured["input"] = kwargs["input"]
        return subprocess.CompletedProcess(args=[], returncode=0, stdout="?a@C@@QAEXXZ\nx\n\n", stderr="")

    monkeypatch.setattr(demangle_module, "tool_path", lambda: "llvm-undname")
    monkeypatch.setattr(demangle_module.subprocess, "run", run)

    demangle(["srGetLibraryVersion", "?a@C@@QAEXXZ"])

    assert "srGetLibraryVersion" not in captured["input"]


def test_missing_tool_is_reported_rather_than_silently_skipped(monkeypatch: Any) -> None:
    monkeypatch.setattr(demangle_module.shutil, "which", lambda _name: None)

    with pytest.raises(DemanglerMissing):
        demangle(["?a@C@@QAEXXZ"])


@pytest.mark.skipif(not _HAS_TOOL, reason="llvm-undname is not installed")
def test_real_demangler_decodes_the_shapes_this_corpus_depends_on() -> None:
    names = [
        "??_7srBinIFStream@@6BsrBinFStream@@@",
        "??1Decompressor@srHuffman@@QAE@XZ",
        "?verify@srMaterial@@UAEXW4e_verify@srRuntimeClass@@@Z",
        "?CPU_Features_Mask@srTimer@@1KB",
    ]

    result = demangle(names)

    assert result[names[0]] == "const srBinIFStream::`vftable'{for `srBinFStream'}"
    assert result[names[1]] == "public: __thiscall srHuffman::Decompressor::~Decompressor(void)"
    assert "virtual" in result[names[2]]
    assert "static" in result[names[3]]
