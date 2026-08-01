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

    def fake_export(settings, selections, *, program_selector, class_name, unit, data, output):
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

    def fake_export(settings, selections, *, program_selector, class_name, unit, data, output):
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

    def fake_export(settings, selections, *, program_selector, class_name, unit, data, output):
        assert selections == []
        assert class_name == "W8GrCycle"
        return {"program": "wiz8", "functions": [], "text": "class W8GrCycle {\n};\n"}

    monkeypatch.setattr("wiz8decomp.ghidra.export_cpp.export_cpp", fake_export)
    result = CliRunner().invoke(app, ["ghidra", "export-cpp", "--class", "W8GrCycle"])
    assert result.exit_code == 0
    assert result.stdout == "class W8GrCycle {\n};\n"


def test_export_cpp_rejects_mixed_or_empty_selection() -> None:
    from wiz8decomp.ghidra.export_cpp import export_cpp

    with pytest.raises(ValueError, match="exactly one"):
        export_cpp(object(), ["0x004a5e50"], class_name="W8GrCycle")
    with pytest.raises(ValueError, match="exactly one"):
        export_cpp(object(), ["0x004a5e50"], unit="src/wiz8/engine_code/GrCycle.cpp")
    with pytest.raises(ValueError, match="exactly one"):
        export_cpp(object(), [])
    with pytest.raises(ValueError, match="exactly one"):
        export_cpp(object(), [], data=True)


def test_assemble_unit_reconstructs_marker_blocks_in_file_order() -> None:
    from wiz8decomp.ghidra.export_cpp import assemble_unit

    markers = [
        {"address": 0x20, "marker_kind": "FUNCTION", "line": 5},
        {
            "address": 0x30,
            "marker_kind": "SYNTHETIC",
            "line": 9,
            "marker_name": "W8GrCycle::`scalar deleting destructor'",
        },
        {
            "address": 0x40,
            "marker_kind": "TEMPLATE",
            "line": 12,
            "marker_name": "W8GrowableVector<W8GrCycle*>::~W8GrowableVector<W8GrCycle*>",
        },
        {"address": 0x50, "marker_kind": "LIBRARY", "line": 15},
    ]
    blocks = {0x20: "// FUNCTION: WIZ8 0x00000020\nvoid f()\n{\n}\n"}
    text = assemble_unit(markers, blocks)
    assert text == (
        "// FUNCTION: WIZ8 0x00000020\nvoid f()\n{\n}\n"
        "\n// SYNTHETIC: WIZ8 0x00000030\n// W8GrCycle::`scalar deleting destructor'\n"
        "\n// TEMPLATE: WIZ8 0x00000040\n"
        "// W8GrowableVector<W8GrCycle*>::~W8GrowableVector<W8GrCycle*>\n"
        "\n// LIBRARY: WIZ8 0x00000050\n"
    )


def test_assemble_unit_reports_a_missing_function_export() -> None:
    from wiz8decomp.ghidra.export_cpp import assemble_unit

    text = assemble_unit([{"address": 0x20, "marker_kind": "FUNCTION", "line": 1}], {})
    assert text == "// error: no export for 0x00000020\n"


def test_data_selections_reject_ranges() -> None:
    from wiz8decomp.ghidra.export_cpp import _resolve_data_addresses

    assert _resolve_data_addresses(["0x10", "0x0065be2c"]) == [0x10, 0x65BE2C]
    with pytest.raises(ValueError, match="plain addresses"):
        _resolve_data_addresses(["0x10:0x20"])
