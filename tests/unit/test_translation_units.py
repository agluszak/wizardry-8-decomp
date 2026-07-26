import csv
import io
from collections import Counter
from itertools import pairwise
from pathlib import Path

from wiz8decomp.reports.translation_units import (
    derive_intervals,
    render_gameplay_map_csv,
    render_interval_csv,
)


def _rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def test_tracked_translation_unit_map_is_current_and_non_overlapping() -> None:
    repository = Path(__file__).resolve().parents[2]
    assertions = _rows(repository / "config/analysis/wiz8/assertions.csv")
    gameplay = _rows(repository / "config/analysis/reccmp/wiz8-gameplay-boundaries.csv")
    intervals = derive_intervals(assertions)

    assert len(intervals) == 113
    assert all(left.upper < right.lower for left, right in pairwise(intervals))

    expected_intervals = render_interval_csv(intervals)
    tracked_intervals = (
        repository / "config/analysis/wiz8/translation-unit-intervals.csv"
    ).read_text(encoding="utf-8")
    assert tracked_intervals == expected_intervals

    expected_gameplay, counts = render_gameplay_map_csv(assertions, gameplay, intervals)
    tracked_gameplay = (
        repository / "config/analysis/wiz8/gameplay-translation-units.csv"
    ).read_text(encoding="utf-8")
    assert tracked_gameplay == expected_gameplay
    assert counts["direct"] + counts["inferred"] >= 25

    mapped = list(csv.DictReader(io.StringIO(tracked_gameplay)))
    assert Counter(row["attribution"] for row in mapped) == counts
    assert (
        next(row for row in mapped if row["symbol"] == "GetMonsterDataByID")["source_path"]
        == "Local Code\\MonsterManager.cpp"
    )
    assert (
        next(row for row in mapped if row["symbol"] == "GetRandomCharacter")["source_path"]
        == "Local Code\\UtilityFunctions.cpp"
    )
