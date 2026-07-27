"""Member vocabulary recovered from assertion text, and its limits."""

from __future__ import annotations

from pathlib import Path

from wiz8decomp.aggregates import _convention, aggregate_model, member_references

REPOSITORY = Path(__file__).resolve().parents[2]

_ASSERTIONS = (
    "program,call_site,call_kind,function_start,source_path,line,expression,message\n"
    "p,00001001,direct,00001000,C:\\Projects\\Wizardry 8\\Status.cpp,1,"
    "gXStatus.uiMonstersInDatabase <= MAX,\n"
    "p,00001002,direct,00001000,C:\\Projects\\Wizardry 8\\Status.cpp,2,gXStatus.fCombatMode,\n"
    "p,00001003,direct,00002000,C:\\Projects\\Wizardry 8\\Combat.cpp,3,"
    "gpCombat->TargetHit.iChar >= 0,\n"
    "p,00001004,direct,00002000,C:\\Projects\\Wizardry 8\\Combat.cpp,4,gStatus.Char[uiChar],\n"
)


def _repo(tmp_path: Path) -> Path:
    directory = tmp_path / "evidence" / "snapshots" / "call-sites"
    directory.mkdir(parents=True)
    (directory / "assertions.csv").write_text(_ASSERTIONS, encoding="utf-8")
    return tmp_path


def test_the_access_operator_says_whether_the_base_is_a_pointer(tmp_path: Path) -> None:
    model = aggregate_model(member_references(_repo(tmp_path)))

    assert model["gXStatus"]["accessed_through_pointer"] is False
    assert model["gpCombat"]["accessed_through_pointer"] is True


def test_a_chain_establishes_the_thing_at_the_end_of_it(tmp_path: Path) -> None:
    # `gpCombat->TargetHit.iChar` proves gpCombat has a TargetHit and that
    # TargetHit has an iChar - two aggregates, not one.
    model = aggregate_model(member_references(_repo(tmp_path)))

    assert [item["member"] for item in model["gpCombat"]["members"]] == ["TargetHit"]
    assert [item["member"] for item in model["gpCombat->TargetHit"]["members"]] == ["iChar"]


def test_a_subscript_marks_the_member_as_an_array(tmp_path: Path) -> None:
    model = aggregate_model(member_references(_repo(tmp_path)))

    assert model["gStatus"]["members"][0] == {
        "member": "Char",
        "convention": "",
        "array": True,
        "functions": ["00002000"],
        "programs": ["p"],
        "units": ["C:\\Projects\\Wizardry 8\\Combat.cpp"],
    }


def test_the_functions_and_units_that_touch_a_member_come_with_it(tmp_path: Path) -> None:
    model = aggregate_model(member_references(_repo(tmp_path)))
    members = {item["member"]: item for item in model["gXStatus"]["members"]}

    assert members["fCombatMode"]["functions"] == ["00001000"]
    assert members["fCombatMode"]["units"] == ["C:\\Projects\\Wizardry 8\\Status.cpp"]


def test_the_prefix_convention_is_a_hypothesis_about_kind_not_a_layout() -> None:
    assert _convention("uiPartyGold") == "unsigned integer"
    assert _convention("plsItemList") == "pointer to list"
    # `m_` marks a class member and sits before the kind prefix.
    assert _convention("m_pEvent") == "pointer"
    # A name with no prefix gets no hypothesis rather than a guessed one.
    assert _convention("TargetHit") == ""
    assert _convention("Char") == ""


def test_the_opaque_global_state_names_its_own_members() -> None:
    model = aggregate_model(member_references(REPOSITORY))
    members = {item["member"] for item in model["gXStatus"]["members"]}

    assert {"uiMonstersInDatabase", "fCombatMode", "plsItemList"} <= members
    assert model["gXStatus"]["accessed_through_pointer"] is False
    assert len(model) > 50
