"""Tests for source-backed calling-convention classification."""

from __future__ import annotations

from wiz8decomp.prototype_repair import classify_pair


def test_classify_pair_agrees_on_explicit_match() -> None:
    assert classify_pair("__thiscall", "__thiscall") == "agree"
    assert classify_pair("__stdcall", "__stdcall") == "agree"


def test_classify_pair_promotes_default_cdecl() -> None:
    assert classify_pair("default", "__cdecl") == "promote-default-cdecl"
    assert classify_pair("unknown", "__cdecl") == "promote-default-cdecl"


def test_classify_pair_sets_thiscall_from_unknown() -> None:
    assert classify_pair("unknown", "__thiscall") == "set-from-source"
    assert classify_pair("default", "__fastcall") == "set-from-source"


def test_classify_pair_hard_disagree() -> None:
    assert classify_pair("__cdecl", "__thiscall") == "hard-disagree"
    assert classify_pair("__stdcall", "__cdecl") == "hard-disagree"
