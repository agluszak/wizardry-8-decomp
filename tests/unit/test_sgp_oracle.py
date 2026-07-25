import csv
from pathlib import Path

import yaml


def test_sgp_oracle_tracks_the_complete_reviewed_census() -> None:
    repository = Path(__file__).resolve().parents[2]
    oracle = yaml.safe_load(
        (repository / "config/analysis/sgp/source-oracle.yml").read_text(encoding="utf-8")
    )

    assert oracle["revision"] == "5ac0a9d56d27e8a7e2c4a7b48ed8932ae7f64033"
    assert oracle["license"]["policy"] == "oracle-only"
    assert len(oracle["active_wiz8_pch_sources"]) == 33
    assert len(oracle["commented_wiz8_pch_sources"]) == 1
    assert len(oracle["additional_wizardry_evidence"]) == 11


def test_sgp_maps_keep_exact_and_absent_evidence_distinct() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/functions/wiz8-sgp.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        functions = list(csv.DictReader(stream))
    with (repository / "config/analysis/sgp/wiz8-source-paths.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        paths = list(csv.DictReader(stream))

    assert len(functions) == 13
    assert {row["confidence"] for row in functions} == {"exact"}
    assert {row["owner"] for row in functions} == {"sgp-shared"}
    assert len(paths) == 7
    assert sum(row["classification"] == "exact-path" for row in paths) == 6
    assert sum(row["classification"] == "not-embedded" for row in paths) == 1
