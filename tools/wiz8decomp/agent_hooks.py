"""Shared, dependency-light lifecycle policy for Codex and OpenCode."""

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any


def _root(event: dict[str, Any]) -> Path:
    cwd = Path(str(event.get("cwd") or os.getcwd())).resolve()
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=1,
            check=False,
        )
        if result.returncode == 0:
            return Path(result.stdout.strip()).resolve()
    except (OSError, subprocess.SubprocessError):
        pass
    return cwd


def session_start(event: dict[str, Any]) -> dict[str, Any]:
    root = _root(event)
    lines = [f"workspace: {root}"]
    try:
        status = subprocess.run(
            ["jj", "status"], cwd=root, capture_output=True, text=True, timeout=2, check=False
        ).stdout.strip()
        if status:
            lines.append("jj: " + status.splitlines()[0])
    except (OSError, subprocess.SubprocessError):
        pass
    return {
        "hookSpecificOutput": {
            "hookEventName": "SessionStart",
            "additionalContext": "\n".join(lines),
        }
    }


def main() -> None:
    try:
        event = json.load(sys.stdin)
        name = str(event.get("hook_event_name", ""))
        result = {"SessionStart": session_start}.get(name, lambda _: {})(event)
        print(json.dumps(result, ensure_ascii=False))
    except (OSError, ValueError, TypeError, KeyError, subprocess.SubprocessError) as exc:
        print(json.dumps({"systemMessage": f"Codex guardrail unavailable: {exc}"}))


if __name__ == "__main__":
    main()
