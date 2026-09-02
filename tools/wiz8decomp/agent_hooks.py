"""Shared, dependency-light lifecycle policy for Codex and OpenCode."""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any

MAX_TOOL_OUTPUT = 50_000
MARKER_RE = re.compile(r"//\s*(?:FUNCTION|TEMPLATE|LIBRARY):.*?\b(0x[0-9a-fA-F]{6,8})\b")


def _text(value: Any) -> str:
    if isinstance(value, str):
        return value
    return json.dumps(value, ensure_ascii=False, sort_keys=True)


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


def _deny(reason: str) -> dict[str, Any]:
    return {
        "hookSpecificOutput": {
            "hookEventName": "PreToolUse",
            "permissionDecision": "deny",
            "permissionDecisionReason": reason,
        }
    }


def _duplicate_markers(root: Path) -> list[str]:
    owners: dict[str, list[str]] = {}
    for path in root.glob("src/**/*"):
        if not path.is_file():
            continue
        try:
            lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
        except OSError:
            continue
        for line_no, line in enumerate(lines, 1):
            if not any(tag in line for tag in ("FUNCTION:", "TEMPLATE:", "LIBRARY:")):
                continue
            for address in MARKER_RE.findall(line):
                owners.setdefault(address.lower(), []).append(f"{path.relative_to(root)}:{line_no}")
    return [f"{address}: {', '.join(paths)}" for address, paths in owners.items() if len(paths) > 1]


def pre_tool(event: dict[str, Any]) -> dict[str, Any]:
    tool = str(event.get("tool_name", ""))
    value = event.get("tool_input", {})
    command = _text(value.get("command", "") if isinstance(value, dict) else value)
    if tool == "Bash":
        if re.search(r"\b_recover\s*\(", command):
            return _deny(
                "Private recovery entry points are unsupported; use the public `just recover` command."
            )
        if re.search(r"(?:^|\s)just\s+triage(?:\s|$)", command):
            return _deny(
                "`just triage` was removed; `just compare` includes the first-divergence evidence."
            )
        if "llvm-objdump" in command and re.search(
            r"(?:^|\s)(?:-s|--full-contents)(?:\s|$)", command
        ):
            return _deny(
                "Unbounded objdump data dumps are blocked; use bounded context/compare evidence or --deep."
            )
    return {}


def post_tool(event: dict[str, Any]) -> dict[str, Any]:
    root = _root(event)
    if str(event.get("tool_name", "")) in {"apply_patch", "Edit", "Write"}:
        duplicates = _duplicate_markers(root)
        if duplicates:
            return {
                "continue": False,
                "reason": "Duplicate canonical ownership detected: " + "; ".join(duplicates[:4]),
                "hookSpecificOutput": {
                    "hookEventName": "PostToolUse",
                    "additionalContext": "Fix the duplicate marker before continuing.",
                },
            }
    response = _text(event.get("tool_response", ""))
    if len(response) <= MAX_TOOL_OUTPUT:
        return {}
    tool_id = re.sub(r"[^A-Za-z0-9_.-]", "_", str(event.get("tool_use_id") or "unknown"))
    path = root / "build" / "logs" / "codex-tool" / f"{tool_id}.log"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(response, encoding="utf-8")
    preview = response[:1200] + "\n... output elided ...\n" + response[-1200:]
    return {
        "continue": False,
        "reason": f"Large tool output saved to {path.relative_to(root)}",
        "hookSpecificOutput": {"hookEventName": "PostToolUse", "additionalContext": preview},
    }


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
        result = {
            "SessionStart": session_start,
            "PreToolUse": pre_tool,
            "PostToolUse": post_tool,
        }.get(name, lambda _: {})(event)
        print(json.dumps(result, ensure_ascii=False))
    except (OSError, ValueError, TypeError, KeyError, subprocess.SubprocessError) as exc:
        print(json.dumps({"systemMessage": f"Codex guardrail unavailable: {exc}"}))


if __name__ == "__main__":
    main()
