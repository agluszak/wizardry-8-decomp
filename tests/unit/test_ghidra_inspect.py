from pathlib import Path
from types import SimpleNamespace

from typer.testing import CliRunner
from wiz8decomp.cli import app
from wiz8decomp.source_index import source_index_freshness, try_load_source_index


def test_try_load_source_index_missing(tmp_path: Path) -> None:
    assert try_load_source_index(tmp_path) is None


def test_source_index_freshness_missing(tmp_path: Path) -> None:
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    source-root: src/wiz8\n"
        "    hash:\n      sha256: abc\n",
        encoding="utf-8",
    )
    freshness = source_index_freshness(tmp_path, "WIZ8")
    assert freshness["state"] == "missing"


def test_ghidra_read_commands_are_registered() -> None:
    runner = CliRunner()
    result = runner.invoke(app, ["ghidra", "--help"])
    assert result.exit_code == 0, result.output
    assert "decompile" in result.output
    assert "asm" in result.output
    assert "sym" in result.output
    assert "sync" in result.output
    assert "class" in result.output
    assert "flow" in result.output


def test_retired_commands_are_gone() -> None:
    runner = CliRunner()
    assert runner.invoke(app, ["recover", "function", "--help"]).exit_code != 0
    assert runner.invoke(app, ["recover", "explain", "--help"]).exit_code != 0
    assert runner.invoke(app, ["report", "context", "--help"]).exit_code != 0
    assert runner.invoke(app, ["report", "instructions", "--help"]).exit_code != 0
    assert runner.invoke(app, ["report", "data", "--help"]).exit_code != 0
    assert runner.invoke(app, ["report", "class", "--help"]).exit_code != 0
    assert runner.invoke(app, ["report", "flow", "--help"]).exit_code != 0
    assert runner.invoke(app, ["analyze", "enrichment-checkpoint", "--help"]).exit_code != 0
    assert runner.invoke(app, ["analyze", "prototype-repair", "--help"]).exit_code != 0


def test_inspect_does_not_write_source_index(monkeypatch) -> None:
    import contextlib

    from wiz8decomp import source_index as source_index_module
    from wiz8decomp.ghidra import env as env_module
    from wiz8decomp.ghidra import inspect
    from wiz8decomp.ghidra import workspace as workspace_module
    from wiz8decomp.ghidra.resolve import ResolveError

    events: list[str] = []
    monkeypatch.setattr(
        source_index_module,
        "write_source_index",
        lambda *_args, **_kwargs: events.append("write") or {},
    )
    monkeypatch.setattr(inspect, "resolve_program_selector", lambda *_args: "wiz8")
    monkeypatch.setattr(
        inspect,
        "source_metadata",
        lambda *_args: {"state": "missing", "detail": "absent", "available": False},
    )
    monkeypatch.setattr(
        workspace_module,
        "seed_record",
        lambda *_args, **_kwargs: {"program": "wiz8", "sha256": "x"},
    )
    monkeypatch.setattr(
        workspace_module,
        "project_seed_freshness",
        lambda *_args: {"status": "current", "detail": None},
    )
    monkeypatch.setattr(
        env_module, "open_program", lambda *_args, **_kwargs: contextlib.nullcontext(object())
    )
    monkeypatch.setattr(
        inspect,
        "resolve_function",
        lambda *_args, **_kwargs: (_ for _ in ()).throw(
            ResolveError("no function contains 0x00529570")
        ),
    )

    result = inspect.decompile_functions(
        SimpleNamespace(repo_dir=Path("/repo"), build_dir=Path("/repo/build")),
        ["0x00529570"],
        include_candidate=False,
    )
    assert "write" not in events
    assert result["failures"]
    assert result["failures"][0]["candidate"] is None


def test_candidate_carries_parameter_defects() -> None:
    from wiz8decomp.ghidra.inspect import _defects, candidate_text_with_defects

    class EmptySymbols:
        def hasNext(self):
            return False

        def next(self):
            raise StopIteration

    function = SimpleNamespace(getParameterCount=lambda: 0)
    high = SimpleNamespace(
        getLocalSymbolMap=lambda: SimpleNamespace(getSymbols=lambda: EmptySymbols()),
        getFunctionPrototype=lambda: SimpleNamespace(getNumParams=lambda: 0),
    )
    assert _defects(function, "void fn(void) {}", high) == []

    unused = SimpleNamespace(getParameterCount=lambda: 4)
    unused_high = SimpleNamespace(
        getLocalSymbolMap=lambda: SimpleNamespace(getSymbols=lambda: EmptySymbols()),
        getFunctionPrototype=lambda: SimpleNamespace(getNumParams=lambda: 3),
    )
    defects = _defects(unused, "void fn(void)\n{\n  int in_stack_00000010;\n}\n", unused_high)
    kinds = {row["kind"] for row in defects}
    assert "parameter-count-mismatch" in kinds
    assert "phantom-stack-variable" in kinds
    text = candidate_text_with_defects("void fn() {}\n", defects)
    assert text is not None
    assert "// defect: parameter-count-mismatch:" in text
    assert "// defect: phantom-stack-variable: in_stack_00000010" in text
