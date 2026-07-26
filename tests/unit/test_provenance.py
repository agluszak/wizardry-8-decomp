import csv
from pathlib import Path

import pytest
from wiz8decomp.provenance import (
    NAME_ORIGIN_CEILING,
    ProvenanceError,
    derive_authority,
    is_original,
    origin_for_fid_source_kind,
    parse_name_origin,
    validate_provenance,
)

REPOSITORY = Path(__file__).resolve().parents[2]
FUNCTION_MAPS = sorted((REPOSITORY / "config/analysis/functions").glob("*.csv"))


def test_authority_is_the_strongest_ceiling_among_the_origins() -> None:
    assert derive_authority(("cosmic-forge", "fan-patch-signature")) == "external-semantic"
    assert derive_authority(("official-demo", "original-runtime-string")) == "string-backed"
    assert derive_authority(("fan-patch-signature", "sgp-source")) == "source-backed"


def test_official_builds_alone_cannot_name_anything() -> None:
    # The demo and retail are boundary oracles: they locate code, they do not name it.
    assert derive_authority(("official-demo",)) == "descriptive"
    assert derive_authority(("official-cross-build",)) == "descriptive"


def test_external_semantic_origins_cannot_be_promoted_by_themselves() -> None:
    with pytest.raises(ProvenanceError, match="not derivable"):
        validate_provenance("cosmic-forge", "source-backed")
    with pytest.raises(ProvenanceError, match="not derivable"):
        validate_provenance("fan-patch-signature|official-demo", "string-backed")


def test_under_claiming_is_expressed_by_dropping_an_origin_not_by_weakening_authority() -> None:
    with pytest.raises(ProvenanceError, match="not derivable"):
        validate_provenance("sgp-source", "descriptive")
    assert validate_provenance("descriptive", "descriptive") == (("descriptive",), "descriptive")


def test_unknown_and_malformed_tokens_are_rejected() -> None:
    with pytest.raises(ProvenanceError, match="unknown name_origin: guessed"):
        parse_name_origin("guessed")
    with pytest.raises(ProvenanceError, match="empty name_origin token"):
        parse_name_origin("sgp-source|")
    with pytest.raises(ProvenanceError, match="duplicate name_origin token"):
        parse_name_origin("sgp-source|sgp-source")
    with pytest.raises(ProvenanceError, match="cannot be combined"):
        parse_name_origin("descriptive|cosmic-forge")
    with pytest.raises(ProvenanceError, match="unknown authority"):
        validate_provenance("descriptive", "medium")


def test_only_original_evidence_counts_as_original() -> None:
    assert is_original("source-backed")
    assert is_original("string-backed")
    assert not is_original("external-semantic")
    assert not is_original("descriptive")


def test_fid_seed_build_provenance_maps_onto_name_provenance() -> None:
    # A pinned library archive carries the original COFF symbol table; a source-built
    # object carries names the pinned source declares. Both are original evidence.
    assert origin_for_fid_source_kind("precompiled-archive") == "original-export"
    assert derive_authority(("original-export",)) == "abi-backed"
    assert origin_for_fid_source_kind("cmake-object-library") == "original-source"
    assert derive_authority(("original-source",)) == "source-backed"


def test_a_fid_match_without_seed_provenance_claims_nothing() -> None:
    # The srs database has no recorded seed provenance, so its names must not
    # inherit the authority of the database they happen to live in.
    assert origin_for_fid_source_kind(None) == "descriptive"
    with pytest.raises(ProvenanceError, match="unknown FID seed source_kind"):
        origin_for_fid_source_kind("hand-written")


def test_every_origin_token_has_a_ceiling() -> None:
    assert set(NAME_ORIGIN_CEILING) == {
        "original-source",
        "sgp-source",
        "original-export",
        "original-runtime-string",
        "original-source-path",
        "official-demo",
        "official-cross-build",
        "fan-patch-signature",
        "cosmic-forge",
        "descriptive",
    }


@pytest.mark.parametrize("path", FUNCTION_MAPS, ids=lambda path: path.name)
def test_reviewed_function_maps_carry_valid_provenance(path: Path) -> None:
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))

    assert rows, f"{path.name} is empty"
    for number, row in enumerate(rows, start=2):
        assert "name_origin" in row and "authority" in row, f"{path.name}:{number} lacks provenance"
        try:
            validate_provenance(row["name_origin"], row["authority"])
        except ProvenanceError as error:
            raise AssertionError(f"{path.name}:{number}: {error}") from error


def test_cfagent_names_remain_external_semantic_until_corroborated() -> None:
    path = REPOSITORY / "config/analysis/functions/wiz8-cfagent-oracle.csv"
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 47
    unpromoted = [row for row in rows if row["name_origin"] == "fan-patch-signature"]
    promoted = [row for row in rows if row["name_origin"] != "fan-patch-signature"]

    assert len(unpromoted) == 46
    assert {row["authority"] for row in unpromoted} == {"external-semantic"}

    # The only promotion so far: the SGP Random.c compile named 0x0040EFA0.
    assert [row["address"] for row in promoted] == ["0040efa0"]
    assert promoted[0]["authority"] == "source-backed"
    assert "sgp-source" in promoted[0]["name_origin"].split("|")


def test_only_sgp_and_upstream_source_matches_are_source_backed() -> None:
    owners: set[str] = set()
    for path in FUNCTION_MAPS:
        with path.open(newline="", encoding="utf-8") as stream:
            for row in csv.DictReader(stream):
                if row["authority"] == "source-backed":
                    owners.add(row["owner"])

    assert owners == {
        "ijg-jpeg-6",
        "infozip-unzip-5.4",
        "infozip-zcrypt-2.8",
        "sgp-compression",
        "sgp-shared",
        "zlib-1.0.4",
    }


def test_the_demo_supplies_names_only_through_retained_diagnostics() -> None:
    path = REPOSITORY / "config/analysis/functions/wiz8-formats.csv"
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))

    demo_named = [row for row in rows if "official-demo" in row["name_origin"].split("|")]

    assert len(demo_named) == 14
    for row in demo_named:
        origins = set(row["name_origin"].split("|"))
        assert "original-runtime-string" in origins
        assert row["authority"] == "string-backed"
