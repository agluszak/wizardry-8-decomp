import csv
from pathlib import Path

import yaml
from wiz8decomp.sgp_oracle import CoffFunction, classify_body


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

    assert len(functions) == 16
    assert {row["confidence"] for row in functions} == {"exact"}
    assert {row["owner"] for row in functions} == {"sgp-shared"}
    assert {row["source_path"] for row in functions} == {
        "sgp/DirectDraw Calls.c",
        "sgp/Random.c",
    }
    assert len(paths) == 7
    assert sum(row["classification"] == "exact-path" for row in paths) == 6
    assert sum(row["classification"] == "not-embedded" for row in paths) == 1


def test_random_unit_is_complete_and_consistent_across_builds() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/functions/wiz8-sgp.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        unit = [row for row in csv.DictReader(stream) if row["source_path"] == "sgp/Random.c"]
    with (repository / "config/analysis/sgp/random-cross-build.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        cross_build = list(csv.DictReader(stream))

    # Wizardry does not define JA2, so PRERANDOM_GENERATOR is off and the unit
    # compiles to exactly these three functions. All three are exact.
    assert [row["provisional_name"] for row in unit] == ["InitializeRandom", "Random", "Chance"]
    assert {row["authority"] for row in unit} == {"source-backed"}

    assert [row["function"] for row in cross_build] == [
        "InitializeRandom",
        "Random",
        "Chance",
    ]
    for row, mapping in zip(unit, cross_build, strict=True):
        assert mapping["gog_base"] == row["address"]
        assert row["size"] == mapping["size"]
        # The demo carries the whole unit at the same +0x360 shift as DirectDraw Calls.c.
        assert int(mapping["demo"], 16) - int(mapping["gog_base"], 16) == 0x360
        # The packed 1.28 patch executable and the protected retail build are
        # recorded as unavailable, never as absent.
        assert mapping["gog_128_patch"] == ""
        assert mapping["retail_2001_12_23"] == ""


def test_the_sgp_name_supersedes_the_cfagent_name_at_0x0040efa0() -> None:
    repository = Path(__file__).resolve().parents[2]
    rows = []
    for name in ("wiz8-sgp.csv", "wiz8-cfagent-oracle.csv"):
        path = repository / "config/analysis/functions" / name
        with path.open(newline="", encoding="utf-8") as stream:
            entries = [row for row in csv.DictReader(stream) if row["address"] == "0040efa0"]
        assert len(entries) == 1, name
        rows.extend(entries)

    for row in rows:
        assert row["provisional_name"] == "Random"
        assert row["aliases"] == "GetRandomNumber"
        assert set(row["name_origin"].split("|")) == {"sgp-source", "fan-patch-signature"}
        assert row["authority"] == "source-backed"


def test_sgp_harness_declares_the_full_flag_matrix_and_reviewed_builds() -> None:
    repository = Path(__file__).resolve().parents[2]
    harness = yaml.safe_load(
        (repository / "config/analysis/sgp/harness.yml").read_text(encoding="utf-8")
    )
    assert harness["schema"] == "wiz8.sgp-harness"
    assert {unit["id"] for unit in harness["units"]} == {"directdraw", "random"}
    assert set(harness["flag_axes"]) == {
        "optimization",
        "inlining",
        "processor",
        "runtime",
    }
    combinations = 1
    for values in harness["flag_axes"].values():
        combinations *= len(values)
    assert combinations == 16
    assert {build["id"] for build in harness["builds"]} >= {
        "demo",
        "gog_base",
        "gog_1261",
        "gog_128_base",
    }


def test_relocation_masked_matcher_uses_the_five_way_vocabulary() -> None:
    function = CoffFunction(
        name="Example",
        body=b"\x55\x8b\xec\xe8\0\0\0\0\xc3",
        relocation_offsets=(4,),
    )
    assert classify_body(function, function.body, near_threshold=0.75)["classification"] == "exact"
    relocated = b"\x55\x8b\xec\xe8\x12\x34\x56\x78\xc3"
    assert (
        classify_body(function, relocated, near_threshold=0.75)["classification"]
        == "relocation-equivalent"
    )
    assert (
        classify_body(function, relocated + b"padding" + relocated, near_threshold=0.75)[
            "classification"
        ]
        == "ambiguous-generic"
    )
    near = b"\x55\x8b\xec\xe8\x12\x34\x56\x78\x90"
    assert (
        classify_body(function, near, near_threshold=0.75)["classification"]
        == "near-source-with-wiz8-modifications"
    )
    assert (
        classify_body(function, b"\xcc" * len(function.body), near_threshold=0.75)[
            "classification"
        ]
        == "absent-or-stripped"
    )
