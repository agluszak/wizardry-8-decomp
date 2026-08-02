from __future__ import annotations

from wiz8decomp.ghidra.unit_intervals import TranslationUnitResolver


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
