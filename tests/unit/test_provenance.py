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
FUNCTION_MAPS = sorted((REPOSITORY / "evidence/reviewed").glob("*/functions.csv"))
FUNCTION_MAPS.append(REPOSITORY / "evidence/reviewed/wiz8/function-provenance.csv")


def _wiz8_rows() -> list[dict[str, str]]:
    with (REPOSITORY / "evidence/reviewed/wiz8/function-provenance.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        return list(csv.DictReader(stream))


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
    assert len({row["address"] for row in rows}) == len(rows)
    assert {row["program"] for row in rows} == {path.parent.name}
    for number, row in enumerate(rows, start=2):
        assert "name_origin" in row and "authority" in row, f"{path.name}:{number} lacks provenance"
        try:
            validate_provenance(row["name_origin"], row["authority"])
        except ProvenanceError as error:
            raise AssertionError(f"{path.name}:{number}: {error}") from error


def test_wiz8_claims_are_many_to_one_without_duplicate_identities() -> None:
    functions = _wiz8_rows()
    with (REPOSITORY / "evidence/reviewed/wiz8/claims.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        evidence = list(csv.DictReader(stream))

    assert len({row["address"] for row in functions}) == len(functions)
    assert {
        "005e22c0",
        "005e2370",
        "005e23e0",
        "005e2440",
        "005e2480",
        "005e2530",
        "005e26b0",
        "005e26e0",
        "005e27c0",
        "005e2870",
        "005e2890",
        "005e2900",
        "005e29a0",
        "005e2a00",
        "005e2a60",
        "005e2aa0",
        "005e2b50",
        "005e2b80",
        "005e2c70",
        "005e2c80",
        "005e2cc0",
    } <= {row["address"] for row in functions}
    assert len({row["claim_id"] for row in evidence}) == len(evidence)
    assert {row["entity_kind"] for row in evidence} == {"function", "type"}
    assert all(
        row["entity_key"].startswith("/wiz8/classes/")
        for row in evidence
        if row["entity_kind"] == "type"
    )
    by_address = {row["address"] for row in functions}
    assert {row["entity_key"] for row in evidence if row["entity_kind"] == "function"} <= by_address
    assert {row["origin"] for row in evidence if row["entity_key"] == "0040efa0"} == {
        "cfagent-oracle",
        "sgp",
        "fan-patch-signature|sgp-source",
    }


def test_analysis_artifacts_are_not_stored_as_configuration() -> None:
    legacy = REPOSITORY / "config" / "analysis"
    assert not [path for path in legacy.rglob("*") if path.is_file()]


def test_cfagent_names_remain_external_semantic_until_corroborated() -> None:
    rows = [row for row in _wiz8_rows() if "fan-patch-signature" in row["name_origin"].split("|")]

    assert len(rows) == 45
    unpromoted = [row for row in rows if row["name_origin"] == "fan-patch-signature"]
    promoted = [row for row in rows if row["name_origin"] != "fan-patch-signature"]

    assert len(unpromoted) == 42
    assert {row["authority"] for row in unpromoted} == {"external-semantic"}

    # Random.c supplies the original SGP name, the official demo retains
    # MonsterDBFromSpecies diagnostics for the canonical retail counterpart, and
    # PointCastSpell names itself in the error text its own body embeds.
    assert [row["address"] for row in promoted] == ["0040efa0", "004e57c0", "004fb220"]
    assert promoted[0]["authority"] == "source-backed"
    assert "sgp-source" in promoted[0]["name_origin"].split("|")
    assert promoted[1]["authority"] == "string-backed"
    assert promoted[1]["aliases"] == "GetMonsterDataByID"
    assert promoted[2]["authority"] == "string-backed"
    assert promoted[2]["aliases"] == "CastSpellAtParty"

    # CFAgent's descriptive seeds are retained only as aliases once SR.DLL's
    # own exports establish the vendor template owner and base type name.
    by_address = {row["address"]: row for row in _wiz8_rows()}
    assert {
        address: (
            by_address[address]["owner"],
            by_address[address]["name_origin"],
            by_address[address]["authority"],
            by_address[address]["aliases"],
        )
        for address in ("00421680", "00446110")
    } == {
        "00421680": (
            "surrender-template",
            "original-export",
            "abi-backed",
            "VectorFromThreeFloats",
        ),
        "00446110": (
            "surrender-template",
            "original-export",
            "abi-backed",
            "Copy3DVector",
        ),
    }


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
    demo_named = [row for row in _wiz8_rows() if "official-demo" in row["name_origin"].split("|")]

    assert len(demo_named) == 15
    for row in demo_named:
        origins = set(row["name_origin"].split("|"))
        assert "original-runtime-string" in origins
        assert row["authority"] == "string-backed"
