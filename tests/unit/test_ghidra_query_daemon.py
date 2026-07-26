from __future__ import annotations

from contextlib import nullcontext
from typing import Any

import pytest
from wiz8decomp.ghidra import query_daemon


def _settings() -> Any:
    return object()


def test_query_automatically_uses_the_daemon(monkeypatch: pytest.MonkeyPatch) -> None:
    settings = _settings()
    started: list[tuple[Any, str]] = []
    monkeypatch.setattr(query_daemon, "resolve_program_name", lambda _settings, selector: selector)
    monkeypatch.setattr(
        query_daemon,
        "ensure_daemon",
        lambda daemon_settings, program: started.append((daemon_settings, program)),
    )
    monkeypatch.setattr(query_daemon, "daemon_query", lambda _settings, request: request)

    result, transport = query_daemon.query(settings, "canonical", "sections", [])

    assert transport == "daemon"
    assert result["command"] == "sections"
    assert result["program"] == "canonical"
    assert started == [(settings, "canonical")]


def test_ensure_daemon_switches_programs_under_the_lifecycle_lock(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    settings = _settings()
    stopped: list[Any] = []
    started: list[tuple[Any, str]] = []
    monkeypatch.setattr(query_daemon, "lifecycle_lock", lambda _settings: nullcontext())
    monkeypatch.setattr(
        query_daemon,
        "daemon_status",
        lambda _settings: {"running": True, "program": "old-program"},
    )
    monkeypatch.setattr(
        query_daemon,
        "_stop_daemon",
        lambda daemon_settings: stopped.append(daemon_settings),
    )
    monkeypatch.setattr(
        query_daemon,
        "_start_daemon",
        lambda daemon_settings, program: (
            started.append((daemon_settings, program)) or {"running": True, "program": program}
        ),
    )

    status = query_daemon.ensure_daemon(settings, "new-program")

    assert status == {"running": True, "program": "new-program"}
    assert stopped == [settings]
    assert started == [(settings, "new-program")]


@pytest.mark.parametrize(
    "failure",
    [ConnectionRefusedError(), query_daemon.DaemonProgramMismatchError()],
)
def test_query_restarts_a_daemon_that_dies_or_switches_before_the_request(
    monkeypatch: pytest.MonkeyPatch, failure: Exception
) -> None:
    settings = _settings()
    ensured: list[str] = []
    stopped: list[Any] = []
    responses: list[object] = [failure, {"ok": True}]
    monkeypatch.setattr(query_daemon, "resolve_program_name", lambda _settings, selector: selector)
    monkeypatch.setattr(
        query_daemon,
        "ensure_daemon",
        lambda _settings, program: ensured.append(program),
    )
    monkeypatch.setattr(
        query_daemon,
        "stop_daemon",
        lambda daemon_settings, quiet=False: stopped.append((daemon_settings, quiet)),
    )

    def daemon_query(_settings: Any, _request: dict[str, Any]) -> dict[str, Any]:
        response = responses.pop(0)
        if isinstance(response, Exception):
            raise response
        return response

    monkeypatch.setattr(query_daemon, "daemon_query", daemon_query)

    result, transport = query_daemon.query(settings, "canonical", "sections", [])

    assert (result, transport) == ({"ok": True}, "daemon")
    assert ensured == ["canonical", "canonical"]
    assert stopped == [(settings, True)]


def test_query_falls_back_only_when_daemon_startup_is_unavailable(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    settings = _settings()
    monkeypatch.setattr(query_daemon, "resolve_program_name", lambda _settings, selector: selector)
    monkeypatch.setattr(
        query_daemon,
        "ensure_daemon",
        lambda _settings, _program: (_ for _ in ()).throw(RuntimeError("unavailable")),
    )
    monkeypatch.setattr(
        query_daemon,
        "one_shot_query",
        lambda _settings, program, command, arguments: {
            "program": program,
            "command": command,
            "arguments": arguments,
        },
    )

    result, transport = query_daemon.query(settings, "canonical", "sections", [])

    assert transport == "one-shot-fallback"
    assert result["program"] == "canonical"


def test_query_does_not_hide_errors_returned_by_a_healthy_daemon(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    settings = _settings()
    monkeypatch.setattr(query_daemon, "resolve_program_name", lambda _settings, selector: selector)
    monkeypatch.setattr(query_daemon, "ensure_daemon", lambda _settings, _program: None)
    monkeypatch.setattr(
        query_daemon,
        "daemon_query",
        lambda _settings, _request: (_ for _ in ()).throw(RuntimeError("bad query")),
    )

    with pytest.raises(RuntimeError, match="bad query"):
        query_daemon.query(settings, "canonical", "sections", [])
