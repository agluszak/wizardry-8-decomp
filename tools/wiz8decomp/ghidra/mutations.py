"""Per-row Ghidra mutation helpers with transactional rollback."""

from __future__ import annotations

from collections.abc import Callable, Mapping, Sequence
from typing import Any


class RowApplyError(Exception):
    """Abort a per-row transaction while recording a structured error payload."""

    def __init__(self, payload: Mapping[str, Any]):
        super().__init__(str(payload.get("error") or "apply-failed"))
        self.payload = dict(payload)


def apply_rows(
    program: Any,
    rows: Sequence[Mapping[str, Any]],
    apply_one: Callable[[Any, Mapping[str, Any]], Mapping[str, Any]],
    *,
    description: str,
) -> dict[str, Any]:
    """Apply each row in its own ``pyghidra.transaction``.

    On exception the row's transaction rolls back and the failure is recorded
    without aborting the remaining rows. If ``apply_one`` returns a mapping with
    an ``error`` key, that row is treated as a failure and rolled back.
    """

    import pyghidra

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
            with pyghidra.transaction(program, f"{description}: {label}"):
                result = dict(apply_one(program, row))
                if result.get("error"):
                    raise RowApplyError(result)
                applied.append(result)
        except RowApplyError as exc:
            errors.append(exc.payload)
        except Exception as exc:  # noqa: BLE001
            errors.append({**dict(row), "error": str(exc)})
    return {"applied": len(applied), "errors": errors, "rows": applied}
