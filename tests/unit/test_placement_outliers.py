from pathlib import Path

from wiz8decomp.reports.placement_outliers import (
    placement_outlier_report,
    placement_outliers,
)


def _marker(
    address: int,
    source: str,
    *,
    kind: str = "FUNCTION",
    folded: bool = False,
    name: str = "",
) -> dict:
    return {
        "address": address,
        "marker_kind": kind,
        "source_file": source,
        "folded": folded,
        "marker_name": name,
        "declaration_key": ["WIZ8", name] if name else None,
    }


def test_placement_outliers_flags_large_gap_without_assuming_misownership() -> None:
    source = "src/wiz8/engine_code/Example.cpp"
    markers = [
        _marker(0x00470000, source, name="NearA"),
        _marker(0x00470100, source, name="NearB"),
        _marker(0x00470200, source, name="NearC"),
        _marker(0x005AA400, source, name="FarEmpty"),
        _marker(0x005AA400, source, kind="SYNTHETIC", name="FarEmptyDeleting"),
    ]
    rows = placement_outliers(markers, {source}, min_gap=0x100000, min_peers=3)
    assert len(rows) == 1
    row = rows[0]
    assert row["address"] == "0x005aa400"
    assert row["name"] == "FarEmpty"
    assert row["synthetic"] is True
    assert row["folded"] is False
    assert row["has_fold_or_emission_evidence"] is True


def test_placement_outliers_annotates_folded_without_template() -> None:
    source = "src/wiz8/local_screens/Example.cpp"
    markers = [
        _marker(0x005A0000, source, name="A"),
        _marker(0x005A0100, source, name="B"),
        _marker(0x005A0200, source, name="C"),
        _marker(0x00400000, source, name="FoldedAlias", folded=True),
    ]
    rows = placement_outliers(markers, {source}, min_gap=0x100000, min_peers=3)
    assert len(rows) == 1
    assert rows[0]["folded"] is True
    assert rows[0]["template"] is False
    assert rows[0]["has_fold_or_emission_evidence"] is True


def test_placement_outliers_flags_megabyte_gap_without_emission_evidence() -> None:
    """Synthetic regression: large gap with no FOLDED/TEMPLATE/SYNTHETIC peers."""

    source = "src/wiz8/engine_code/SyntheticOutlier.cpp"
    markers = [
        _marker(0x00470040, source, name="NearLow"),
        _marker(0x00473260, source, name="NearMid"),
        _marker(0x004748C0, source, name="NearHigh"),
        _marker(0x005AA400, source, name="FarBody"),
    ]
    rows = placement_outliers(markers, {source}, min_gap=0x100000, min_peers=3)
    assert len(rows) == 1
    assert rows[0]["address"] == "0x005aa400"
    assert rows[0]["source_file"] == source
    assert rows[0]["has_fold_or_emission_evidence"] is False


def test_placement_outlier_report_writes_artifact(tmp_path: Path) -> None:
    repository = Path(__file__).resolve().parents[2]
    (tmp_path / "build").mkdir()
    # Reuse a real ORIGINAL_TU path so source_unit_records classifies it; the
    # markers themselves are synthetic and do not bind production policy.
    tu_source = "src/wiz8/engine_code/stMeshModel.cpp"
    tu_file = tmp_path / tu_source
    tu_file.parent.mkdir(parents=True)
    tu_file.write_text("// FUNCTION: WIZ8 0x005aa400\nvoid FarBody();\n")
    import shutil

    shutil.copyfile(
        repository / "src/wiz8/source_units.json",
        tmp_path / "src/wiz8/source_units.json",
    )
    shutil.copyfile(
        repository / "src/wiz8/sources.cmake",
        tmp_path / "src/wiz8/sources.cmake",
    )
    index = {
        "schema": "reccmp-source-index-v3",
        "markers": [
            _marker(0x00470040, tu_source, name="NearLow"),
            _marker(0x00473260, tu_source, name="NearMid"),
            _marker(0x004748C0, tu_source, name="NearHigh"),
            _marker(0x005AA400, tu_source, name="FarBody"),
        ],
        "declarations": [],
    }
    import json

    (tmp_path / "build" / "source-index.json").write_text(json.dumps(index) + "\n")
    observations = tmp_path / "evidence" / "observations" / "wiz8"
    observations.mkdir(parents=True)
    tree = repository / "evidence/observations/wiz8/source-tree.csv"
    if tree.is_file():
        shutil.copyfile(tree, observations / "source-tree.csv")

    report = placement_outlier_report(tmp_path, min_gap=0x100000, min_peers=3)
    assert report["informational"] is True
    assert "fixture_present" not in report
    assert report["outlier_count"] == 1
    assert report["outliers"][0]["name"] == "FarBody"
    assert (tmp_path / report["artifact"]).is_file()
