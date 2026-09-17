"""Tests for calling-convention evidence classification."""

from __future__ import annotations

from wiz8decomp.convention_heuristics import classify_convention_evidence


def test_ret_n_alone_is_not_high_confidence_stdcall() -> None:
    result = classify_convention_evidence(in_vtable=False, incoming_ecx=None, purge=8)
    assert result["proposed"] is None
    assert result["high_confidence"] is False
    assert "callee-cleans-8" in result["evidence"]


def test_vtable_alone_is_high_confidence_thiscall() -> None:
    result = classify_convention_evidence(in_vtable=True, incoming_ecx=None, purge=None)
    assert result["proposed"] == "__thiscall"
    assert result["high_confidence"] is True
    assert "vtable-slot" in result["evidence"]


def test_ecx_alone_is_not_high_confidence() -> None:
    result = classify_convention_evidence(in_vtable=False, incoming_ecx=True, purge=None)
    assert result["proposed"] == "__thiscall"
    assert result["high_confidence"] is False


def test_ecx_plus_purge_is_not_high_confidence_thiscall() -> None:
    """``__fastcall`` also uses ECX and callee cleanup; do not auto-apply."""

    result = classify_convention_evidence(in_vtable=False, incoming_ecx=True, purge=4)
    assert result["proposed"] == "__thiscall"
    assert result["high_confidence"] is False


def test_no_ecx_no_purge_is_inconclusive_not_cdecl() -> None:
    result = classify_convention_evidence(in_vtable=False, incoming_ecx=False, purge=0)
    assert result["proposed"] is None
    assert result["high_confidence"] is False
    assert "no-incoming-ecx" in result["evidence"]
    assert "callee-cleans-0" in result["evidence"]


def test_invalid_purge_max_value_never_high_confidence() -> None:
    result = classify_convention_evidence(in_vtable=False, incoming_ecx=True, purge=2147483647)
    assert result["proposed"] == "__thiscall"
    assert result["high_confidence"] is False
    assert not any(item.startswith("callee-cleans-") for item in result["evidence"])


def test_invalid_purge_max_value_minus_one_never_high_confidence() -> None:
    result = classify_convention_evidence(in_vtable=False, incoming_ecx=True, purge=2147483646)
    assert result["proposed"] == "__thiscall"
    assert result["high_confidence"] is False
    assert not any(item.startswith("callee-cleans-") for item in result["evidence"])
