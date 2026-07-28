from itertools import pairwise

from wiz8decomp.reports.data_segmentation import (
    attribute_globals,
    fit_storage_class,
    longest_non_decreasing,
    single_unit_baseline,
    unit_lookup,
)


class _Interval:
    def __init__(self, lower: int, upper: int, source_path: str) -> None:
        self.lower = lower
        self.upper = upper
        self.source_path = source_path


def test_longest_non_decreasing_keeps_order_consistent_majority() -> None:
    # Index 2 (the 9) fights the otherwise ascending order and is dropped.
    positions = longest_non_decreasing([1, 2, 9, 3, 3, 4])
    assert positions == [0, 1, 3, 4, 5]
    assert longest_non_decreasing([]) == []


def test_single_unit_baseline_requires_agreement() -> None:
    lookup = unit_lookup([_Interval(0x1000, 0x1FFF, "A.cpp"), _Interval(0x3000, 0x3FFF, "B.cpp")])
    references = [
        {"function_start": "00001100", "target": "00600000"},
        {"function_start": "00001200", "target": "00600000"},
        {"function_start": "00001100", "target": "00600010"},
        {"function_start": "00003100", "target": "00600010"},
        # A function in no interval attributes nothing.
        {"function_start": "00002500", "target": "00600020"},
        {"function_start": "", "target": "00600030"},
    ]
    baseline = single_unit_baseline(references, lookup)
    assert baseline == {0x00600000: "A.cpp"}


def test_fit_excludes_scattered_utility_units_and_bounds_the_rest() -> None:
    order = {"A.cpp": 0, "Utility.cpp": 1, "B.cpp": 2}
    # Utility.cpp's baseline globals are scattered through both other units'
    # runs, so the fit excludes it and the survivors bound clean intervals.
    baseline = sorted(
        [
            (0x100, "A.cpp"),
            (0x110, "A.cpp"),
            (0x120, "A.cpp"),
            (0x130, "Utility.cpp"),
            (0x200, "B.cpp"),
            (0x210, "Utility.cpp"),
            (0x220, "B.cpp"),
            (0x230, "Utility.cpp"),
            (0x240, "B.cpp"),
            (0x250, "Utility.cpp"),
            (0x260, "B.cpp"),
            (0x270, "B.cpp"),
        ]
    )
    fit = fit_storage_class(baseline, order)
    assert fit["excluded_units"] == ["Utility.cpp"]
    assert fit["intervals"] == [
        ("A.cpp", [0x100, 0x120]),
        ("B.cpp", [0x200, 0x270]),
    ]
    assert fit["dropped"] == 0


def test_attribution_scores_agreement_against_the_baseline() -> None:
    fits = {
        ".data/initialized": {"intervals": [("A.cpp", [0x100, 0x140]), ("B.cpp", [0x200, 0x240])]}
    }
    baseline = {0x110: "A.cpp", 0x210: "A.cpp"}
    globals_rows = [
        {"address": "00000110", "section": ".data", "storage": "initialized", "kind": "data"},
        # Inside B.cpp's interval but referenced only from A.cpp: a recorded
        # disagreement, not a silent correction.
        {"address": "00000210", "section": ".data", "storage": "initialized", "kind": "data"},
        {"address": "00000300", "section": ".data", "storage": "initialized", "kind": "data"},
        {
            "address": "00000400",
            "section": ".data",
            "storage": "initialized",
            "kind": "import-slot",
        },
    ]
    rows, counts = attribute_globals(globals_rows, fits, baseline)
    assert counts["attributed"] == 2
    assert counts["unattributed"] == 1
    assert counts["excluded-import-slot"] == 1
    assert counts["agree"] == 1
    assert counts["disagree"] == 1
    assert rows[0]["unit"] == "A.cpp"
    assert rows[1]["unit"] == "B.cpp"
    assert rows[1]["baseline_unit"] == "A.cpp"
    assert rows[2]["unit"] == ""


def test_unit_data_interval_snapshot_is_ordered_and_pins_the_octree_anchor() -> None:
    import csv
    from pathlib import Path

    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/snapshots/data-segmentation/unit-data-intervals.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert rows
    assert {row["program"] for row in rows} == {"wiz8--gog-base--wiz8--18a74ff61c65"}
    # Within one storage class the fitted intervals are disjoint and ordered.
    by_class: dict[str, list[tuple[int, int]]] = {}
    for row in rows:
        assert int(row["baseline_globals"]) > 0
        by_class.setdefault(row["storage_class"], []).append(
            (int(row["lower"], 16), int(row["upper"], 16))
        )
    assert set(by_class) == {".rdata/initialized", ".data/initialized", ".data/bss"}
    for spans in by_class.values():
        for (_, left_upper), (right_lower, _) in pairwise(spans):
            assert left_upper < right_lower

    # The hand-derived Octree.cpp anchor from the wiz8-d4o bead, byte-for-byte.
    octree = next(
        row
        for row in rows
        if row["unit"] == "Engine Code\\Octree.cpp" and row["storage_class"] == ".data/initialized"
    )
    assert (octree["lower"], octree["upper"]) == ("00605afc", "006066f0")
