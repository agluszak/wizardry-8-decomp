"""Typed boundary around the third-party GDB/MI record parser."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from pygdbmi.gdbmiparser import parse_response


@dataclass(frozen=True)
class MiRecord:
    raw: str
    record_type: str
    message: str | None
    payload: Any
    token: int | None
    parser_error: str | None = None


def parse_mi_record(line: str) -> MiRecord:
    """Parse one MI line without exposing pygdbmi's dictionary schema."""
    try:
        parsed = parse_response(line)
    except Exception as error:  # noqa: BLE001 - pygdbmi raises untyped parse errors
        return MiRecord(line, "unparsed", None, None, None, str(error))
    token = parsed.get("token")
    return MiRecord(
        raw=line,
        record_type=str(parsed.get("type", "unparsed")),
        message=parsed.get("message"),
        payload=parsed.get("payload"),
        token=token if isinstance(token, int) else None,
    )


def stream_text(record: MiRecord) -> str | None:
    if record.record_type not in {"console", "target", "log", "gdb-stderr"}:
        return None
    return record.payload if isinstance(record.payload, str) else record.raw
