import csv
import io
import shutil
from collections import Counter
from itertools import pairwise
from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.reports.translation_units import (
    call_site_anchors,
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
    assertions = _rows(repository / "evidence/observations/wiz8/assertions.csv")
    gameplay = _rows(repository / "config/reccmp/wiz8-gameplay-boundaries.csv")
    intervals = derive_intervals(assertions)

    assert len(intervals) == 117
    assert all(left.upper < right.lower for left, right in pairwise(intervals))

    rendered_intervals = render_interval_csv(intervals)
    rendered_gameplay, counts = render_gameplay_map_csv(assertions, gameplay, intervals)
    assert counts["direct"] + counts["inferred"] >= 25
    assert counts["external"] == 2

    mapped = list(csv.DictReader(io.StringIO(rendered_gameplay)))
    assert Counter(row["attribution"] for row in mapped) == counts
    external = [row for row in mapped if row["attribution"] == "external"]
    assert {row["symbol"] for row in external} == {
        "method_00421680",
        "method_00446110",
    }
    assert all(not row["source_path"] for row in external)
    assert all(not row["interval_lower"] and not row["interval_upper"] for row in external)
    assert len(list(csv.DictReader(io.StringIO(rendered_intervals)))) == 233
    assert (
        next(row for row in mapped if row["symbol"] == "MonsterDBFromSpecies")["source_path"]
        == "Local Code\\MonsterManager.cpp"
    )
    assert (
        next(row for row in mapped if row["symbol"] == "GetRandomCharacter")["source_path"]
        == "Local Code\\UtilityFunctions.cpp"
    )


def test_call_site_anchors_agree_with_the_reviewed_table_where_both_know_a_function() -> None:
    """The two sources resolve the enclosing function independently.

    The reviewed table takes it from Ghidra; the snapshot derives it from
    inter-function padding. A disagreement means one of them is wrong, so this
    is the check that lets the derived anchors be trusted as `direct`.
    """
    repository = Path(__file__).resolve().parents[2]
    assertions = _rows(repository / "evidence/observations/wiz8/assertions.csv")
    snapshot = _rows(repository / "evidence/snapshots/call-sites/assertions.csv")
    program = next(row["program"] for row in snapshot if "--gog-base--" in row["program"])

    derived = call_site_anchors(snapshot, program)
    reviewed = {
        int(row["containing_function"], 16): row["source_path"][len("C:\\Projects\\Wizardry 8\\") :]
        for row in assertions
        if row["containing_function"]
        and row["source_path"].startswith("C:\\Projects\\Wizardry 8\\")
        and row["source_path"].casefold().endswith(".cpp")
    }

    shared = set(derived) & set(reviewed)
    assert len(shared) > 300
    assert {anchor for anchor in shared if derived[anchor] != reviewed[anchor]} == set()


def test_snapshot_anchors_extend_the_interval_map_without_overlapping() -> None:
    repository = Path(__file__).resolve().parents[2]
    assertions = _rows(repository / "evidence/observations/wiz8/assertions.csv")
    snapshot = _rows(repository / "evidence/snapshots/call-sites/assertions.csv")
    program = next(row["program"] for row in snapshot if "--gog-base--" in row["program"])

    merged = derive_intervals(assertions, call_site_anchors(snapshot, program))

    assert len(merged) > len(derive_intervals(assertions))
    assert all(left.upper < right.lower for left, right in pairwise(merged))


def test_translation_unit_report_writes_generated_outputs_under_build(tmp_path: Path) -> None:
    repository = Path(__file__).resolve().parents[2]
    observations = tmp_path / "evidence" / "observations" / "wiz8"
    reccmp = tmp_path / "config" / "reccmp"
    observations.mkdir(parents=True)
    reccmp.mkdir(parents=True)
    shutil.copyfile(
        repository / "evidence/observations/wiz8/assertions.csv",
        observations / "assertions.csv",
    )
    shutil.copyfile(
        repository / "config/reccmp/wiz8-gameplay-boundaries.csv",
        reccmp / "wiz8-gameplay-boundaries.csv",
    )

    settings = SimpleNamespace(repo_dir=tmp_path, build_dir=tmp_path / "build")
    result = translation_unit_report(settings)

    assert result["outputs"] == [
        "build/reports/translation-units/translation-unit-intervals.csv",
        "build/reports/translation-units/gameplay-translation-units.csv",
    ]
    assert all((tmp_path / path).is_file() for path in result["outputs"])
