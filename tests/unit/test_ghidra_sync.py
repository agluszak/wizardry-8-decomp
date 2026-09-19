from types import SimpleNamespace

from wiz8decomp.ghidra.mutations import apply_rows, transaction_is_open
from wiz8decomp.ghidra.sync import (
    _explicit_parameter_types,
    _has_function_overlap,
    _parameter_names_from_signature,
    _projection_complete,
)


def test_function_overlap_is_a_hard_sync_conflict() -> None:
    assert _has_function_overlap(
        [{"address": "0x00470380", "action": "conflict", "error": "function-overlap"}]
    )
    assert not _has_function_overlap([{"error": "unresolved-signature"}])
    assert not _has_function_overlap([])


def test_projection_complete_rejects_hard_apply_errors() -> None:
    assert _projection_complete([], [], [{"step": "globals", "result": {"applied": 1}}])
    assert not _projection_complete(
        [{"error": "function-overlap"}],
        [],
        [{"step": "globals", "result": {"applied": 1}}],
    )
    assert not _projection_complete(
        [],
        [{"action": "unresolved-signature", "error": "parse-failed"}],
        [],
    )
    assert not _projection_complete(
        [],
        [],
        [{"step": "vbtables", "result": {"error": "boom"}}],
    )
    assert not _projection_complete(
        [],
        [],
        [{"step": "surrender-iat", "result": {"errors": [{"error": "iat-cell-not-typed"}]}}],
    )


def test_explicit_parameter_types_keep_explicit_owner_pointer() -> None:
    identity = SimpleNamespace(
        has_this=True,
        owning_class="Node",
        parameter_types=("Node *", "int"),
    )
    assert _explicit_parameter_types(identity) == ("Node *", "int")
    free = SimpleNamespace(
        has_this=False,
        owning_class=None,
        parameter_types=("W8Character *", "W8ItemInstance *", "int", "int"),
    )
    assert _explicit_parameter_types(free) == free.parameter_types
    const_method = SimpleNamespace(has_this=True, parameter_types=("int",))
    static_method = SimpleNamespace(has_this=False, parameter_types=("int",))
    ctor = SimpleNamespace(has_this=True, parameter_types=())
    dtor = SimpleNamespace(has_this=True, parameter_types=())
    variadic = SimpleNamespace(has_this=False, parameter_types=("char *",), is_variadic=True)
    assert _explicit_parameter_types(const_method) == ("int",)
    assert _explicit_parameter_types(static_method) == ("int",)
    assert _explicit_parameter_types(ctor) == ()
    assert _explicit_parameter_types(dtor) == ()
    assert _explicit_parameter_types(variadic) == ("char *",)
    assert variadic.is_variadic is True


def test_parameter_names_come_from_the_source_signature() -> None:
    signature = (
        "void EquipMatchingPartnerItem(W8Character * character, W8ItemInstance * item, "
        "int item_id, int equip_slot)"
    )
    assert _parameter_names_from_signature(signature, 4) == [
        "character",
        "item",
        "item_id",
        "equip_slot",
    ]
    assert _parameter_names_from_signature("void fn(void)", 0) == []


def test_apply_rows_does_not_nest_when_a_transaction_is_open() -> None:
    program = SimpleNamespace(getCurrentTransactionInfo=lambda: object())
    result = apply_rows(
        program,
        [{"address": "0x1", "action": "set"}],
        lambda _program, row: {**dict(row), "ok": True},
        description="test",
    )
    assert result["applied"] == 1
    assert result["errors"] == []
    assert transaction_is_open(program)


def test_apply_rows_records_error_without_raising_when_nested() -> None:
    program = SimpleNamespace(getCurrentTransactionInfo=lambda: object())
    result = apply_rows(
        program,
        [{"address": "0x1"}],
        lambda _program, row: {**dict(row), "error": "nope"},
        description="test",
    )
    assert result["applied"] == 0
    assert result["errors"][0]["error"] == "nope"
