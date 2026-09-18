from types import SimpleNamespace

from wiz8decomp.ghidra.mutations import apply_rows, transaction_is_open
from wiz8decomp.ghidra.sync import (
    _explicit_parameter_types,
    _has_function_overlap,
    _parameter_names_from_signature,
)


def test_function_overlap_is_a_hard_sync_conflict() -> None:
    assert _has_function_overlap(
        [{"address": "0x00470380", "action": "conflict", "error": "function-overlap"}]
    )
    assert not _has_function_overlap([{"error": "unresolved-signature"}])
    assert not _has_function_overlap([])


def test_explicit_parameter_types_drop_this_pointer() -> None:
    identity = SimpleNamespace(
        has_this=True,
        owning_class="W8Character",
        parameter_types=("W8Character *", "int"),
    )
    assert _explicit_parameter_types(identity) == ("int",)
    free = SimpleNamespace(
        has_this=False,
        owning_class=None,
        parameter_types=("W8Character *", "W8ItemInstance *", "int", "int"),
    )
    assert _explicit_parameter_types(free) == free.parameter_types


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
