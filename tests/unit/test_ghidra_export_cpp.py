"""Unit tests for the export-cpp selection parsing, jar build, and CLI shape.

The Java extension itself needs a JVM and the live project, so these tests
cover the Python surface only: everything here runs with fakes.
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest
from typer.testing import CliRunner
from wiz8decomp import command_support
from wiz8decomp.cli import app
from wiz8decomp.config import Settings
from wiz8decomp.ghidra import export_cpp as export_cpp_module
from wiz8decomp.ghidra.export_cpp import ensure_exporter_jar, parse_selection


def test_parse_selection_accepts_addresses_and_ranges() -> None:
    assert parse_selection("0x004a5e50") == (0x4A5E50, None)
    assert parse_selection("4882000") == (4882000, None)
    assert parse_selection("0x004a5e50:0x004a6610") == (0x4A5E50, 0x4A6610)
    assert parse_selection(" 0x10:0x10 ") == (0x10, 0x10)


@pytest.mark.parametrize("text", ["", "0x20:0x10", "nonsense", "0x10:", "-0x10"])
def test_parse_selection_rejects_malformed_input(text: str) -> None:
    with pytest.raises(ValueError):
        parse_selection(text)


def _settings(tmp_path: Path) -> Settings:
    install = tmp_path / "ghidra-install"
    (install / "Ghidra" / "Features" / "Decompiler" / "lib").mkdir(parents=True)
    (install / "Ghidra" / "Features" / "Decompiler" / "lib" / "Decompiler.jar").write_bytes(b"")
    repo = tmp_path / "repo"
    source_dir = repo / "tools" / "ghidra-extension" / "src" / "wiz8" / "exporter"
    source_dir.mkdir(parents=True)
    (source_dir / "Wiz8RecoveryExporter.java").write_text("class A {}\n", encoding="utf-8")
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": str(install),
            "WIZ8_INPUT_DIR": str(tmp_path / "inputs"),
            "WIZ8_WORK_DIR": str(tmp_path / "work"),
            "repo_dir": repo,
        }
    )


def _fake_compiler(monkeypatch: pytest.MonkeyPatch) -> list[list[str]]:
    invocations: list[list[str]] = []

    def fake_run(argv, *, cwd, log_path=None, env=None, check=True):
        invocations.append([str(part) for part in argv])

    monkeypatch.setattr(export_cpp_module, "_find_javac", lambda: Path("/fake/javac"))
    monkeypatch.setattr(export_cpp_module, "_check_javac_version", lambda settings, javac: None)
    monkeypatch.setattr(export_cpp_module.subprocesses, "run", fake_run)
    return invocations


def test_jar_build_is_skipped_while_the_stamp_matches(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _settings(tmp_path)
    invocations = _fake_compiler(monkeypatch)

    jar = ensure_exporter_jar(settings)
    assert jar.is_file()
    assert len(invocations) == 1
    stamp = json.loads((jar.parent / "stamp.json").read_text(encoding="utf-8"))
    assert stamp["ghidra_install_dir"] == str(settings.ghidra_install_dir)

    assert ensure_exporter_jar(settings) == jar
    assert len(invocations) == 1


def test_jar_rebuilds_when_a_source_changes(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _settings(tmp_path)
    invocations = _fake_compiler(monkeypatch)
    ensure_exporter_jar(settings)

    source = next(export_cpp_module.source_directory(settings).rglob("*.java"))
    source.write_text("class A { int x; }\n", encoding="utf-8")
    ensure_exporter_jar(settings)
    assert len(invocations) == 2


def test_missing_javac_names_the_jdk_requirement(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _settings(tmp_path)
    monkeypatch.delenv("JAVA_HOME", raising=False)
    monkeypatch.setattr(export_cpp_module.shutil, "which", lambda name: None)

    with pytest.raises(RuntimeError, match="JDK 21"):
        ensure_exporter_jar(settings)


def test_export_cpp_command_streams_raw_text_to_stdout(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(command_support, "settings", lambda: object())

    def fake_export(settings, selections, *, program_selector, class_name, output):
        assert selections == ["0x004a5e50"]
        assert program_selector == "wiz8"
        assert class_name is None
        assert output is None
        return {"program": "wiz8", "functions": [], "text": "// FUNCTION: WIZ8 0x004a5e50\n"}

    monkeypatch.setattr(
        "wiz8decomp.ghidra.export_cpp.export_cpp",
        fake_export,
    )
    result = CliRunner().invoke(app, ["ghidra", "export-cpp", "0x004a5e50"])
    assert result.exit_code == 0
    assert result.stdout == "// FUNCTION: WIZ8 0x004a5e50\n"


def test_export_cpp_command_reports_a_summary_for_file_output(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(command_support, "settings", lambda: object())
    target = tmp_path / "out.cpp"

    def fake_export(settings, selections, *, program_selector, class_name, output):
        assert output == target
        return {
            "program": "wiz8",
            "functions": [{"entry": "0x004a5e50", "kind": "constructor"}],
            "text": "ignored",
            "outputs": [str(target)],
        }

    monkeypatch.setattr("wiz8decomp.ghidra.export_cpp.export_cpp", fake_export)
    result = CliRunner().invoke(
        app,
        ["--json", "ghidra", "export-cpp", "0x004a5e50", "--output", str(target)],
    )
    assert result.exit_code == 0
    summary = json.loads(result.stdout)
    assert summary["outputs"] == [str(target)]
    assert "text" not in summary


def test_export_cpp_command_passes_the_class_selector(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    monkeypatch.setattr(command_support, "settings", lambda: object())

    def fake_export(settings, selections, *, program_selector, class_name, output):
        assert selections == []
        assert class_name == "W8GrCycle"
        return {"program": "wiz8", "functions": [], "text": "class W8GrCycle {\n};\n"}

    monkeypatch.setattr("wiz8decomp.ghidra.export_cpp.export_cpp", fake_export)
    result = CliRunner().invoke(app, ["ghidra", "export-cpp", "--class", "W8GrCycle"])
    assert result.exit_code == 0
    assert result.stdout == "class W8GrCycle {\n};\n"


def test_export_cpp_rejects_mixed_class_and_address_selection() -> None:
    from wiz8decomp.ghidra.export_cpp import export_cpp

    with pytest.raises(ValueError, match="not both"):
        export_cpp(object(), ["0x004a5e50"], class_name="W8GrCycle")
    with pytest.raises(ValueError, match="at least one"):
        export_cpp(object(), [])
