import csv
import shutil
from pathlib import Path
from types import SimpleNamespace

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
    ]
    assert all((tmp_path / path).is_file() for path in result["outputs"])
