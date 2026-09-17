from pathlib import Path

from wiz8decomp.reports.placement_outliers import (
    FIXTURE_ADDRESS,
    FIXTURE_SOURCE,
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


def test_placement_outliers_includes_notify_linked_model_shaped_fixture() -> None:
    """Regression shape for 0x005AA400 in stMeshModel.cpp: megabyte-scale gap, no fold mark."""

    source = FIXTURE_SOURCE
    markers = [
        _marker(0x00470040, source, name="NearLow"),
        _marker(0x00473260, source, name="NearMid"),
        _marker(0x004748C0, source, name="NearHigh"),
        _marker(FIXTURE_ADDRESS, source, name="NotifyLinkedModel005AA400"),
    ]
    rows = placement_outliers(markers, {source}, min_gap=0x100000, min_peers=3)
    assert len(rows) == 1
    assert rows[0]["address"] == f"0x{FIXTURE_ADDRESS:08x}"
    assert rows[0]["source_file"] == FIXTURE_SOURCE
    assert rows[0]["has_fold_or_emission_evidence"] is False


def test_placement_outlier_report_writes_artifact(tmp_path: Path) -> None:
    repository = Path(__file__).resolve().parents[2]
    (tmp_path / "build").mkdir()
    mesh = tmp_path / FIXTURE_SOURCE
    mesh.parent.mkdir(parents=True)
    mesh.write_text("// FUNCTION: WIZ8 0x005aa400\nvoid NotifyLinkedModel005AA400();\n")
    import shutil

    shutil.copyfile(
        repository / "src/wiz8/source_units.json",
        tmp_path / "src/wiz8/source_units.json",
    )
    shutil.copyfile(
        repository / "src/wiz8/sources.cmake",
        tmp_path / "src/wiz8/sources.cmake",
    )
    # Minimal index: three clustered functions plus the fixture outlier.
    index = {
        "schema": "reccmp-source-index-v3",
        "markers": [
            _marker(0x00470040, FIXTURE_SOURCE, name="NearLow"),
            _marker(0x00473260, FIXTURE_SOURCE, name="NearMid"),
            _marker(0x004748C0, FIXTURE_SOURCE, name="NearHigh"),
            _marker(FIXTURE_ADDRESS, FIXTURE_SOURCE, name="NotifyLinkedModel005AA400"),
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
    assert report["fixture_present"] is True
    assert (tmp_path / report["artifact"]).is_file()
