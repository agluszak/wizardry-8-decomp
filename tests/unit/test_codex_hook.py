from __future__ import annotations

import json
from pathlib import Path

from wiz8decomp import agent_hooks as HOOK


def event(tmp_path: Path, name: str, **values: object) -> dict[str, object]:
    return {"cwd": str(tmp_path), "hook_event_name": name, "session_id": "test", **values}


def test_pre_tool_blocks_private_and_obsolete_commands(tmp_path: Path) -> None:
    assert HOOK.pre_tool(
        event(
            tmp_path,
            "PreToolUse",
            tool_name="Bash",
            tool_input={"command": "python -c '_recover(...)'"},
        )
    )
    assert HOOK.pre_tool(
        event(
            tmp_path, "PreToolUse", tool_name="Bash", tool_input={"command": "just triage 0x123456"}
        )
    )
    assert not HOOK.pre_tool(
        event(
            tmp_path,
            "PreToolUse",
            tool_name="Bash",
            tool_input={"command": "just context 0x123456"},
        )
    )
    assert HOOK.pre_tool(
        event(
            tmp_path,
            "PreToolUse",
            tool_name="Bash",
            tool_input={"command": "llvm-objdump -s Wiz8.exe"},
        )
    )


def test_pre_tool_rejects_redundant_validation(tmp_path: Path) -> None:
    cache = tmp_path / "build" / "codex"
    cache.mkdir(parents=True)
    tree = HOOK._tree(tmp_path)
    (cache / "test.json").write_text(
        json.dumps({"tree": tree, "checks": {"test": {"status": "pass"}}})
    )
    result = HOOK.pre_tool(
        event(tmp_path, "PreToolUse", tool_name="Bash", tool_input={"command": "just test"})
    )
    assert result["hookSpecificOutput"]["permissionDecision"] == "deny"
    (tmp_path / "src").mkdir()
    (tmp_path / "src" / "changed.cpp").write_text("changed")
    assert not HOOK.pre_tool(
        event(tmp_path, "PreToolUse", tool_name="Bash", tool_input={"command": "just test"})
    )


def test_post_tool_rejects_duplicate_marker_for_any_edit_tool(tmp_path: Path) -> None:
    source = tmp_path / "src" / "x.cpp"
    source.parent.mkdir(parents=True)
    source.write_text("// FUNCTION: WIZ8 0x005CF520\nvoid f();\n")
    source.write_text(
        "// FUNCTION: WIZ8 0x005CF520\nvoid f();\n// FUNCTION: WIZ8 0x005CF520\nvoid g();\n"
    )
    result = HOOK.post_tool(
        event(
            tmp_path,
            "PostToolUse",
            tool_name="apply_patch",
            tool_input={"command": "edit"},
            tool_response={"exit_code": 0},
        )
    )
    assert result["continue"] is False
    assert "duplicate" in result["reason"].lower()


def test_post_tool_bounds_large_output_and_records_validation(tmp_path: Path) -> None:
    response = {"exit_code": 0, "output": "x" * 60_000}
    result = HOOK.post_tool(
        event(
            tmp_path,
            "PostToolUse",
            tool_name="Bash",
            tool_use_id="abc",
            tool_input={"command": "just test"},
            tool_response=response,
        )
    )
    assert result["continue"] is False
    assert (tmp_path / "build/logs/codex-tool/abc.log").exists()
    cache = json.loads((tmp_path / "build/codex/test.json").read_text())
    assert cache["checks"]["test"]["status"] == "pass"


def test_failed_validation_is_attempted_and_does_not_loop_stop(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.setattr(HOOK, "_root", lambda _: tmp_path)
    monkeypatch.setattr(
        HOOK,
        "_changed_paths",
        lambda _root: [Path("src/changed.cpp")],
    )
    monkeypatch.setattr(HOOK, "_changed_compare_keys", lambda _root: {"compare:0x005cf520"})
    for command in ("just test", "just compare 0x005cf520"):
        HOOK.post_tool(
            event(
                tmp_path,
                "PostToolUse",
                tool_name="Bash",
                tool_input={"command": command},
                tool_response={"exit_code": 1},
            )
        )
    assert HOOK.stop(event(tmp_path, "Stop")) == {}


def test_stop_requests_missing_checks_only_for_changed_cpp(tmp_path: Path, monkeypatch) -> None:
    monkeypatch.setattr(HOOK, "_root", lambda _: tmp_path)
    monkeypatch.setattr(
        HOOK.subprocess,
        "run",
        lambda *args, **kwargs: type("R", (), {"stdout": "src/x.cpp\n", "returncode": 0})(),
    )
    result = HOOK.stop(event(tmp_path, "Stop"))
    assert result["decision"] == "block"
    assert "just test" in result["reason"]


def test_command_key_normalizes_validation_commands() -> None:
    assert HOOK.command_key("just lint") == "lint"
    assert (
        HOOK.command_key("just compare W8DialogInterface::open")
        == "compare:W8DialogInterface::open"
    )
    assert HOOK.command_key("just compare 0x005CFA00") == "compare:0x005cfa00"


def test_frontend_adapters_delegate_to_uv_shared_policy() -> None:
    hooks = Path(".codex/hooks.json").read_text()
    plugin = Path(".opencode/plugins/wiz8-guard.ts").read_text()
    assert "uv run" in hooks
    for text in (plugin,):
        assert '"uv", "run", "--frozen"' in text
        assert "wiz8decomp.agent_hooks" in text
        assert "/usr/bin/python" not in text
    assert "just triage" not in plugin
