from __future__ import annotations

from wiz8decomp.ghidra.unit_intervals import TranslationUnitResolver
from wiz8decomp.reports.recovery_context import _assertion_boundary_defects


def test_direct_assertion_ownership_wins_over_an_interval() -> None:
    assertions = [
        {
            "source_path": r"C:\Projects\Wizardry 8\Local Code\ItemManager.cpp",
            "containing_function": "004f88f0",
        }
    ]
    result = TranslationUnitResolver(assertions).resolve(0x004F88F0)

    assert result == {
        "source_path": r"Local Code\ItemManager.cpp",
        "attribution": "direct",
        "alternatives": [],
    }


def test_multiple_direct_units_are_reported_as_inlined_instead_of_guessed() -> None:
    assertions = [
        {
            "source_path": r"C:\Projects\Wizardry 8\Engine Code\A.cpp",
            "containing_function": "00401000",
        },
        {
            "source_path": r"C:\Projects\Wizardry 8\Local Code\B.cpp",
            "containing_function": "00401000",
        },
    ]
    result = TranslationUnitResolver(assertions).resolve(0x00401000)

    assert result["attribution"] == "inlined-or-conflicting"
    assert result["source_path"] == ""
    assert result["alternatives"] == [r"Engine Code\A.cpp", r"Local Code\B.cpp"]


def test_invalid_assertion_function_boundary_is_structured() -> None:
    assertions = [{"containing_function": "004a42b0", "call_site": "004a42ce"}]

    assert _assertion_boundary_defects(assertions, {0x004A42B0: None, 0x004A42CE: 0x004A42C0}) == [
        {
            "kind": "invalid-assertion-function-boundary",
            "containing_function": "0x004a42b0",
            "call_site": "0x004a42ce",
            "anchor_owner": None,
            "call_site_owner": "0x004a42c0",
        }
    ]
