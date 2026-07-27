"""Type-variable derivation and unification, on the Prop shapes and real ledger."""

from __future__ import annotations

from pathlib import Path

from wiz8decomp.typevars import derive_type_variables, load_knowledge, unify

REPOSITORY = Path(__file__).resolve().parents[2]

# The access stream field-accesses returns for the Prop destructor at
# 0x0044BEC0, reduced to the fields the derivation consumes. Regenerate with:
# just ghidra query <canonical> type-variables 0x0044bec0 this
_PROP_ACCESSES = [
    {"kind": "load", "site": "0044bee4", "path": "this", "offset": "0x14"},
    {"kind": "load", "site": "0044bef9", "path": "this", "offset": "0x20"},
    {"kind": "load", "site": "0044bf09", "path": "this", "offset": "0x28"},
    {"kind": "load", "site": "0044bf16", "path": "this", "offset": "0x38"},
    {"kind": "null-test", "site": "0044bee7", "path": "this[0x14]", "offset": "0x0"},
    {"kind": "load", "site": "0044bef3", "path": "this[0x14]", "offset": "0x0"},
    {"kind": "load", "site": "0044bef7", "path": "this[0x14][0x0]", "offset": "0x0"},
    {
        "kind": "indirect-call-target",
        "site": "0044bef7",
        "path": "this[0x14][0x0][0x0]",
        "offset": "0x0",
    },
    {"kind": "null-test", "site": "0044befc", "path": "this[0x20]", "offset": "0x0"},
    {
        "kind": "call-arg",
        "site": "0044bf01",
        "path": "this[0x20]",
        "offset": "0x0",
        "argument": 0,
        "target": "005e1c10",
    },
    {"kind": "null-test", "site": "0044bf0c", "path": "this[0x28]", "offset": "0x0"},
    {"kind": "load", "site": "0044bf10", "path": "this[0x28]", "offset": "0x0"},
    {"kind": "load", "site": "0044bf14", "path": "this[0x28][0x0]", "offset": "0x0"},
    {
        "kind": "indirect-call-target",
        "site": "0044bf14",
        "path": "this[0x28][0x0][0x0]",
        "offset": "0x0",
    },
    {"kind": "null-test", "site": "0044bf19", "path": "this[0x38]", "offset": "0x0"},
    {
        "kind": "call-arg",
        "site": "0044bf25",
        "path": "this[0x38]",
        "offset": "0x0",
        "argument": 0,
        "target": "005e1c10",
    },
]
_PROP_CALLS = [
    {"op": "CALLIND", "site": "0044bef7", "order": 55, "block": 1, "target": {"space": "unique"}},
    {"op": "CALL", "site": "0044bf01", "order": 74, "block": 3, "target": "005e1c10"},
    {"op": "CALLIND", "site": "0044bf14", "order": 106, "block": 5, "target": {"space": "unique"}},
    {"op": "CALL", "site": "0044bf1f", "order": 123, "block": 7, "target": "004b6ed0"},
    {"op": "CALL", "site": "0044bf25", "order": 129, "block": 7, "target": "005e1c10"},
    {"op": "CALL", "site": "0044bf37", "order": 145, "block": 8, "target": "004b6b60"},
]


def _prop_variables() -> dict[str, dict]:
    variables = derive_type_variables(
        "0044bec0", "this", _PROP_ACCESSES, _PROP_CALLS, {"005e1c10"}
    )
    return {v["root_offset"]: v for v in variables}


def test_the_four_prop_pointee_shapes_are_derived_not_written() -> None:
    variables = _prop_variables()

    assert variables["0x14"]["constraints"]["has_virtual_destructor"] is True
    assert variables["0x14"]["constraints"]["deleting_destructor_slot"] == 0
    assert variables["0x20"]["constraints"]["declared_empty_destructor"] is True
    assert variables["0x28"]["constraints"]["has_virtual_destructor"] is True
    assert variables["0x38"]["constraints"]["complete_destructor"] == "004b6ed0"
    assert variables["0x38"]["constraints"]["destructor_is_virtual"] is False
    for variable in variables.values():
        assert variable["constraints"]["null_checked"] is True
        assert variable["name"].startswith("AnonymousType_0044bec0_field")


def test_an_unrecovered_destructor_unifies_with_nothing() -> None:
    # 0x004B6ED0 writes no vtable and belongs to no reviewed class; the right
    # answer is empty, and the variable keeps its identity until someone
    # recovers that type - never a forced guess.
    variables = _prop_variables()
    knowledge = load_knowledge(REPOSITORY)

    assert unify(variables["0x38"], knowledge) == []


def test_a_known_destructor_unifies_decisively_with_its_class() -> None:
    # GrCycle's reviewed complete destructor is 0x004A6610; a variable
    # constrained to that address selects exactly GrCycle, at reviewed tier.
    knowledge = load_knowledge(REPOSITORY)
    variable = {
        "name": "AnonymousType_test_field0",
        "constraints": {"pointer": True, "complete_destructor": "004a6610"},
    }

    matches = unify(variable, knowledge)

    assert [match["type"] for match in matches] == ["GrCycle"]
    assert matches[0]["tier"] == "reviewed"


def test_shape_only_constraints_filter_rather_than_identify() -> None:
    variables = _prop_variables()
    knowledge = load_knowledge(REPOSITORY)

    matches = unify(variables["0x14"], knowledge)

    # Many polymorphic types are consistent with "virtual destructor in slot
    # 0"; the answer is a candidate pool, and every reason says so.
    assert len(matches) > 20
    assert all("polymorphic" in match["reasons"][0] for match in matches)
