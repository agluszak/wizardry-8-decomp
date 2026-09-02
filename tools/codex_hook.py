#!/usr/bin/env python3
"""Small, dependency-free Codex lifecycle guardrails for this repository."""

from __future__ import annotations

import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any

MAX_TOOL_OUTPUT = 50_000
VALIDATION_RE = re.compile(r"(?:^|\s)just\s+(test|lint|check|build(?:\s+\S+)?|compare(?:\s+\S+)*)")
MARKER_RE = re.compile(
    r"^\+\s*//\s*(?:FUNCTION|TEMPLATE|LIBRARY):.*?\b(0x[0-9a-fA-F]{6,8})\b", re.MULTILINE
)


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


def _tree(root: Path) -> str:
    try:
        result = subprocess.run(
            ["jj", "log", "-r", "@", "--no-graph", "-T", "commit_id"],
            cwd=root,
            capture_output=True,
            text=True,
            timeout=1,
            check=False,
        )
        if result.returncode == 0 and result.stdout.strip():
            return result.stdout.strip()
    except (OSError, subprocess.SubprocessError):
        pass
    digest = hashlib.sha256()
    for path in sorted(root.glob("src/**/*")):
        if path.is_file():
            digest.update(str(path.relative_to(root)).encode())
            try:
                digest.update(path.read_bytes())
            except OSError:
                continue
    return digest.hexdigest()


def _cache_path(root: Path, event: dict[str, Any]) -> Path:
    session = re.sub(r"[^A-Za-z0-9_.-]", "_", str(event.get("session_id") or "default"))
    return root / "build" / "codex" / f"{session}.json"


def _load_cache(root: Path, event: dict[str, Any]) -> dict[str, Any]:
    path = _cache_path(root, event)
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
        if data.get("tree") == _tree(root):
            return data
    except (OSError, ValueError):
        pass
    return {"tree": _tree(root), "checks": {}}


def _save_cache(root: Path, event: dict[str, Any], data: dict[str, Any]) -> None:
    path = _cache_path(root, event)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def command_key(command: str) -> str | None:
    match = VALIDATION_RE.search(command)
    if not match:
        return None
    value = match.group(1)
    if value.startswith("build"):
        return "build"
    return value.split()[0] if value in {"test", "lint", "check"} else "compare"


def _deny(reason: str) -> dict[str, Any]:
    return {
        "hookSpecificOutput": {
            "hookEventName": "PreToolUse",
            "permissionDecision": "deny",
            "permissionDecisionReason": reason,
        }
    }


def _marker_owner(root: Path, address: str) -> str | None:
    for path in root.glob("src/**/*.cpp"):
        try:
            for line_no, line in enumerate(
                path.read_text(encoding="utf-8", errors="ignore").splitlines(), 1
            ):
                if (
                    address.lower() in line.lower()
                    and "//" in line
                    and any(tag in line for tag in ("FUNCTION:", "TEMPLATE:", "LIBRARY:"))
                ):
                    return f"{path.relative_to(root)}:{line_no}"
        except OSError:
            continue
    return None


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
        key = command_key(command)
        if key:
            cache = _load_cache(_root(event), event)
            record = cache.get("checks", {}).get(key)
            if record and record.get("status") == "pass":
                return _deny(f"`{key}` already passed for this unchanged working tree.")
    if tool == "apply_patch":
        root = _root(event)
        for address in MARKER_RE.findall(command):
            owner = _marker_owner(root, address)
            if owner:
                return _deny(
                    f"{address} already has a canonical marker at {owner}; extend that owner."
                )
    return {}


def _success(response: Any) -> bool:
    if isinstance(response, dict):
        if response.get("exit_code") not in (None, 0):
            return False
        if response.get("isError") is True or response.get("error"):
            return False
    return True


def post_tool(event: dict[str, Any]) -> dict[str, Any]:
    root = _root(event)
    tool_input = event.get("tool_input")
    command = _text(
        tool_input.get("command", "") if isinstance(tool_input, dict) else tool_input or ""
    )
    key = command_key(command)
    if key:
        cache = _load_cache(root, event)
        cache.setdefault("checks", {})[key] = {
            "status": "pass" if _success(event.get("tool_response")) else "fail",
            "command": command,
        }
        _save_cache(root, event, cache)
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
    lines = [f"workspace: {root}", f"tree: {_tree(root)[:12]}"]
    try:
        status = subprocess.run(
            ["jj", "status"], cwd=root, capture_output=True, text=True, timeout=2, check=False
        ).stdout.strip()
        if status:
            lines.append("jj: " + status.splitlines()[0])
    except (OSError, subprocess.SubprocessError):
        pass
    cache = _load_cache(root, event)
    passed = sorted(
        key for key, record in cache.get("checks", {}).items() if record.get("status") == "pass"
    )
    if passed:
        lines.append("validated: " + ", ".join(passed))
    return {
        "hookSpecificOutput": {
            "hookEventName": "SessionStart",
            "additionalContext": "\n".join(lines),
        }
    }


def stop(event: dict[str, Any]) -> dict[str, Any]:
    if event.get("stop_hook_active"):
        return {}
    root = _root(event)
    cache = _load_cache(root, event)
    try:
        diff = subprocess.run(
            ["jj", "diff", "--name-only"],
            cwd=root,
            capture_output=True,
            text=True,
            timeout=1,
            check=False,
        ).stdout
    except (OSError, subprocess.SubprocessError):
        diff = ""
    changed_paths = [Path(path) for path in diff.splitlines()]
    changed_cpp = any(path.suffix in {".cpp", ".h"} for path in changed_paths)
    changed_python = any(path.suffix == ".py" for path in changed_paths)
    changed_policy = any(
        path.name in {"AGENTS.md", "hooks.json"} or ".agents" in path.parts
        for path in changed_paths
    )
    changed = changed_cpp or changed_python or changed_policy
    if not changed:
        return {}
    checks = cache.get("checks", {})
    required = (
        ("test", "compare")
        if changed_cpp
        else ("check",)
        if changed_python or changed_policy
        else ()
    )
    missing = [
        key
        for key in required
        if not checks.get(key) or checks[key].get("status") not in {"pass", "fail"}
    ]
    if missing:
        return {
            "decision": "block",
            "reason": "Before stopping, run the missing validation: "
            + ", ".join("just " + key for key in missing),
        }
    return {}


def main() -> None:
    try:
        event = json.load(sys.stdin)
        name = str(event.get("hook_event_name", ""))
        result = {
            "SessionStart": session_start,
            "PreToolUse": pre_tool,
            "PostToolUse": post_tool,
            "Stop": stop,
        }.get(name, lambda _: {})(event)
        print(json.dumps(result, ensure_ascii=False))
    except (OSError, ValueError, TypeError, KeyError, subprocess.SubprocessError) as exc:
        print(json.dumps({"systemMessage": f"Codex guardrail unavailable: {exc}"}))


if __name__ == "__main__":
    main()
