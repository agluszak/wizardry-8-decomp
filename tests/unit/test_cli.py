import json
from pathlib import Path
from types import SimpleNamespace

import pytest
from typer.testing import CliRunner
from wiz8decomp import command_support
from wiz8decomp.cli import app


def _reccmp_project(root: Path) -> None:
    (root / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )


@pytest.mark.parametrize("scenario", [None, "main-game-start"])
@pytest.mark.parametrize(
    "reason,exit_code",
    [
        ("exited with code 3", 3),
        ("SIGSEGV", 1),
        ("breakpoint-hit", 0),
    ],
)
def test_debug_uses_existing_selected_product_before_launch(
    monkeypatch, scenario, reason, exit_code
) -> None:
    from wiz8decomp import build
    from wiz8decomp.debug import debugger

    events = []
    monkeypatch.setattr(command_support, "settings", lambda: SimpleNamespace())
    monkeypatch.setattr(build, "build_target", lambda *_: pytest.fail("must not build by default"))
    monkeypatch.setattr(
        build, "warn_if_product_may_be_stale", lambda _, target: events.append(("check", target))
    )

    def launch(_, arguments, **options):
        events.append("launch")
        assert arguments == []
        assert options == {
            "scenario": scenario,
            "timeout": 7,
            "breakpoints": [(0x401000, "$eax == 1")],
        }
        return {
            "report": "stopped\n",
            "reason": reason,
            "exit_code": exit_code,
            "log": "raw.txt",
            "session": "session.json",
        }

    monkeypatch.setattr(debugger, "run_debugger", launch)
    arguments = ["debug", "--timeout", "7", "--break", "0x401000:$eax == 1"]
    if scenario is not None:
        arguments.extend(["--scenario", scenario])
    result = CliRunner().invoke(app, arguments)
    assert result.exit_code == exit_code, result.output
    assert events == [("check", "runtime-test" if scenario else "runtime"), "launch"]
    assert "session: session.json" in result.output


def test_compare_changed_uses_existing_index_without_building(tmp_path, monkeypatch) -> None:
    from wiz8decomp import build, comparison, source_index

    settings = SimpleNamespace(repo_dir=tmp_path, ghidra_install_dir=tmp_path / "ghidra")
    _reccmp_project(tmp_path)
    source = tmp_path / "new.cpp"
    source.write_text("// FUNCTION: WIZ8 0x00401000\nvoid added() {}\n")
    (tmp_path / "build").mkdir()
    index = tmp_path / "build/source-index.json"
    stale = {"markers": []}
    index.write_text(json.dumps(stale))
    events = []

    index.write_text(
        json.dumps(
            {
                **stale,
                "markers": [
                    {
                        "marker_kind": "FUNCTION",
                        "address": 0x401000,
                        "source_file": "new.cpp",
                        "target": "WIZ8",
                    },
                ],
            }
        )
    )

    inventory = tmp_path / "evidence/observations/compiler-emissions.csv"
    inventory.parent.mkdir(parents=True)
    inventory.write_text(
        "target|address|symbol|name|type|source_files\nWIZ8|00401020||Grow<int>|template|new.cpp\n"
    )

    def compare(_repo, _target, selected, ghidra_install_dir, **_kwargs):
        assert selected == [0x401000, 0x401020]
        assert ghidra_install_dir == settings.ghidra_install_dir
        events.append("compare")
        return {
            "ok": True,
            "functions": [{"orig": "0x00401000", "name": "added", "outcome": "no-differences"}],
        }

    monkeypatch.setattr(command_support, "settings", lambda: settings)
    monkeypatch.setattr(comparison, "changed_source_files", lambda *_args: [source])
    monkeypatch.setattr(
        source_index, "write_source_index", lambda *_args: pytest.fail("must not refresh index")
    )
    monkeypatch.setattr(
        build, "build_target", lambda *_args: pytest.fail("must not build products")
    )
    monkeypatch.setattr(comparison, "compare_selected", compare)

    result = CliRunner().invoke(app, ["compare", "--changed"])
    assert result.exit_code == 0, result.output
    assert events == ["compare"]
    payload = json.loads(result.stdout)
    assert len(payload["functions"]) == 1
    assert payload["omitted"] == 0
    assert payload["selection"]["changed_files"] == ["new.cpp"]
    assert payload["selection"]["dependent_files"] == []


def test_compare_changed_does_not_fall_back_to_whole_image(tmp_path, monkeypatch) -> None:
    from wiz8decomp import comparison

    _reccmp_project(tmp_path)
    monkeypatch.setattr(command_support, "settings", lambda: SimpleNamespace(repo_dir=tmp_path))
    monkeypatch.setattr(comparison, "changed_source_files", lambda *_args: [])
    result = CliRunner().invoke(app, ["compare", "--changed"])
    assert result.exit_code != 0
    assert isinstance(result.exception, ValueError)
    assert "no functions selected" in str(result.exception)


@pytest.mark.parametrize(
    "arguments,function_name",
    [
        (["vtable", "Widget"], "compare_vtables"),
        (["datacmp"], "compare_data"),
        (["addr", "0x401000"], "translate_addresses"),
    ],
)
def test_inspection_commands_do_not_build_by_default(
    tmp_path, monkeypatch, arguments, function_name
) -> None:
    from wiz8decomp import build, comparison

    _reccmp_project(tmp_path)
    settings = SimpleNamespace(repo_dir=tmp_path)
    calls = []
    monkeypatch.setattr(command_support, "settings", lambda: settings)
    monkeypatch.setattr(build, "build_target", lambda *_args: pytest.fail("must not build"))
    monkeypatch.setattr(
        comparison, function_name, lambda *_args: calls.append(function_name) or {"ok": True}
    )

    result = CliRunner().invoke(app, arguments)

    assert result.exit_code == 0, result.output
    assert calls == [function_name]


@pytest.mark.parametrize(
    "arguments,function_name",
    [
        (["vtable", "Widget"], "compare_vtables"),
        (["datacmp"], "compare_data"),
    ],
)
def test_vtable_and_datacmp_exit_nonzero_when_not_ok(
    tmp_path, monkeypatch, arguments, function_name
) -> None:
    from wiz8decomp import comparison

    _reccmp_project(tmp_path)
    settings = SimpleNamespace(repo_dir=tmp_path)
    monkeypatch.setattr(command_support, "settings", lambda: settings)
    monkeypatch.setattr(comparison, function_name, lambda *_args: {"ok": False, "issue_count": 1})

    result = CliRunner().invoke(app, arguments)

    assert result.exit_code == 1
    assert '"ok": false' in result.output


def test_compare_changed_without_target_markers_is_an_empty_success(tmp_path, monkeypatch) -> None:
    from wiz8decomp import comparison

    _reccmp_project(tmp_path)
    (tmp_path / "build").mkdir()
    (tmp_path / "build/source-index.json").write_text(json.dumps({"markers": []}))
    source = tmp_path / "other.cpp"
    source.write_text("void helper() {}\n")
    monkeypatch.setattr(
        command_support,
        "settings",
        lambda: SimpleNamespace(repo_dir=tmp_path, ghidra_install_dir=tmp_path),
    )
    monkeypatch.setattr(comparison, "changed_source_files", lambda *_args: [source])
    monkeypatch.setattr(
        comparison, "compare_selected", lambda *_args, **_kwargs: pytest.fail("nothing to compare")
    )

    result = CliRunner().invoke(app, ["compare", "--changed"])

    assert result.exit_code == 0, result.output
    assert json.loads(result.stdout)["selected"] == 0
