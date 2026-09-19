"""Per-row Ghidra mutation helpers with transactional rollback."""

from __future__ import annotations

from collections.abc import Callable, Iterator, Mapping, Sequence
from contextlib import contextmanager
from typing import Any


class RowApplyError(Exception):
    """Abort a per-row transaction while recording a structured error payload."""

    def __init__(self, payload: Mapping[str, Any]):
        super().__init__(str(payload.get("error") or "apply-failed"))
        self.payload = dict(payload)


def transaction_is_open(program: Any) -> bool:
    """True when ``program`` already has a Ghidra transaction."""

    for name in ("getCurrentTransactionInfo", "getCurrentTransaction"):
        getter = getattr(program, name, None)
        if getter is None:
            continue
        try:
            value = getter()
        except Exception:  # noqa: BLE001
            value = None
        if value is not None:
            return True
    return False


def auto_parameters(function: Any) -> list[Any]:
    """Ghidra's synthetic parameters for ``function``: thiscall ``this``, return storage.

    ``Function.replaceParameters`` silently ignores a call that omits them, so
    every prototype rewrite must carry the existing auto parameters over.
    """

    return [
        parameter for parameter in function.getParameters() if bool(parameter.isAutoParameter())
    ]


@contextmanager
def program_transaction(program: Any, description: str) -> Iterator[None]:
    """Start a transaction only when the program is not already in one.

    Nested Ghidra transactions are not savepoints: rolling one back aborts the
    outer transaction. ``ghidra sync`` therefore does not hold a batch
    transaction; each apply row owns a top-level transaction that can roll
    back independently. This guard still refuses to nest if a caller already
    opened a transaction.
    """

    if transaction_is_open(program):
        yield
        return
    import pyghidra

    with pyghidra.transaction(program, description):
        yield


def apply_rows(
    program: Any,
    rows: Sequence[Mapping[str, Any]],
    apply_one: Callable[[Any, Mapping[str, Any]], Mapping[str, Any]],
    *,
    description: str,
) -> dict[str, Any]:
    """Apply each row in its own transaction and record failures without aborting the batch.

    An ``error`` result or exception rolls that row back. Other rows keep their
    committed mutations. Do not wrap a whole ``ghidra sync`` in an outer
    transaction; nested rollback would discard successful rows.
    """

    applied: list[dict[str, Any]] = []
    errors: list[dict[str, Any]] = []
    for row in rows:
        label = (
            row.get("address")
            or row.get("class")
            or row.get("name")
            or row.get("field")
            or row.get("family")
            or "?"
        )
        try:
            with program_transaction(program, f"{description}: {label}"):
                result = dict(apply_one(program, row))
                if result.get("error"):
                    raise RowApplyError(result)
            applied.append(result)
        except RowApplyError as exc:
            errors.append(exc.payload)
        except Exception as exc:  # noqa: BLE001
            errors.append({**dict(row), "error": str(exc)})
    return {"applied": len(applied), "errors": errors, "rows": applied}
