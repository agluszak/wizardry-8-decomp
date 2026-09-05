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
from wiz8decomp.source_index import source_functions

REPOSITORY = Path(__file__).resolve().parents[2]
FUNCTION_MAPS = sorted((REPOSITORY / "evidence/reviewed").glob("*/functions.csv"))


def test_authority_is_the_strongest_ceiling_among_the_origins() -> None:
    assert derive_authority(("cosmic-forge", "fan-patch-signature")) == "external-semantic"
    assert derive_authority(("official-demo", "original-runtime-string")) == "string-backed"
    assert derive_authority(("fan-patch-signature", "sgp-source")) == "source-backed"


def test_official_builds_alone_cannot_name_anything() -> None:
    assert derive_authority(("official-demo",)) == "descriptive"
    assert derive_authority(("official-cross-build",)) == "descriptive"


def test_external_semantic_origins_cannot_be_promoted_by_themselves() -> None:
    with pytest.raises(ProvenanceError, match="not derivable"):
        validate_provenance("cosmic-forge", "source-backed")
    with pytest.raises(ProvenanceError, match="not derivable"):
        validate_provenance("fan-patch-signature|official-demo", "string-backed")


def test_unknown_and_malformed_tokens_are_rejected() -> None:
    with pytest.raises(ProvenanceError, match="unknown name_origin: guessed"):
        parse_name_origin("guessed")
    with pytest.raises(ProvenanceError, match="empty name_origin token"):
        parse_name_origin("sgp-source|")
    with pytest.raises(ProvenanceError, match="duplicate name_origin token"):
        parse_name_origin("sgp-source|sgp-source")
    with pytest.raises(ProvenanceError, match="cannot be combined"):
        parse_name_origin("descriptive|cosmic-forge")


def test_only_original_evidence_counts_as_original() -> None:
    assert is_original("source-backed")
    assert is_original("string-backed")
    assert not is_original("external-semantic")
    assert not is_original("descriptive")


def test_fid_seed_build_provenance_maps_onto_name_provenance() -> None:
    assert origin_for_fid_source_kind("precompiled-archive") == "original-export"
    assert derive_authority(("original-export",)) == "abi-backed"
    assert origin_for_fid_source_kind("cmake-object-library") == "original-source"
    assert derive_authority(("original-source",)) == "source-backed"


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
    assert rows
    assert len({row["address"] for row in rows}) == len(rows)
    for number, row in enumerate(rows, start=2):
        try:
            validate_provenance(row["name_origin"], row["authority"])
        except ProvenanceError as error:
            raise AssertionError(f"{path.name}:{number}: {error}") from error


def test_claims_exclude_source_owned_identities_and_type_layouts() -> None:
    source_addresses = set(source_functions(REPOSITORY))
    with (REPOSITORY / "evidence/reviewed/wiz8/claims.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        claims = list(csv.DictReader(stream))

    assert claims
    assert {row["entity_kind"] for row in claims} == {"function"}
    assert len({row["claim_id"] for row in claims}) == len(claims)
    assert not [
        row
        for row in claims
        if int(row["entity_key"], 16) in source_addresses
        and row["predicate"] in {"accepted-identity", "identity-provenance"}
    ]


def test_analysis_artifacts_are_not_stored_as_configuration() -> None:
    legacy = REPOSITORY / "config/analysis"
    assert not [path for path in legacy.rglob("*") if path.is_file()]
