import sys
from collections.abc import Iterator
from contextlib import contextmanager
from types import SimpleNamespace
from typing import Any

from wiz8decomp.ghidra.mutations import RowApplyError, apply_rows, transaction_is_open
from wiz8decomp.ghidra.sync import (
    _explicit_parameter_types,
    _has_function_overlap,
    _parameter_names_from_signature,
    _projection_complete,
    _record_step,
    _stored_signature_matches,
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


def _data_type(path: str) -> SimpleNamespace:
    return SimpleNamespace(getPathName=lambda: path)


def _parameter(name: str, data_type: Any, *, auto: bool = False) -> SimpleNamespace:
    return SimpleNamespace(
        getName=lambda: name,
        getDataType=lambda: data_type,
        isAutoParameter=lambda: auto,
    )


def _function(
    return_type: Any, parameters: list[Any], *, varargs: bool = False, convention: str = "__cdecl"
) -> SimpleNamespace:
    return SimpleNamespace(
        getReturnType=lambda: return_type,
        getParameters=lambda: parameters,
        hasVarArgs=lambda: varargs,
        getCallingConventionName=lambda: convention,
    )


def _identity(*, has_this: bool = False, convention: str | None = "__cdecl") -> SimpleNamespace:
    return SimpleNamespace(
        has_this=has_this,
        calling_convention=convention,
        is_variadic=False,
    )


def test_stored_signature_agreement_ignores_only_ghidra_auto_this() -> None:
    void = _data_type("/void")
    integer = _data_type("/int")
    auto_this = _parameter("this", _data_type("/Node *"), auto=True)
    stored = _parameter("item_id", integer)
    ghidra_thiscall = _function(void, [auto_this, stored], convention="__thiscall")
    resolved = {"return_type": void, "parameters": [_parameter("item_id", integer)]}

    assert _stored_signature_matches(
        ghidra_thiscall, resolved, _identity(has_this=True, convention=None)
    )

    # A placeholder recovered name defers to whatever ProgramDB already stores.
    resolved_placeholder = {"return_type": void, "parameters": [_parameter("param_0", integer)]}
    assert _stored_signature_matches(
        _function(void, [auto_this, _parameter("iVar1", integer)], convention="__thiscall"),
        resolved_placeholder,
        _identity(has_this=True, convention=None),
    )


def test_stored_signature_rejects_extra_programdb_parameters() -> None:
    """A stored prototype with more parameters is a disagreement, not agreement."""

    void = _data_type("/void")
    integer = _data_type("/int")
    function = _function(void, [_parameter("a", integer), _parameter("b", integer)])
    resolved = {"return_type": void, "parameters": [_parameter("a", integer)]}

    assert not _stored_signature_matches(function, resolved, _identity())


def test_stored_signature_rejects_pointer_for_pointee_return() -> None:
    """``T *`` stored for a ``T`` return is a disagreement, never equivalence."""

    structure = _data_type("/Node")
    pointer = _data_type("/Node *")
    resolved = {"return_type": structure, "parameters": []}

    assert not _stored_signature_matches(_function(pointer, []), resolved, _identity())


def test_stored_signature_rejects_varargs_and_convention_drift() -> None:
    void = _data_type("/void")
    resolved = {"return_type": void, "parameters": []}

    assert not _stored_signature_matches(_function(void, [], varargs=True), resolved, _identity())
    assert not _stored_signature_matches(
        _function(void, [], convention="__stdcall"), resolved, _identity(convention="__cdecl")
    )
    assert not _stored_signature_matches(
        _function(void, [_parameter("a", _data_type("/int"))]),
        resolved,
        _identity(),
    )


class _FakeProgram:
    def __init__(self) -> None:
        self.stored: dict[str, Any] = {}
        self.staged: dict[str, Any] = {}
        self.depth = 0

    def getCurrentTransactionInfo(self) -> None:
        return None


@contextmanager
def _fake_transaction(program: Any, _description: str) -> Iterator[None]:
    """Commit staged writes on success and discard them on any exception."""

    program.depth += 1
    program.staged = {}
    try:
        yield
        program.stored.update(program.staged)
    finally:
        program.staged = {}
        program.depth -= 1


def test_apply_rows_rolls_back_the_failing_row_and_keeps_the_rest(monkeypatch) -> None:
    program = _FakeProgram()
    monkeypatch.setitem(sys.modules, "pyghidra", SimpleNamespace(transaction=_fake_transaction))

    def apply_one(target: Any, row: Any) -> dict[str, Any]:
        target.staged[row["address"]] = row["value"]
        return {"error": "bad-row"} if row.get("bad") else {"action": "set"}

    result = apply_rows(
        program,
        [
            {"address": "0x1", "value": "a"},
            {"address": "0x2", "value": "b", "bad": True},
            {"address": "0x3", "value": "c"},
        ],
        apply_one,
        description="test",
    )

    assert result["applied"] == 2
    assert result["errors"] == [{"error": "bad-row"}]
    # The failing row rolled back; its neighbours committed; no transaction stayed open.
    assert program.stored == {"0x1": "a", "0x3": "c"}
    assert program.depth == 0


def test_row_rollback_blocks_provenance_advance() -> None:
    steps: list[dict[str, Any]] = []
    conflicts: list[dict[str, Any]] = []
    _record_step(steps, conflicts, "globals", {"applied": 1, "errors": [{"error": "bad-row"}]})

    assert conflicts == [{"error": "bad-row"}]
    assert not _projection_complete(conflicts, [], steps)


def test_bad_signature_row_rolls_back_and_blocks_provenance(monkeypatch) -> None:
    """One injected bad signature row: it rolls back, neighbours survive, provenance stops."""

    program = _FakeProgram()
    monkeypatch.setitem(sys.modules, "pyghidra", SimpleNamespace(transaction=_fake_transaction))
    bad = {
        "address": "0x2",
        "action": "unresolved-signature",
        "error": "unresolved-parameter:class BitArray &",
    }

    def apply_one(target: Any, row: Any) -> dict[str, Any]:
        target.staged[row["address"]] = row["signature"]
        return dict(bad) if row["address"] == bad["address"] else {"action": "agree"}

    def guarded(target: Any, row: Any) -> dict[str, Any]:
        # This mirrors the sync loop: a conflict/unresolved row aborts that row.
        result = apply_one(target, row)
        if result.get("action") in {"conflict", "unresolved-signature"}:
            raise RowApplyError(result)
        return result

    result = apply_rows(
        program,
        [
            {"address": "0x1", "signature": "void a(int)"},
            {"address": "0x2", "signature": "void b(class BitArray &)"},
            {"address": "0x3", "signature": "void c(void)"},
        ],
        guarded,
        description="Prototype 0x2",
    )

    assert result["applied"] == 2
    assert result["errors"] == [bad]
    # The bad row left no trace; its neighbours committed; nothing stayed open.
    assert program.stored == {"0x1": "void a(int)", "0x3": "void c(void)"}
    assert program.depth == 0

    steps: list[dict[str, Any]] = []
    conflicts: list[dict[str, Any]] = []
    _record_step(steps, conflicts, "prototypes", result)
    assert conflicts == [bad]
    assert not _projection_complete(conflicts, [], steps)
