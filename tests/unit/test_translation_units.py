import csv
import shutil
from itertools import pairwise
from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.reports.translation_units import (
    call_site_anchors,
    derive_intervals,
    translation_unit_report,
)


def _rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


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
    observations.mkdir(parents=True)
    shutil.copyfile(
        repository / "evidence/observations/wiz8/assertions.csv",
        observations / "assertions.csv",
    )
    (tmp_path / "build").mkdir()
    shutil.copyfile(repository / "build/source-index.json", tmp_path / "build/source-index.json")

    settings = SimpleNamespace(repo_dir=tmp_path, build_dir=tmp_path / "build")
    from wiz8decomp.ghidra import audits
    from wiz8decomp.source_model import build_source_model

    original = audits.function_inventory
    audits.function_inventory = lambda _settings: [
        {"entry": f"0x{address:08x}", "name": function.name}
        for address, function in build_source_model(tmp_path).functions.items()
    ]
    try:
        result = translation_unit_report(settings)
    finally:
        audits.function_inventory = original

    assert result["outputs"] == [
        "build/reports/translation-units/translation-unit-intervals.csv",
        "build/reports/translation-units/gameplay-translation-units.csv",
    ]
    assert all((tmp_path / path).is_file() for path in result["outputs"])
