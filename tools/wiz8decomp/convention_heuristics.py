"""Binary calling-convention evidence classifier for unrecovered functions.

Source-backed ``prototype_repair`` covers recovered ``FUNCTION`` markers during
``ghidra sync``. This classifier scores remaining bodies for investigation; it
does not mutate ProgramDB.

Evidence rules (asymmetric by design):

- Vtable membership is strong ``__thiscall`` evidence.
- Incoming ECX before definition is strong-ish ``__thiscall`` evidence.
- ``ret N`` / stack purge only means "callee cleans N bytes". It never proposes
  ``__stdcall`` by itself (MSVC ``__thiscall`` methods also purge explicit args).
- ``__cdecl`` is never inferred from "no purge + no ECX".
"""

from __future__ import annotations

from typing import Any

# Ghidra uses Integer.MAX_VALUE / MAX_VALUE-1 as invalid / unknown purge sentinels.
_INVALID_PURGE = frozenset({2147483647, 2147483646})


def _valid_purge(purge: int | None) -> int | None:
    if purge is None or purge in _INVALID_PURGE:
        return None
    return purge


def classify_convention_evidence(
    *,
    in_vtable: bool,
    incoming_ecx: bool | None,
    purge: int | None,
) -> dict[str, Any]:
    """Pure evidence classifier.

    Returns ``proposed``, ``evidence``, and ``high_confidence``.
    Invalid Ghidra purge sentinels never contribute evidence or high confidence.
    """

    purge = _valid_purge(purge)
    evidence: list[str] = []
    if in_vtable:
        evidence.append("vtable-slot")
    if incoming_ecx is True:
        evidence.append("incoming-ecx")
    if purge is not None and 0 < purge < 0x7FFFFFFE:
        evidence.append(f"callee-cleans-{purge}")
    if purge == 0:
        evidence.append("callee-cleans-0")
    if incoming_ecx is False:
        evidence.append("no-incoming-ecx")

    proposed: str | None = None
    if "vtable-slot" in evidence or "incoming-ecx" in evidence:
        proposed = "__thiscall"

    # Apply only when method identity is independently corroborated (vtable slot).
    # Incoming ECX + callee cleanup also matches x86 __fastcall and must stay
    # report-only until stronger ABI evidence excludes competing conventions.
    high = proposed == "__thiscall" and "vtable-slot" in evidence

    return {
        "proposed": proposed,
        "evidence": evidence,
        "high_confidence": high,
    }
