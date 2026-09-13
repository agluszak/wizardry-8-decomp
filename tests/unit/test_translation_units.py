import csv
import json
import shutil
from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.ghidra.unit_intervals import TranslationUnitLayout
from wiz8decomp.reports.translation_units import (
    translation_unit_report,
)


def _rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def test_translation_unit_report_writes_generated_outputs_under_build(tmp_path: Path) -> None:
    repository = Path(__file__).resolve().parents[2]
    observations = tmp_path / "evidence" / "observations" / "wiz8"
    observations.mkdir(parents=True)
    shutil.copyfile(
        repository / "evidence/observations/wiz8/assertions.csv",
        observations / "assertions.csv",
    )
    (tmp_path / "src" / "wiz8").mkdir(parents=True)
    shutil.copyfile(
        repository / "src/wiz8/sources.cmake",
        tmp_path / "src/wiz8/sources.cmake",
    )
    shutil.copyfile(
        repository / "src/wiz8/source_units.json",
        tmp_path / "src/wiz8/source_units.json",
    )
    (tmp_path / "build").mkdir()
    shutil.copyfile(repository / "build/source-index.json", tmp_path / "build/source-index.json")

    settings = SimpleNamespace(repo_dir=tmp_path, build_dir=tmp_path / "build")
    from wiz8decomp.ghidra import query
    from wiz8decomp.source_index import source_functions

    original = query.function_inventory
    query.function_inventory = lambda _settings: [
        {"entry": f"0x{address:08x}", "name": function.name}
        for address, function in source_functions(tmp_path).items()
    ]
    try:
        result = translation_unit_report(settings)
    finally:
        query.function_inventory = original

    assert result["outputs"] == [
        "build/reports/translation-units/translation-unit-intervals.csv",
        "build/reports/translation-units/gameplay-translation-units.csv",
        "build/reports/translation-units/original-translation-units.csv",
        "build/reports/translation-units/misplaced-functions.csv",
    ]
    assert all((tmp_path / path).is_file() for path in result["outputs"])


def test_original_unit_statuses_and_misplaced_detection(tmp_path: Path) -> None:
    observations = tmp_path / "evidence" / "observations" / "wiz8"
    observations.mkdir(parents=True)
    (tmp_path / "build").mkdir()

    engine = tmp_path / "src" / "wiz8" / "engine_code"
    engine.mkdir(parents=True)
    (engine / "Recovered.cpp").write_text("// recovered\n", encoding="utf-8")
    (engine / "OtherUnit.cpp").write_text("// fragment\n", encoding="utf-8")
    (tmp_path / "src" / "wiz8" / "sources.cmake").write_text(
        "set(WIZ8_SOURCE_UNITS\n"
        "    src/wiz8/engine_code/Recovered.cpp\n"
        "    src/wiz8/engine_code/OtherUnit.cpp\n"
        ")\n",
        encoding="utf-8",
    )
    (tmp_path / "src" / "wiz8" / "source_units.json").write_text(
        json.dumps({"schema": "wiz8.source-units-v1"}), encoding="utf-8"
    )

    tree = observations / "source-tree.csv"
    with tree.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "relative_path",
                "subsystem",
                "canonical_absolute_path",
                "demo_absolute_path",
                "variants",
            ]
        )
        for name in ("Recovered", "Misplaced", "Absent", "Silent"):
            writer.writerow(
                [
                    f"Engine Code\\{name}.cpp",
                    "Engine Code",
                    f"C:\\Projects\\Wizardry 8\\Engine Code\\{name}.cpp",
                    "",
                    "gog-base",
                ]
            )
    assertions = observations / "assertions.csv"
    with assertions.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(
            [
                "call_site",
                "call_kind",
                "containing_function",
                "source_path",
                "line",
                "expression",
                "message",
            ]
        )
        for function, unit, line in (
            ("00401000", "Recovered", "10"),
            ("00401100", "Recovered", "20"),
            ("00402000", "Misplaced", "30"),
            ("00402100", "Misplaced", "40"),
            ("00403000", "Absent", "50"),
            ("00403100", "Absent", "60"),
        ):
            writer.writerow(
                [
                    f"00{function[2:]}",
                    "direct",
                    function,
                    f"C:\\Projects\\Wizardry 8\\Engine Code\\{unit}.cpp",
                    line,
                    "x",
                    "y",
                ]
            )

    from wiz8decomp import source_index
    from wiz8decomp.ghidra import query

    def marker(address: int, source_file: str, name: str) -> SimpleNamespace:
        return SimpleNamespace(
            name=name,
            marker_kind="FUNCTION",
            source_file=source_file,
        )

    original = source_index.source_functions
    original_inventory = query.function_inventory
    source_index.source_functions = lambda _repo, target="WIZ8": {
        0x401050: marker(0x401050, "src/wiz8/engine_code/Recovered.cpp", "RecoveredFn"),
        0x402050: marker(0x402050, "src/wiz8/engine_code/OtherUnit.cpp", "MisplacedFn"),
    }
    query.function_inventory = lambda _settings: [
        {"entry": "0x00401050", "name": "RecoveredFn"},
        {"entry": "0x00402050", "name": "MisplacedFn"},
    ]
    try:
        settings = SimpleNamespace(repo_dir=tmp_path, build_dir=tmp_path / "build")
        from wiz8decomp.ghidra.unit_intervals import assertion_anchors

        rows = list(csv.DictReader(assertions.open(newline="", encoding="utf-8")))
        units, headers = assertion_anchors(rows)
        result = translation_unit_report(
            settings, layout=TranslationUnitLayout(units, header_anchors=headers)
        )
    finally:
        source_index.source_functions = original
        query.function_inventory = original_inventory

    by_path = {
        row["original_path"]: row
        for row in _rows(
            tmp_path / "build/reports/translation-units/original-translation-units.csv"
        )
    }
    assert by_path["Engine Code\\Recovered.cpp"]["status"] == "recovered-original-tu"
    assert by_path["Engine Code\\Recovered.cpp"]["recovered_in_unit"] == "1"
    assert by_path["Engine Code\\Recovered.cpp"]["recovered_anchored"] == "0"
    assert by_path["Engine Code\\Misplaced.cpp"]["status"] == "partially-represented"
    assert by_path["Engine Code\\Absent.cpp"]["status"] == "absent"
    assert by_path["Engine Code\\Absent.cpp"]["unrecovered"] == "0"
    assert by_path["Engine Code\\Silent.cpp"]["status"] == "evidence-insufficient"
    assert by_path["Engine Code\\Recovered.cpp"]["rank"] == ""

    misplaced = _rows(tmp_path / "build/reports/translation-units/misplaced-functions.csv")
    assert [row["address"] for row in misplaced] == ["00402050"]
    assert misplaced[0]["owner_original_path"] == "Engine Code\\Misplaced.cpp"
    assert misplaced[0]["current_source_path"] == "src/wiz8/engine_code/OtherUnit.cpp"
    assert misplaced[0]["current_class"] == "unresolved-fragment"
    assert result["misplaced_functions"] == 1
    assert result["original_units"] == {
        "recovered-original-tu": 1,
        "partially-represented": 1,
        "absent": 1,
        "evidence-insufficient": 1,
    }
