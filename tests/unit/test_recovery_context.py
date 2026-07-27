from __future__ import annotations

from wiz8decomp.reports.recovery_context import _in_body, _translation_unit


def test_membership_uses_actual_instruction_addresses_not_a_contiguous_size() -> None:
    body = {0x00401000, 0x00401005, 0x00402000}

    assert _in_body({"site": "00402000"}, "site", body)
    assert not _in_body({"site": "00401500"}, "site", body)


def test_direct_assertion_ownership_wins_over_an_interval() -> None:
    assertions = [
        {
            "source_path": r"C:\Projects\Wizardry 8\Local Code\ItemManager.cpp",
        }
    ]
    intervals = [
        {
            "record_type": "translation-unit",
            "lower_address": "004f0000",
            "upper_address": "004fffff",
            "source_path": r"Local Code\Other.cpp",
        }
    ]

    result = _translation_unit(assertions, intervals, 0x004F88F0)

    assert result == {
        "source_path": r"Local Code\ItemManager.cpp",
        "attribution": "direct",
        "alternatives": [],
    }


def test_multiple_direct_units_are_reported_as_inlined_instead_of_guessed() -> None:
    assertions = [
        {"source_path": r"C:\Projects\Wizardry 8\Engine Code\A.cpp"},
        {"source_path": r"C:\Projects\Wizardry 8\Local Code\B.cpp"},
    ]

    result = _translation_unit(assertions, [], 0x00401000)

    assert result["attribution"] == "inlined-or-conflicting"
    assert result["source_path"] == ""
    assert result["alternatives"] == [r"Engine Code\A.cpp", r"Local Code\B.cpp"]
