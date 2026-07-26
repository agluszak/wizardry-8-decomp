import csv
import io
import shutil
from collections import Counter
from itertools import pairwise
from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.reports.translation_units import (
    derive_intervals,
    render_gameplay_map_csv,
    render_interval_csv,
    translation_unit_report,
)


def _rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def test_translation_unit_map_is_current_and_non_overlapping() -> None:
    repository = Path(__file__).resolve().parents[2]
    assertions = _rows(repository / "config/analysis/wiz8/assertions.csv")
    gameplay = _rows(repository / "config/analysis/reccmp/wiz8-gameplay-boundaries.csv")
    intervals = derive_intervals(assertions)

    assert len(intervals) == 113
    assert all(left.upper < right.lower for left, right in pairwise(intervals))

    rendered_intervals = render_interval_csv(intervals)
    rendered_gameplay, counts = render_gameplay_map_csv(assertions, gameplay, intervals)
    assert counts["direct"] + counts["inferred"] >= 25

    mapped = list(csv.DictReader(io.StringIO(rendered_gameplay)))
    assert Counter(row["attribution"] for row in mapped) == counts
    assert len(list(csv.DictReader(io.StringIO(rendered_intervals)))) == 225
    assert (
        next(row for row in mapped if row["symbol"] == "GetMonsterDataByID")["source_path"]
        == "Local Code\\MonsterManager.cpp"
    )
    assert (
        next(row for row in mapped if row["symbol"] == "GetRandomCharacter")["source_path"]
        == "Local Code\\UtilityFunctions.cpp"
    )


def test_translation_unit_report_writes_generated_outputs_under_build(tmp_path: Path) -> None:
    repository = Path(__file__).resolve().parents[2]
    analysis = tmp_path / "config" / "analysis"
    (analysis / "wiz8").mkdir(parents=True)
    (analysis / "reccmp").mkdir(parents=True)
    shutil.copyfile(
        repository / "config/analysis/wiz8/assertions.csv",
        analysis / "wiz8/assertions.csv",
    )
    shutil.copyfile(
        repository / "config/analysis/reccmp/wiz8-gameplay-boundaries.csv",
        analysis / "reccmp/wiz8-gameplay-boundaries.csv",
    )

    settings = SimpleNamespace(repo_dir=tmp_path, build_dir=tmp_path / "build")
    result = translation_unit_report(settings)

    assert result["outputs"] == [
        "build/reports/translation-units/translation-unit-intervals.csv",
        "build/reports/translation-units/gameplay-translation-units.csv",
    ]
    assert all((tmp_path / path).is_file() for path in result["outputs"])
