from __future__ import annotations

from contextlib import contextmanager
from typing import Any

import pytest
from wiz8decomp.ghidra import query_daemon


@contextmanager
def _opened(program: Any, calls: list[str]):
    calls.append("open")
    yield "canonical", program
    calls.append("close")


def test_query_uses_one_existing_project_open(monkeypatch: pytest.MonkeyPatch) -> None:
    program = object()
    calls: list[str] = []
    monkeypatch.setattr(
        query_daemon,
        "open_program",
        lambda _settings, _selector: _opened(program, calls),
    )
    monkeypatch.setattr(
        query_daemon,
        "execute_query",
        lambda actual, command, arguments: {
            "same_program": actual is program,
            "command": command,
            "arguments": arguments,
        },
    )

    result, transport = query_daemon.query(object(), "canonical", "sections", [])

    assert transport == "one-shot"
    assert result == {"same_program": True, "command": "sections", "arguments": []}
    assert calls == ["open", "close"]


def test_query_many_preserves_order_and_opens_only_once(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    program = object()
    calls: list[str] = []
    monkeypatch.setattr(
        query_daemon,
        "open_program",
        lambda _settings, _selector: _opened(program, calls),
    )
    monkeypatch.setattr(
        query_daemon,
        "execute_query",
        lambda actual, command, arguments: {
            "same_program": actual is program,
            "command": command,
            "arguments": arguments,
        },
    )
    queries = [("function", ["0x401000"]), ("read-data", ["0x402000", "16"])]

    result, transport = query_daemon.query_many(object(), "canonical", queries)

    assert transport == "one-shot"
    assert [item["command"] for item in result] == ["function", "read-data"]
    assert [item["result"]["same_program"] for item in result] == [True, True]
    assert calls == ["open", "close"]


def test_query_many_rejects_an_empty_batch() -> None:
    with pytest.raises(ValueError, match="at least one query"):
        query_daemon.query_many(object(), "canonical", [])


def test_daemon_compatibility_calls_are_explicit_no_ops() -> None:
    assert query_daemon.daemon_status(object()) == {"running": False, "removed": True}
    assert query_daemon.stop_daemon(object()) == {
        "running": False,
        "stopped": False,
        "removed": True,
    }
