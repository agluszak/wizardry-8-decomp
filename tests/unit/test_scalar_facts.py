"""Contracts for shared facts and report-only, evidence-seeded type recovery."""

from __future__ import annotations

import hashlib
import importlib
import json
import re
import subprocess
import sys
from pathlib import Path

import pytest

PLUGIN = Path(__file__).resolve().parents[2] / "docker/msvc600/clang-tidy-plugin"


@pytest.fixture
def scalar(monkeypatch):
    monkeypatch.syspath_prepend(str(PLUGIN))
    return importlib.import_module("scalar_facts")


def declaration(
    key,
    *,
    kind="variable",
    width=32,
    signedness="signed",
    spelling="int",
    byte=False,
    name_bool=False,
    domain="integer",
):
    return f"D\t{key}\tsrc/wiz8/test.cpp\t1\t1\t{kind}\t{key}\t{int(name_bool)}\t{int(byte)}\t{width}\t{signedness}\t{domain}\t{spelling}"


def read(scalar, tmp_path, *rows):
    (tmp_path / "facts-1.tsv").write_text("\n".join(rows) + "\n")
    return scalar.read_scalar_facts(tmp_path)


def claim(key, property_="signedness", value="unsigned", **extra):
    return {
        "key": key,
        "property": property_,
        "value": value,
        "basis": {
            "kind": "retail",
            "reference": "retail-sha256:address",
            "reason": "reviewed operand identity",
            "mnemonic": "jnc",
            "value_width": 32 if property_ == "signedness" else value,
            "operand_width": 32,
        },
        **extra,
    }


def evidence(scalar, tmp_path, facts, *claims):
    path = tmp_path / "evidence.json"
    path.write_text(json.dumps({"schema": "wiz8.scalar-evidence-v1", "claims": claims}))
    return scalar.read_evidence(path, facts)


def property_report(scalar, facts, claims=(), property_="signedness"):
    return scalar.integer_report(facts, list(claims))["integer_components"][property_][0]


def source_enum_facts(scalar, tmp_path, *rows):
    facts = read(
        scalar,
        tmp_path,
        "M\tsrc/wiz8/test.cpp",
        declaration("owner", domain="enum", spelling="W8Condition"),
        *rows,
    )
    facts.expected_units = {"src/wiz8/test.cpp"}
    return facts


def test_source_enum_copy_chain_is_separate_from_historical_recovery(scalar, tmp_path):
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration("first"),
        declaration("second"),
        "F\tfirst\towner\tinitializer\tsrc/wiz8/test.cpp\t2\t1",
        "F\tsecond\tfirst\tassignment\tsrc/wiz8/test.cpp\t3\t1",
    )
    result = scalar.enum_propagation_report(facts, ["W8Condition"])
    assert result["proposals"] == [
        {
            "members": ["first", "second"],
            "changes": ["first", "second"],
            "value": "enum:W8Condition",
            "status": "candidate",
            "blockers": [],
        }
    ]
    assert "no independent historical evidence" in result["basis"]
    assert all(
        row["status"] == "unknown"
        for row in scalar.integer_report(facts, [])["integer_components"]["domain"]
    )


@pytest.mark.parametrize("source_problem", ["missing", "inconsistent"])
def test_source_enum_cast_requires_consistent_input_facts(scalar, tmp_path, source_problem):
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration("packed", width=8, spelling="char"),
        declaration("copy"),
        "F\tcopy\towner\tinitializer\tsrc/wiz8/test.cpp\t1\t1",
        "V\tcopy\tpacked\tchar\tenum W8Condition",
        "F\tcopy\tpacked\texplicit-conversion\tsrc/wiz8/test.cpp\t2\t1",
    )
    if source_problem == "missing":
        del facts.declarations["packed"]
    else:
        facts.inconsistent.add("packed")
    assert (
        scalar.enum_propagation_report(facts, ["W8Condition"])["proposals"][0]["status"]
        == "blocked"
    )


def test_source_enum_cast_result_propagates_without_widening_packed_input(scalar, tmp_path):
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration(
            "packed", kind="field", width=8, signedness="unsigned", spelling="unsigned char"
        ),
        declaration("first"),
        declaration("second"),
        "V\tfirst\tpacked\tunsigned char\tenum W8Condition",
        "F\tfirst\tpacked\texplicit-conversion\tsrc/wiz8/test.cpp\t2\t1",
        "A\tfirst\texplicit conversion\tsrc/wiz8/test.cpp\t2\t1",
        "F\tsecond\tfirst\tassignment\tsrc/wiz8/test.cpp\t3\t1",
    )
    proposals = scalar.enum_propagation_report(facts, ["W8Condition"])["proposals"]
    assert len(proposals) == 1
    assert proposals[0]["status"] == "candidate"
    assert proposals[0]["changes"] == ["first", "second"]
    assert all(
        row["status"] == "unknown"
        for row in scalar.integer_report(facts, [])["integer_components"]["domain"]
    )


@pytest.mark.parametrize(
    "extra",
    [
        "K\tfirst\t0\tsrc/wiz8/test.cpp\t1\t1",
        "F\tfirst\tpacked\tassignment\tsrc/wiz8/test.cpp\t3\t1",
        "V\tfirst\tpacked\tunsigned char\tint",  # ambiguous conversion destinations
        "U\tfirst\t++\tsrc/wiz8/test.cpp\t3\t1",
        "A\tfirst\taddress taken\tsrc/wiz8/test.cpp\t3\t1",
    ],
)
def test_source_enum_cast_keeps_mixed_producers_and_unsafe_uses_blocked(scalar, tmp_path, extra):
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration(
            "packed", kind="field", width=8, signedness="unsigned", spelling="unsigned char"
        ),
        declaration("first"),
        "F\tfirst\towner\tinitializer\tsrc/wiz8/test.cpp\t1\t1",
        "V\tfirst\tpacked\tunsigned char\tenum W8Condition",
        "F\tfirst\tpacked\texplicit-conversion\tsrc/wiz8/test.cpp\t2\t1",
        extra,
    )
    assert (
        scalar.enum_propagation_report(facts, ["W8Condition"])["proposals"][0]["status"]
        == "blocked"
    )


@pytest.mark.parametrize(
    "kind,width,signedness",
    [("field", 32, "signed"), ("variable", 8, "signed"), ("variable", 32, "unsigned")],
)
def test_source_enum_cast_preserves_destination_storage(scalar, tmp_path, kind, width, signedness):
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration("packed", width=8, signedness="unsigned", spelling="unsigned char"),
        declaration("copy", kind=kind, width=width, signedness=signedness),
        "V\tcopy\tpacked\tunsigned char\tenum W8Condition",
        "F\tcopy\tpacked\texplicit-conversion\tsrc/wiz8/test.cpp\t2\t1",
    )
    assert (
        scalar.enum_propagation_report(facts, ["W8Condition"])["proposals"][0]["status"]
        == "blocked"
    )


@pytest.mark.parametrize(
    "extra",
    [
        "K\tfirst\t0\tsrc/wiz8/test.cpp\t1\t1",  # mixed enum/numeric producers
        "U\tfirst\t++\tsrc/wiz8/test.cpp\t2\t1",
        "A\tfirst\taddress taken\tsrc/wiz8/test.cpp\t2\t1",
        "F\tfirst\tunknown\tassignment\tsrc/wiz8/test.cpp\t2\t1",
        "F\tfirst\tsecond\tassignment\tsrc/wiz8/test.cpp\t2\t1",  # cycle
    ],
)
def test_source_enum_blocks_unsafe_upstream_and_all_downstream(scalar, tmp_path, extra):
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration("first"),
        declaration("second"),
        "F\tfirst\towner\tinitializer\tsrc/wiz8/test.cpp\t2\t1",
        "F\tsecond\tfirst\tassignment\tsrc/wiz8/test.cpp\t3\t1",
        extra,
    )
    proposals = scalar.enum_propagation_report(facts, ["W8Condition"])["proposals"]
    assert len(proposals) == 2
    assert all(row["status"] == "blocked" and not row["changes"] for row in proposals)


@pytest.mark.parametrize("kind,width", [("field", 32), ("parameter", 32), ("variable", 8)])
def test_source_enum_keeps_storage_signature_and_width_boundaries(scalar, tmp_path, kind, width):
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration("consumer", kind=kind, width=width),
        "F\tconsumer\towner\tassignment\tsrc/wiz8/test.cpp\t2\t1",
    )
    assert (
        scalar.enum_propagation_report(facts, ["W8Condition"])["proposals"][0]["status"]
        == "blocked"
    )


def test_source_enum_conflict_and_incomplete_corpus_block_patch(scalar, tmp_path):
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration("other", domain="enum", spelling="W8Skill"),
        declaration("consumer"),
        "F\tconsumer\towner\tassignment\tsrc/wiz8/test.cpp\t2\t1",
        "F\tconsumer\tother\tassignment\tsrc/wiz8/test.cpp\t3\t1",
    )
    row = scalar.enum_propagation_report(facts, ["W8Condition"])["proposals"][0]
    assert row["status"] == "blocked"
    assert {b["reason"] for b in row["blockers"]} == {"different enum producers"}
    facts.expected_units.add("src/wiz8/missing.cpp")
    assert any(
        b["reason"] == "incomplete source corpus"
        for b in scalar.enum_propagation_report(facts, ["W8Condition"])["proposals"][0]["blockers"]
    )


def test_unsigned_evidence_propagates_entire_copy_chain(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("field", kind="field"),
        declaration("getter", kind="function"),
        declaration("local"),
        declaration("parameter", kind="parameter"),
        declaration("stored", kind="field"),
        "H\tgetter\tbody\tsrc/wiz8/test.cpp\t2\t1",
        "H\tparameter\tbody\tsrc/wiz8/other.cpp\t2\t1",
        "F\tgetter\tfield\treturn\tsrc/wiz8/test.cpp\t2\t1",
        "F\tlocal\tgetter\tinitializer\tsrc/wiz8/test.cpp\t3\t1",
        "F\tparameter\tlocal\targument\tsrc/wiz8/test.cpp\t4\t1",
        "F\tstored\tparameter\tassignment\tsrc/wiz8/other.cpp\t3\t1",
    )
    seeds = evidence(scalar, tmp_path, facts, claim("field"))
    result = property_report(scalar, facts, seeds)
    assert result["status"] == "candidate"
    assert result["changes"] == ["field", "getter", "local", "parameter", "stored"]
    assert property_report(scalar, facts, seeds, "width")["status"] == "unknown"


def test_current_unsigned_source_type_is_not_a_seed(scalar, tmp_path):
    facts = read(
        scalar, tmp_path, declaration("A", signedness="unsigned", spelling="unsigned long")
    )
    assert property_report(scalar, facts)["status"] == "unknown"


@pytest.mark.parametrize(
    "spelling,domain,hazard",
    [
        ("unsigned long", "integer", "long_ilp32_llp64_32_lp64_64"),
        ("size_t", "integer", "pointer_sized_integer"),
        ("wchar_t *", "pointer", "vc6_wide_code_unit_16"),
        ("Mode", "enum", "enum_representation_review"),
        ("int Owner::*", "pointer", "pointer_representation"),
    ],
)
def test_abi_inventory_never_seeds_retail_width(scalar, tmp_path, spelling, domain, hazard):
    facts = read(scalar, tmp_path, declaration("A", spelling=spelling, domain=domain))
    report = scalar.integer_report(facts, [])
    assert hazard in report["abi_width_inventory"][0]["hazards"]
    assert property_report(scalar, facts, property_="width")["status"] == "unknown"


def test_long_long_and_unresolved_alias_require_no_false_long_classification(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("A", spelling="unsigned long long"),
        declaration("B", spelling="FLAGS32"),
    )
    assert scalar.abi_width_inventory(facts) == []


def test_pointer_transport_through_typedef_is_attributed_without_narrowing_claim(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("pointer", domain="pointer", spelling="HWND"),
        declaration("slot", spelling="DWORD"),
        "F\tslot\tpointer\texplicit-conversion\tsrc/wiz8/test.cpp\t8\t1",
    )
    row = scalar.integer_report(facts, [])["pointer_integer_transports"][0]
    assert row["source_type"] == "HWND"
    assert row["target_type"] == "DWORD"
    assert row["line"] == 8
    assert row["status"] == "requires_abi_boundary_review"


def test_saved_scalar_report_attributes_inputs_without_claiming_source_freshness(scalar, tmp_path):
    read(scalar, tmp_path, declaration("slot", spelling="DWORD"))
    report = scalar.write_integer_report(tmp_path, None, tmp_path / "report.json")
    snapshot = report["input_snapshot"]
    assert snapshot["current_source_status"] == "not_verified"
    assert snapshot["files"] == [
        {
            "path": str(tmp_path / "facts-1.tsv"),
            "sha256": hashlib.sha256((tmp_path / "facts-1.tsv").read_bytes()).hexdigest(),
        }
    ]


def test_conflicts_are_reported_without_changes(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A"))
    result = property_report(scalar, facts, [claim("A"), claim("A", value="signed")])
    assert result["status"] == "conflict"
    assert not result["changes"]


@pytest.mark.parametrize(
    "reason",
    [
        "address taken",
        "function pointer",
        "virtual slot",
        "explicit conversion",
        "reference binding",
        "aggregate storage argument",
    ],
)
def test_escape_blocks_whole_chain(scalar, tmp_path, reason):
    facts = read(
        scalar,
        tmp_path,
        declaration("A"),
        declaration("B"),
        "F\tB\tA\tassignment\tsrc/wiz8/test.cpp\t2\t1",
        f"A\tB\t{reason}\tsrc/wiz8/test.cpp\t3\t1",
    )
    result = property_report(scalar, facts, [claim("A")])
    assert result["status"] == "blocked"
    assert result["blockers"][0]["key"] == "B"


def test_operation_needs_review_at_exact_node_and_site(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A"), "U\tA\t<\tsrc/wiz8/test.cpp\t5\t1")
    assert property_report(scalar, facts, [claim("A")])["status"] == "blocked"
    seed = claim("A", covered_uses=[{"file": "src/wiz8/test.cpp", "line": 5, "operation": "<"}])
    assert property_report(scalar, facts, [seed])["status"] == "candidate"


def test_missing_body_blocks_binary_only_return_seed(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A", kind="function"))
    assert property_report(scalar, facts, [claim("A")])["status"] == "blocked"


def test_known_external_api_can_anchor_without_body(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("API", kind="function"),
        declaration("local"),
        "F\tlocal\tAPI\tinitializer\tsrc/wiz8/test.cpp\t2\t1",
    )
    seed = claim("API")
    seed["basis"] = {
        "kind": "external-api",
        "reference": "released-header:API",
        "reason": "original API contract",
    }
    result = property_report(scalar, facts, evidence(scalar, tmp_path, facts, seed))
    assert result["status"] == "candidate"
    assert result["changes"] == ["API", "local"]


def test_width_and_signedness_are_independent(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A"))
    seed = claim("A", "width", 8)
    result = property_report(scalar, facts, evidence(scalar, tmp_path, facts, seed), "width")
    assert result["status"] == "candidate"
    assert result["value"] == 8
    assert property_report(scalar, facts, [seed])["value"] is None


def test_constant_range_does_not_seed_width(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A"), "K\tA\t1\tsrc/wiz8/test.cpp\t2\t1")
    assert property_report(scalar, facts, property_="width")["status"] == "unknown"


@pytest.mark.parametrize("constant", [-1, 256, 65536])
def test_incompatible_constants_block_narrowing(scalar, tmp_path, constant):
    facts = read(scalar, tmp_path, declaration("A"), f"K\tA\t{constant}\tsrc/wiz8/test.cpp\t2\t1")
    assert property_report(scalar, facts, [claim("A", "width", 8)], "width")["status"] == "blocked"


def test_low_register_return_alone_does_not_establish_narrow_return(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A", kind="function"))
    with pytest.raises(ValueError, match="complete caller"):
        evidence(scalar, tmp_path, facts, claim("A", "width", 8))


def test_field_width_needs_receiver_attributed_complete_accesses(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A", kind="field"))
    with pytest.raises(ValueError, match="receiver-attributed"):
        evidence(scalar, tmp_path, facts, claim("A", "width", 8))


@pytest.mark.parametrize(
    "mnemonic,value",
    [
        ("jl", "signed"),
        ("jge", "signed"),
        ("idiv", "signed"),
        ("movsx", "signed"),
        ("jb", "unsigned"),
        ("jnc", "unsigned"),
        ("div", "unsigned"),
        ("movzx", "unsigned"),
    ],
)
def test_instruction_signedness_evidence(scalar, tmp_path, mnemonic, value):
    width = 8 if mnemonic in {"movsx", "movzx"} else 32
    facts = read(scalar, tmp_path, declaration("A", width=width))
    seed = claim("A", value=value)
    seed["basis"]["mnemonic"] = mnemonic
    seed["basis"].update(value_width=width, operand_width=width, operand_role="input")
    assert evidence(scalar, tmp_path, facts, seed)[0]["value"] == value


def test_shift_evidence_requires_high_bits_to_matter(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A"))
    seed = claim("A")
    seed["basis"]["mnemonic"] = "shr"
    with pytest.raises(ValueError, match="instruction"):
        evidence(scalar, tmp_path, facts, seed)
    seed["basis"]["high_bits_relevant"] = True
    assert evidence(scalar, tmp_path, facts, seed)


def test_promoted_byte_comparison_is_not_signed_byte_proof(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A", width=8, byte=True))
    seed = claim("A", value="signed")
    seed["basis"].update(mnemonic="jl", value_width=8)
    with pytest.raises(ValueError, match="promotion"):
        evidence(scalar, tmp_path, facts, seed)


@pytest.mark.parametrize("kind", ["source-projection", "recomp-pdb", "heuristic"])
def test_projected_types_cannot_anchor_recovery(scalar, tmp_path, kind):
    facts = read(scalar, tmp_path, declaration("A"))
    seed = claim("A")
    seed["basis"]["kind"] = kind
    with pytest.raises(ValueError, match="independent provenance"):
        evidence(scalar, tmp_path, facts, seed)


def test_int_long_spelling_is_preserved(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("A", spelling="long"),
        declaration("B", spelling="int"),
        "F\tB\tA\tassignment\tsrc/wiz8/test.cpp\t2\t1",
    )
    report = scalar.integer_report(facts, [claim("A", value="signed")])
    assert property_report(scalar, facts, [claim("A", value="signed")])["changes"] == []
    assert [d["spelling"] for d in report["declarations"]] == ["long", "int"]


def test_duplicate_declarations_merge_bool_metadata(scalar, tmp_path):
    read(scalar, tmp_path, declaration("A", width=8, byte=True))
    (tmp_path / "facts-2.tsv").write_text(
        declaration("A", width=8, byte=True, name_bool=True) + "\n"
    )
    facts = scalar.read_scalar_facts(tmp_path)
    assert facts.declarations["A"].name_bool
    assert not facts.inconsistent


def test_inconsistent_cross_tu_metadata_blocks_recovery(scalar, tmp_path):
    read(scalar, tmp_path, declaration("A"))
    (tmp_path / "facts-2.tsv").write_text(declaration("A", width=16) + "\n")
    facts = scalar.read_scalar_facts(tmp_path)
    assert property_report(scalar, facts, [claim("A")])["status"] == "blocked"


def test_malformed_facts_fail_closed(scalar, tmp_path):
    with pytest.raises(ValueError, match="facts-1.tsv:1"):
        read(scalar, tmp_path, "F\tmissing")


def test_32_bit_predicates_are_inventory_only(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("A", kind="function", name_bool=True),
        "H\tA\tbody\tsrc/wiz8/test.cpp\t2\t1",
        "G\tA\tbool\tsrc/wiz8/test.cpp\t3\t1",
    )
    report = scalar.integer_report(facts, [])
    assert report["predicate32_inventory"][0]["key"] == "A"
    assert report["report_only"]
    assert property_report(scalar, facts)["status"] == "unknown"


def test_existing_bool_solver_contract():
    subprocess.run(
        [sys.executable, str(PLUGIN / "clang-tidy-wrapper.py"), "--wiz8-wrapper-self-test"],
        check=True,
    )


def test_narrow_operand_cannot_seed_wider_storage_signedness(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A"))
    seed = claim("A")
    seed["basis"].update(mnemonic="movzx", value_width=8, operand_width=8, operand_role="input")
    with pytest.raises(ValueError, match="storage-width"):
        evidence(scalar, tmp_path, facts, seed)
    assert evidence(scalar, tmp_path, facts, seed, claim("A", "width", 8))


def test_extension_output_does_not_prove_its_source_type(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A"))
    seed = claim("A")
    seed["basis"].update(mnemonic="movzx", operand_role="output")
    with pytest.raises(ValueError, match="narrow input"):
        evidence(scalar, tmp_path, facts, seed)


def test_parameter_width_requires_argument_boundary(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A", kind="parameter"))
    with pytest.raises(ValueError, match="argument/callee"):
        evidence(scalar, tmp_path, facts, claim("A", "width", 8))


def test_integer_report_cli_reuses_saved_facts_without_compiling(scalar, tmp_path):
    read(scalar, tmp_path, declaration("A"))
    destination = tmp_path / "report.json"
    subprocess.run(
        [
            sys.executable,
            str(PLUGIN / "clang-tidy-wrapper.py"),
            "--wiz8-scalar-report",
            str(tmp_path),
            "--output",
            str(destination),
        ],
        check=True,
    )
    report = json.loads(destination.read_text())
    assert report["report_only"]
    assert report["integer_components"]["signedness"][0]["status"] == "unknown"


@pytest.mark.parametrize("body_present", [False, True])
def test_bool_pending_escape_resolves_across_translation_units(scalar, tmp_path, body_present):
    read(
        scalar,
        tmp_path,
        declaration("A", width=8, byte=True),
        "P\tA\tsrc/wiz8/test.cpp\t2\t1\tMutate/1",
    )
    (tmp_path / "facts-2.tsv").write_text("B\tMutate/1\n" if body_present else "")
    facts = scalar.read_scalar_facts(tmp_path)
    assert ("A" in facts.bool_escaped) != body_present


def enum_claim(key, name="W8Mode"):
    result = claim(key, "domain", "enum:" + name)
    result["basis"] = {
        "kind": "source-oracle",
        "reference": "original-header:W8Mode",
        "reason": "released enum declaration and owner signature",
    }
    return result


def test_known_enum_propagates_without_inventing_domain(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("A", domain="enum", spelling="W8Mode"),
        declaration("B"),
        "F\tB\tA\tinitializer\tsrc/wiz8/test.cpp\t2\t1",
    )
    seeds = evidence(scalar, tmp_path, facts, enum_claim("A"))
    report = property_report(scalar, facts, seeds, "domain")
    assert report["status"] == "candidate"
    assert report["changes"] == ["B"]


def test_numeric_range_cannot_invent_enum(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A"), "K\tA\t1\tsrc/wiz8/test.cpp\t2\t1")
    with pytest.raises(ValueError, match="cannot invent"):
        evidence(scalar, tmp_path, facts, enum_claim("A"))


@pytest.mark.parametrize("producer", ["constant", "different_enum"])
def test_enum_propagation_blocks_other_producer_domains(scalar, tmp_path, producer):
    rows = [
        declaration("A", domain="enum", spelling="W8Mode"),
        declaration("B"),
        "F\tB\tA\tassignment\tsrc/wiz8/test.cpp\t2\t1",
    ]
    if producer == "constant":
        rows.append("K\tB\t1\tsrc/wiz8/test.cpp\t3\t1")
    else:
        rows.extend(
            [
                declaration("C", domain="enum", spelling="W8Other"),
                "F\tB\tC\tassignment\tsrc/wiz8/test.cpp\t3\t1",
            ]
        )
    facts = read(scalar, tmp_path, *rows)
    assert property_report(scalar, facts, [enum_claim("A")], "domain")["status"] == "blocked"


def test_retail_numbers_cannot_prove_enum_identity(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A", domain="enum", spelling="W8Mode"))
    with pytest.raises(ValueError, match="independent symbol"):
        evidence(scalar, tmp_path, facts, claim("A", "domain", "enum:W8Mode"))


@pytest.mark.parametrize("kind", ["field", "function", "parameter"])
def test_propagated_width_requires_each_changed_boundary_review(scalar, tmp_path, kind):
    facts = read(
        scalar,
        tmp_path,
        declaration("A"),
        declaration("B", kind=kind),
        "H\tB\tbody\tsrc/wiz8/test.cpp\t2\t1",
        "F\tB\tA\tassignment\tsrc/wiz8/test.cpp\t3\t1",
    )
    seeds = evidence(scalar, tmp_path, facts, claim("A", "width", 8))
    result = property_report(scalar, facts, seeds, "width")
    assert result["status"] == "blocked"
    assert any(b["reason"] == "unreviewed width boundary" for b in result["blockers"])


def test_fact_fields_round_trip_delimiters_and_backslashes(scalar, tmp_path):
    row = declaration("A").split("\t")
    row[2] = r"src\\wiz8\todd\nfile.cpp"
    row[12] = r"int (*)(\nint,\tint)"
    facts = read(scalar, tmp_path, "\t".join(row))
    assert facts.declarations["A"].file == "src\\wiz8\todd\nfile.cpp"
    assert facts.declarations["A"].spelling == "int (*)(\nint,\tint)"


def test_unicode_line_separator_does_not_split_fact_record(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A").replace("test.cpp", "test\u2028.cpp"))
    assert "\u2028" in facts.declarations["A"].file


def test_invalid_field_escape_fails_closed(scalar, tmp_path):
    with pytest.raises(ValueError, match="invalid fact field escape"):
        read(scalar, tmp_path, declaration("A").replace("test.cpp", r"test\z.cpp"))


def test_failed_compiler_is_not_hidden_by_partial_facts(tmp_path):
    compiler = tmp_path / "failed-compiler.py"
    compiler.write_text(
        "#!/usr/bin/env python3\nimport os\nfrom pathlib import Path\n"
        "Path(os.environ['WIZ8_SCALAR_FACTS_DIR'], 'facts-1.tsv').write_text('D\\tpartial')\n"
        "raise SystemExit(7)\n"
    )
    compiler.chmod(0o755)
    runner = tmp_path / "runner.py"
    runner.write_text(
        "import importlib.util, sys\n"
        + f"sys.path.insert(0, {str(PLUGIN)!r})\n"
        + f"spec=importlib.util.spec_from_file_location('wrapper', {str(PLUGIN / 'clang-tidy-wrapper.py')!r})\n"
        "m=importlib.util.module_from_spec(spec);sys.modules['wrapper']=m;spec.loader.exec_module(m)\n"
        + f"m.REAL_CLANG_TIDY={str(compiler)!r}\n"
        + "m.main()\n"
    )
    result = subprocess.run(
        [sys.executable, str(runner)], capture_output=True, text=True, check=False
    )
    assert result.returncode == 7
    assert "ValueError" not in result.stderr


def test_member_pointer_domain_cannot_be_promoted_as_integer(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("A", domain="pointer", spelling="int Owner::*"))
    assert property_report(scalar, facts, [claim("A")])["status"] == "blocked"


@pytest.mark.parametrize("sentinel,tag", [(-1, "K"), (-1, "O"), (4294967295, "O")])
def test_unsigned_seed_cannot_erase_sentinel_anywhere_in_chain(scalar, tmp_path, sentinel, tag):
    row = f"K\tB\t{sentinel}\tx.cpp\t2\t1" if tag == "K" else f"O\tB\t==\t{sentinel}\tx.cpp\t2\t1"
    facts = read(
        scalar,
        tmp_path,
        declaration("A"),
        declaration("B"),
        "F\tB\tA\tassignment\tx.cpp\t1\t1",
        row,
    )
    result = property_report(scalar, facts, [claim("A")])
    assert result["status"] == "blocked"
    assert any("sentinel" in blocker["reason"] for blocker in result["blockers"])


def test_flags_and_status_share_graph_without_inventing_types(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("flags"),
        declaration("status"),
        "O\tflags\t|=\t4\tx.cpp\t2\t1",
        "O\tflags\t&\t8\tx.cpp\t3\t1",
        "K\tstatus\t-1\tx.cpp\t4\t1",
        "K\tstatus\t0\tx.cpp\t5\t1",
        "K\tstatus\t1\tx.cpp\t6\t1",
    )
    inventory = {item["members"][0]: item for item in scalar.domain_inventory(facts)}
    assert inventory["flags"]["behavior"] == "flags-like"
    assert inventory["status"]["value_domain"] == "tri-state-values"
    assert not inventory["flags"]["source_type_recovered"]


def test_status_values_propagate_directionally_and_unknown_producer_blocks(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("producer"),
        declaration("local"),
        "K\tproducer\t0\tx.cpp\t1\t1",
        "K\tproducer\t1\tx.cpp\t2\t1",
        "F\tlocal\tproducer\tinitializer\tx.cpp\t3\t1",
        "A\tlocal\tunmodeled expression producer\tx.cpp\t4\t1",
    )
    item = scalar.domain_inventory(facts)[0]
    assert item["observed_values"] == [0, 1]
    assert not item["complete_value_domain"]


def independent_claim(key, property_, value, **extra):
    return {
        "key": key,
        "property": property_,
        "value": value,
        "basis": {
            "kind": "source-oracle",
            "reference": "original-header",
            "reason": "independently established owner",
        },
        **extra,
    }


def test_void_pointer_chain_uses_known_owner_and_cast_metadata(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("owner", domain="pointer", spelling="Record *"),
        declaration("storage", domain="pointer", spelling="void *"),
        declaration("result", domain="pointer", spelling="Record *"),
        "T\towner\tRecord *\t\tRecord",
        "T\tstorage\tvoid *\t\tvoid",
        "T\tresult\tRecord *\t\tRecord",
        "F\tstorage\towner\tassignment\tx.cpp\t1\t1",
        "F\tresult\tstorage\texplicit-conversion\tx.cpp\t2\t1",
        "V\tresult\tstorage\tvoid *\tRecord *",
        "A\tresult\texplicit conversion\tx.cpp\t2\t1",
        "A\tstorage\texplicit conversion\tx.cpp\t2\t1",
    )
    # Erasure is directional: the typed producer must agree with the erased
    # storage's own evidence, but never seeds that storage by itself.
    seeds = evidence(scalar, tmp_path, facts, independent_claim("storage", "pointee", "Record"))
    by_member = {
        member: item
        for item in scalar.anchored_report(facts, seeds, "pointee")
        for member in item["members"]
    }
    assert by_member["storage"]["status"] == "candidate"
    assert by_member["storage"]["changes"] == ["storage"]
    owner_seed = independent_claim("owner", "pointee", "Record")
    assert not any(
        item["changes"] for item in scalar.anchored_report(facts, [owner_seed], "pointee")
    )
    assert scalar.anchored_report(facts, [], "pointee")[0]["status"] == "unknown"


@pytest.mark.parametrize(
    "producer,pointee,escape",
    [
        ("Record", "Other", ""),
        ("Record", "const Record", ""),
        ("Record", "Record", "address taken"),
        ("Other", "void", ""),
    ],
)
def test_pointer_conflicts_qualifiers_and_mutation_block(
    scalar, tmp_path, producer, pointee, escape
):
    rows = [
        declaration("owner", domain="pointer"),
        declaration("storage", domain="pointer"),
        f"T\towner\t{producer} *\t\t{producer}",
        f"T\tstorage\t{pointee} *\t\t{pointee}",
        "F\tstorage\towner\tassignment\tx.cpp\t1\t1",
    ]
    if escape:
        rows.append(f"A\tstorage\t{escape}\tx.cpp\t2\t1")
    facts = read(scalar, tmp_path, *rows)
    result = next(
        item
        for item in scalar.anchored_report(
            facts, [independent_claim("storage", "pointee", "Record")], "pointee"
        )
        if "storage" in item["members"]
    )
    assert result["status"] == "blocked"


def test_nominal_id_owner_does_not_normalize_identical_other_typedef(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("owner"),
        declaration("copy"),
        "T\towner\tunsigned int\tMonsterId\t",
        "T\tcopy\tunsigned int\tSkillId\t",
        "F\tcopy\towner\tassignment\tx.cpp\t1\t1",
    )
    # A typedef'd producer neither seeds nor normalizes a differently typed consumer.
    seeds = evidence(
        scalar, tmp_path, facts, independent_claim("owner", "nominal", "MonsterId", role="ID")
    )
    assert not any(item["changes"] for item in scalar.anchored_report(facts, seeds, "nominal"))
    consumer = independent_claim("copy", "nominal", "MonsterId", role="ID")
    result = next(
        item
        for item in scalar.anchored_report(facts, [consumer], "nominal")
        if "copy" in item["members"]
    )
    assert result["status"] == "blocked"


def test_nominal_identity_and_role_require_independent_evidence(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("owner"), "T\towner\tunsigned int\tMonsterId\t")
    with pytest.raises(ValueError, match="semantic role"):
        evidence(scalar, tmp_path, facts, independent_claim("owner", "nominal", "MonsterId"))
    with pytest.raises(ValueError, match="existing typedef"):
        evidence(scalar, tmp_path, facts, independent_claim("owner", "nominal", "NewId", role="ID"))
    with pytest.raises(ValueError, match="concrete pointee"):
        evidence(scalar, tmp_path, facts, claim("owner", "pointee", "Record"))


def test_callback_slot_is_not_an_independent_scalar_lint(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("slot", domain="pointer"),
        declaration("slot::callback-arg#0", kind="callback-parameter"),
        "J\tslot\t1\t0\t0",
        "C\tslot\timplementation\t1\t0\t0",
    )
    report = scalar.callback_report(facts)[0]
    assert report["status"] == "modeled"
    assert report["nodes"] == ["slot::callback-arg#0"]
    assert "before edits" in report["requires"]


@pytest.mark.parametrize(
    "binding",
    [
        "C\tslot\timplementation\t2\t0\t0",
        "C\tslot\timplementation\t1\t1\t0",
        "C\tslot\timplementation\t1\t0\t1",
    ],
)
def test_callback_abi_mismatch_blocks_component(scalar, tmp_path, binding):
    facts = read(
        scalar, tmp_path, declaration("slot", domain="pointer"), "J\tslot\t1\t0\t0", binding
    )
    assert scalar.callback_report(facts)[0]["status"] == "blocked"


def span(key, file, offset, text, original, component="type"):
    import hashlib

    return f"L\t{key}\t{component}\t{file}\t{offset}\t{len(text.encode())}\t{text}\t{hashlib.sha256(original.encode()).hexdigest()}"


def test_reviewable_patch_changes_entire_chain_and_all_redeclarations(scalar, tmp_path):
    header = "long Read();\n"
    source = "long Read() { return 0; }\nlong copy;\n"
    (tmp_path / "test.h").write_text(header)
    (tmp_path / "test.cpp").write_text(source)
    facts = read(
        scalar,
        tmp_path,
        declaration("return", kind="function", spelling="long"),
        declaration("copy", spelling="long"),
        "H\treturn\tbody\ttest.cpp\t1\t1",
        "F\tcopy\treturn\tinitializer\ttest.cpp\t2\t1",
        span("return", "test.h", 0, "long", header),
        span("return", "test.cpp", 0, "long", source),
        span("copy", "test.cpp", source.index("long copy"), "long", source),
    )
    patch = tmp_path / "recovery.patch"
    result = scalar.write_recovery_patch(facts, [claim("return")], tmp_path, patch)
    assert result["changed_declarations"] == ["copy", "return"]
    assert patch.read_text().count("+unsigned long") == 3
    assert (tmp_path / "test.cpp").read_text() == source
    assert (tmp_path / "test.h").read_text() == header


def test_recovery_patch_rejects_partial_component_and_shared_atoms(scalar, tmp_path):
    source = "int a, b;\n"
    (tmp_path / "test.cpp").write_text(source)
    facts = read(
        scalar,
        tmp_path,
        declaration("a"),
        declaration("b"),
        span("a", "test.cpp", 0, "int", source),
        span("b", "test.cpp", 0, "int", source),
    )
    patch = tmp_path / "recovery.patch"
    result = scalar.write_recovery_patch(facts, [claim("a")], tmp_path, patch)
    assert not result["changed_declarations"]
    assert "shared" in result["rejected"][0]["reason"]
    assert patch.read_text() == ""
    facts.flows.add(scalar.Flow("b", "a", "assignment", "test.cpp", 1, 1))
    facts.spans = {item for item in facts.spans if item[0] == "a"}
    result = scalar.write_recovery_patch(facts, [claim("a")], tmp_path, patch)
    assert not result["changed_declarations"]
    assert "every changed declaration" in result["rejected"][0]["reason"]


def test_recovery_patch_rejects_stale_source_before_writing(scalar, tmp_path):
    source = "int a;\n"
    (tmp_path / "test.cpp").write_text(source + "// edited since collection\n")
    facts = read(scalar, tmp_path, declaration("a"), span("a", "test.cpp", 0, "int", source))
    patch = tmp_path / "recovery.patch"
    with pytest.raises(ValueError, match="stale source"):
        scalar.write_recovery_patch(facts, [claim("a")], tmp_path, patch)
    assert not patch.exists()


def test_width_proposal_does_not_generate_a_partial_signedness_edit(scalar, tmp_path):
    source = "int a;\n"
    (tmp_path / "test.cpp").write_text(source)
    facts = read(scalar, tmp_path, declaration("a"), span("a", "test.cpp", 0, "int", source))
    result = scalar.write_recovery_patch(
        facts, [claim("a"), claim("a", "width", 8)], tmp_path, tmp_path / "recovery.patch"
    )
    assert not result["changed_declarations"]


def test_increment_prevents_complete_finite_domain(scalar, tmp_path):
    facts = read(
        scalar, tmp_path, declaration("a"), "K\ta\t0\tx.cpp\t1\t1", "U\ta\t++\tx.cpp\t2\t1"
    )
    assert not scalar.domain_inventory(facts)[0]["complete_value_domain"]


def test_nominal_owner_uses_representation_not_int_long_spelling(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("owner", signedness="unsigned", spelling="FLAGS32"),
        declaration("copy", signedness="unsigned", spelling="unsigned int"),
        "T\towner\tunsigned long\tFLAGS32\t",
        "T\tcopy\tunsigned int\t\t",
        "F\tcopy\towner\tassignment\tx.cpp\t1\t1",
    )
    seeds = evidence(
        scalar, tmp_path, facts, independent_claim("copy", "nominal", "FLAGS32", role="flags")
    )
    result = next(
        item
        for item in scalar.anchored_report(facts, seeds, "nominal")
        if "copy" in item["members"]
    )
    assert result["status"] == "candidate"
    assert result["changes"] == ["copy"]


def test_whole_program_reader_deduplicates_identical_rows_preserving_conflicts(scalar, tmp_path):
    row = declaration("duration")
    (tmp_path / "facts-1.tsv").write_text((row + "\n") * 1000)
    (tmp_path / "facts-2.tsv").write_text(
        row + "\n" + declaration("duration", signedness="unsigned") + "\n"
    )
    facts = scalar.read_scalar_facts(tmp_path)
    assert set(facts.declarations) == {"duration"}
    assert facts.inconsistent == {"duration"}
    assert len(facts.locations) == 2


def test_predicate32_inventory_does_not_depend_on_bool_client_names(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("CheckReady", kind="function"),
        "H\tCheckReady\tbody\tsrc/wiz8/test.cpp\t2\t1",
        "K\tCheckReady\t0\tsrc/wiz8/test.cpp\t3\t1",
        "K\tCheckReady\t1\tsrc/wiz8/test.cpp\t4\t1",
    )
    report = scalar.integer_report(facts, [])
    assert [row["key"] for row in report["predicate32_inventory"]] == ["CheckReady"]
    assert report["integer_components"]["signedness"][0]["status"] == "unknown"


def test_callback_topology_requires_all_parameter_nodes(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("callback", domain="pointer"),
        "J\tcallback\t1\t0\t0",
        "C\tcallback\timpl\t1\t0\t0",
    )
    report = scalar.callback_report(facts)[0]
    assert report["status"] == "blocked"
    assert "unmodeled callback parameter" in report["blockers"]


def test_translation_unit_coverage_is_explicit_and_deduplicated(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        "M\tsrc/wiz8/one.cpp",
        "M\tsrc/wiz8/one.cpp",
        "M\tsrc/sgp/timer.c",
        declaration("duration"),
    )
    assert scalar.integer_report(facts, [])["translation_units"] == [
        "src/sgp/timer.c",
        "src/wiz8/one.cpp",
    ]


def test_array_and_record_observations_do_not_seed_recovery(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        "ARR\ta\tsrc/wiz8/test.cpp\t3\t1\tbuffer\tchar\t5\t8\ta::element",
        "AU\ta\tstring-initializer\tsrc/wiz8/test.cpp\t3\t8",
        "REC\tr\tsrc/wiz8/test.cpp\t4\tPacked\t40\t8\t64\t8",
        "RF\tr\tr.prefix\tprefix\t0\t8\t8\tchar",
        "RF\tr\tr.payload\tpayload\t8\t32\t32\tint",
    )
    report = scalar.structural_report(facts)
    assert report["arrays"][0]["extent"] == 5
    assert report["arrays"][0]["text_initializer"]
    assert report["records"][0]["packing_changes_size"]
    assert not any(scalar.integer_report(facts, [])["integer_components"].values())


def test_evidence_selector_survives_line_changes_and_rejects_ambiguity(scalar, tmp_path):
    facts = read(scalar, tmp_path, declaration("owner"))
    evidence = tmp_path / "evidence.json"
    payload = {
        "schema": "wiz8.scalar-evidence-v1",
        "claims": [
            {
                "selector": {"file": "src/wiz8/test.cpp", "kind": "variable", "name": "owner"},
                "property": "signedness",
                "value": "unsigned",
                "basis": {
                    "kind": "external-api",
                    "reference": "fixture contract",
                    "reason": "test",
                },
            }
        ],
    }
    evidence.write_text(json.dumps(payload))
    assert scalar.read_evidence(evidence, facts)[0]["key"] == "owner"
    facts.declarations["other"] = scalar.replace(facts.declarations["owner"], key="other", line=30)
    with pytest.raises(ValueError, match="one declaration"):
        scalar.read_evidence(evidence, facts)


def test_existing_enum_domain_generates_whole_chain_patch(scalar, tmp_path):
    source = "int mode;\nint Read() { return mode; }\n"
    (tmp_path / "test.cpp").write_text(source)
    facts = read(
        scalar,
        tmp_path,
        declaration("owner", domain="enum", spelling="W8Mode"),
        declaration("mode"),
        declaration("read", kind="function"),
        "H\tread\tbody\ttest.cpp\t2\t1",
        "F\tmode\towner\tassignment\ttest.cpp\t1\t1",
        "F\tread\tmode\treturn\ttest.cpp\t2\t1",
        span("mode", "test.cpp", 0, "int", source),
        span("read", "test.cpp", source.index("int Read"), "int", source, "return-type"),
    )
    seeds = evidence(scalar, tmp_path, facts, independent_claim("owner", "domain", "enum:W8Mode"))
    result = scalar.write_recovery_patch(facts, seeds, tmp_path, tmp_path / "recovery.patch")
    assert result["changed_declarations"] == ["mode", "read"]
    assert "+W8Mode mode;" in (tmp_path / "recovery.patch").read_text()
    assert "+W8Mode Read()" in (tmp_path / "recovery.patch").read_text()


def test_callback_typedef_parameter_and_implementation_patch_together(scalar, tmp_path):
    source = "typedef int (*Callback)(int);\nint Impl(int value);\n"
    (tmp_path / "test.cpp").write_text(source)
    slot = "slot::callback-arg#0"
    facts = read(
        scalar,
        tmp_path,
        declaration("slot", domain="pointer"),
        declaration(slot, kind="callback-parameter"),
        declaration("impl", kind="parameter"),
        "J\tslot\t1\t0\t0",
        "C\tslot\timplementation\t1\t0\t0",
        "H\timpl\tbody\ttest.cpp\t2\t1",
        f"F\timpl\t{slot}\tcallback-parameter\ttest.cpp\t2\t1",
        span(slot, "test.cpp", source.index("int);"), "int", source, "callback-param#0"),
        span("impl", "test.cpp", source.index("int value"), "int", source, "parameter-type"),
    )
    seeds = evidence(scalar, tmp_path, facts, claim("impl"))
    result = scalar.write_recovery_patch(facts, seeds, tmp_path, tmp_path / "recovery.patch")
    assert result["changed_declarations"] == ["impl", slot]
    assert "+typedef int (*Callback)(unsigned int);" in (tmp_path / "recovery.patch").read_text()
    assert "+int Impl(unsigned int value);" in (tmp_path / "recovery.patch").read_text()
    facts.escapes.add(scalar.Use("slot", "external ABI", "test.cpp", 1, 1))
    result = scalar.write_recovery_patch(facts, seeds, tmp_path, tmp_path / "blocked.patch")
    assert not result["changed_declarations"]


def test_shared_callback_typedef_requires_every_collected_slot(scalar, tmp_path):
    source = "typedef int (*Callback)(int);\n"
    (tmp_path / "test.cpp").write_text(source)
    changed, unbound = "a::callback-arg#0", "b::callback-arg#0"
    offset = source.index("int);")
    facts = read(
        scalar,
        tmp_path,
        declaration(changed),
        declaration(unbound),
        span(changed, "test.cpp", offset, "int", source, "callback-param#0"),
        span(unbound, "test.cpp", offset, "int", source, "callback-param#0"),
    )
    result = scalar.write_recovery_patch(
        facts, [claim(changed)], tmp_path, tmp_path / "recovery.patch"
    )
    assert not result["changed_declarations"]
    assert "shared" in result["rejected"][0]["reason"]


def test_array_extent_component_is_not_rewritten_as_type(scalar, tmp_path):
    source = "int values[3];\n"
    (tmp_path / "test.cpp").write_text(source)
    facts = read(
        scalar,
        tmp_path,
        declaration("element", kind="array-element"),
        span("element", "test.cpp", 0, "int", source, "array-element"),
        span("element", "test.cpp", source.index("3"), "3", source, "array-extent"),
    )
    scalar.write_recovery_patch(facts, [claim("element")], tmp_path, tmp_path / "recovery.patch")
    assert "+unsigned int values[3];" in (tmp_path / "recovery.patch").read_text()


def test_oracle_harvest_pairs_contracts_not_type_similarity(scalar, tmp_path):
    current_dir, original_dir = tmp_path / "current", tmp_path / "original"
    current_dir.mkdir()
    original_dir.mkdir()
    current_key = "src/current.h:1:1:function:Clock"
    original_key = "/oracle/source/clock.h:1:1:function:Clock"
    current = read(
        scalar,
        current_dir,
        declaration(current_key, kind="function"),
        f"FN\t{current_key}\tClock\t0\t0\t0\t_Clock",
        f"T\t{current_key}\tint\tTIMER\t",
    )
    original = read(
        scalar,
        original_dir,
        declaration(original_key, kind="function", signedness="unsigned", spelling="TIMER"),
        f"FN\t{original_key}\tClock\t0\t0\t0\t_Clock",
        f"T\t{original_key}\tunsigned long\tTIMER\t",
    )
    mapping = {
        "original": {"key": original_key},
        "current": {"semantic_id": "_Clock"},
        "nominal_roles": {"return": "timer"},
        "basis": {
            "kind": "source-oracle",
            "reference": "git:pinned:clock.h",
            "reason": "retained API",
        },
    }
    result = scalar.harvest_declaration_evidence(current, original, [mapping])
    claims = evidence(scalar, tmp_path, current, *result["claims"])
    assert {(c["property"], c["value"]) for c in claims} == {
        ("width", 32),
        ("signedness", "unsigned"),
        ("nominal", "TIMER"),
    }
    assert all(c["basis"]["declaration"]["file"] == "src/wiz8/test.cpp" for c in claims)
    mapping["basis"]["kind"] = "decorated-export"
    assert not any(
        c["property"] == "nominal"
        for c in scalar.harvest_declaration_evidence(current, original, [mapping])["claims"]
    )
    original.signatures[original_key] = ("Clock", 1, 0, False, "_Clock")
    assert scalar.harvest_declaration_evidence(current, original, [mapping])["claims"] == []
    original.signatures[original_key] = ("Clock", 0, 0, False, "_Clock")
    original.types[original_key] = ("char", "", "")
    assert {
        c["property"]
        for c in scalar.harvest_declaration_evidence(current, original, [mapping])["claims"]
    } == {"width"}
    mapping["current"] = {"semantic_id": "_Missing"}
    result = scalar.harvest_declaration_evidence(current, original, [mapping])
    assert not result["claims"] and "requires one declaration" in result["skipped"][0]["reason"]


def test_enum_identity_uses_canonical_owner_not_in_class_spelling(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("parameter", kind="parameter", domain="enum", spelling="e_mode"),
        declaration("field", kind="field", domain="enum", spelling="const ModeAlias"),
        "T\tparameter\tenum Owner::e_mode\t\t",
        "T\tfield\tconst enum Owner::e_mode\tModeAlias\t",
        "F\tfield\tparameter\tassignment\tsrc/wiz8/test.cpp\t2\t1",
        "H\tparameter\tbody\tsrc/wiz8/test.cpp\t2\t1",
    )
    c = {
        "key": "parameter",
        "property": "domain",
        "value": "enum:Owner::e_mode",
        "basis": {
            "kind": "decorated-export",
            "reference": "independent export",
            "reason": "existing enum owner",
        },
    }
    claims = evidence(scalar, tmp_path, facts, c)
    report = property_report(scalar, facts, claims, "domain")
    assert report["status"] == "candidate" and report["changes"] == []
    facts.types["field"] = ("enum Other::e_mode", "", "")
    assert property_report(scalar, facts, claims, "domain")["status"] == "blocked"


def test_qualified_void_is_erased_storage_not_a_concrete_pointee_seed(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("pointer", domain="pointer", spelling="const void *"),
        "T\tpointer\tconst void *\t\tconst void",
    )
    mapping = {
        "original": {"key": "pointer"},
        "current": {"key": "pointer"},
        "basis": {
            "kind": "decorated-export",
            "reference": "original export",
            "reason": "opaque API buffer",
        },
    }
    assert scalar.harvest_declaration_evidence(facts, facts, [mapping])["claims"] == []
    with pytest.raises(ValueError, match="concrete pointee"):
        evidence(
            scalar,
            tmp_path,
            facts,
            {
                **mapping["basis"],
                "key": "pointer",
                "property": "pointee",
                "value": "const void",
                "basis": mapping["basis"],
            },
        )


def test_narrowing_is_independent_storage_but_widening_consumes_signedness(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("wide"),
        declaration("narrow", width=16, spelling="short"),
        declaration("widened"),
        "F\tnarrow\twide\tassignment\tx.cpp\t1\t1",
        "F\twidened\tnarrow\tassignment\tx.cpp\t2\t1",
    )
    by_member = {
        member: item
        for item in scalar.component_report(facts, [claim("wide")], "signedness")
        for member in item["members"]
    }
    # Truncation does not care about the producer's signedness.
    assert by_member["wide"]["status"] == "candidate"
    assert by_member["wide"]["changes"] == ["wide"]
    assert by_member["narrow"]["status"] == "unknown"
    narrow = claim("narrow")
    narrow["basis"] |= {"value_width": 16, "operand_width": 16}
    result = next(
        item
        for item in scalar.component_report(facts, [narrow], "signedness")
        if "narrow" in item["members"]
    )
    # Sign- versus zero-extension of the narrow producer changes `widened`.
    assert result["status"] == "blocked"
    assert any("widening" in blocker["reason"] for blocker in result["blockers"])


@pytest.mark.parametrize("units", [None, "src/wiz8/test.cpp\n", "src/wiz8/missing.cpp\n"])
def test_width_boundary_requires_machine_checked_source_coverage(scalar, tmp_path, units):
    rows = [
        declaration("A", kind="field", width=16, spelling="short"),
        "M\tsrc/wiz8/test.cpp",
    ]
    if units is not None:
        (tmp_path / "expected-units.txt").write_text(units)
    facts = read(scalar, tmp_path, *rows)
    seed = claim("A", "width", 32, complete_storage_accesses=True)
    result = property_report(scalar, facts, [seed], "width")
    if units == "src/wiz8/test.cpp\n":
        assert result["status"] == "candidate", result
    else:
        assert result["status"] == "blocked"
        assert result["blockers"][0]["reason"].startswith("source boundary incomplete")


def record_rows(*fields, size=96, align=32, pack=0, natural=96):
    rows = [f"REC\tr\tsrc/wiz8/test.cpp\t1\tRecord\t{size}\t{align}\t{natural}\t{pack}"]
    for name, offset, bits, field_align, type_ in fields:
        rows.append(f"RF\tr\tr:{name}\t{name}\t{offset}\t{bits}\t{field_align}\t{type_}")
    return rows


def test_padding_removal_requires_unchanged_layout_and_no_use(scalar, tmp_path):
    source = "struct Record {\n    char tag;\n    char padding_1[3]; // +0x01\n    int value;\n};\n"
    (tmp_path / "test.cpp").write_text(source)
    start = source.index("char padding_1")
    facts = read(
        scalar,
        tmp_path,
        *record_rows(
            ("tag", 0, 8, 8, "char"),
            ("padding_1", 8, 24, 8, "char[3]"),
            ("value", 32, 32, 32, "int"),
            ("pad_tail", 64, 8, 8, "char"),
            size=96,
        ),
        span("r:padding_1", "test.cpp", start, "char padding_1[3];", source, "field-declaration"),
        span("r:pad_tail", "test.cpp", 0, "struct", source, "field-declaration"),
    )
    rows = {row["name"]: row for row in scalar.padding_report(facts)}
    assert rows["padding_1"]["status"] == "candidate"
    assert "record size" in rows["pad_tail"]["reason"]
    patch = tmp_path / "recovery.patch"
    assert not scalar.write_recovery_patch(facts, [], tmp_path, patch)["edits"]
    scalar.write_recovery_patch(facts, [], tmp_path, patch, padding=True)
    assert "-    char padding_1[3]; // +0x01\n" in patch.read_text()
    # Another TU compiling a member type under a different pack has no single replay.
    facts.record_fields.add(("r", "r:value", "value", "32", "32", "8", "int"))
    assert (
        "differs across"
        in {row["name"]: row for row in scalar.padding_report(facts)}["padding_1"]["reason"]
    )
    facts.record_fields.discard(("r", "r:value", "value", "32", "32", "8", "int"))
    facts.field_references.add(scalar.Use("r:padding_1", "reference", "test.cpp", 9, 1))
    assert {row["name"]: row for row in scalar.padding_report(facts)}["padding_1"][
        "reason"
    ] == "member is referenced"


def array_rows(extent="30", element_type="int", bits=32):
    return [
        declaration("a::element", kind="array-element", width=bits, spelling=element_type),
        f"ARR\ta\tsrc/wiz8/test.cpp\t1\t1\ttable\t{element_type}\t{extent}\t{bits}\ta::element",
    ]


def extent_claim(value, **extra):
    return independent_claim(
        "a::element",
        "extent",
        value,
        **extra,
    ) | {
        "basis": {
            "kind": "source-oracle",
            "reference": "released header",
            "reason": "declaration contract",
            "extent_kind": "declaration-contract",
        }
    }


@pytest.mark.parametrize(
    "extent_text,use,blocked",
    [
        ("30", None, False),
        ("TABLE_SIZE", None, True),
        ("30", "sizeof", True),
        ("30", "index:25", True),
        ("30", "indexed", True),
    ],
)
def test_extent_recovery_requires_literal_and_complete_census(
    scalar, tmp_path, extent_text, use, blocked
):
    source = f"int table[{extent_text}];\n"
    (tmp_path / "test.cpp").write_text(source)
    rows = [
        *array_rows(),
        span("a::element", "test.cpp", 10, extent_text, source, "array-extent"),
    ]
    if use:
        rows.append(f"AU\ta\t{use}\tsrc/wiz8/test.cpp\t3\t1")
    facts = read(scalar, tmp_path, *rows)
    claims = evidence(scalar, tmp_path, facts, extent_claim(25))
    result = scalar.extent_report(facts, claims)[0]
    assert result["status"] == ("blocked" if blocked else "candidate"), result
    if not blocked:
        scalar.write_recovery_patch(facts, claims, tmp_path, tmp_path / "recovery.patch")
        assert "+int table[25];" in (tmp_path / "recovery.patch").read_text()


def test_retail_extent_requires_reviewed_storage_census(scalar, tmp_path):
    facts = read(scalar, tmp_path, *array_rows())
    retail = {
        "key": "a::element",
        "property": "extent",
        "value": 25,
        "basis": {
            "kind": "retail",
            "reference": "retail-sha256:address",
            "reason": "loop bound",
            "extent_kind": "indexing-range",
        },
    }
    with pytest.raises(ValueError, match="census"):
        evidence(scalar, tmp_path, facts, retail)
    assert evidence(scalar, tmp_path, facts, retail | {"complete_storage_accesses": True})


@pytest.mark.parametrize(
    "kind,role,blocked",
    [
        ("char", "api:strcpy#0", False),
        ("raw-byte", "api:memcpy#0", False),
        ("wchar_t", "api:lstrcpyW#0", True),
        ("type:unsigned short", "call:LoadName#0", True),
    ],
)
def test_character_recovery_follows_every_storage_consumer(scalar, tmp_path, kind, role, blocked):
    facts = read(
        scalar,
        tmp_path,
        *array_rows("64", "unsigned char", 8),
        f"CH\ta\t{kind}\t{role}\tsrc/wiz8/test.cpp\t4\t1",
        "A\ta::element\tarray storage argument\tsrc/wiz8/test.cpp\t4\t1",
    )
    seed = independent_claim("a::element", "character", "char")
    result = scalar.array_report(facts, [seed])["character_components"][0]
    assert result["status"] == ("blocked" if blocked else "candidate"), result


@pytest.mark.parametrize("body,blocked", [(True, False), (False, True)])
def test_receiver_passed_to_collected_body_does_not_escape(scalar, tmp_path, body, blocked):
    rows = [
        declaration("field", kind="field"),
        "A\tfield\taggregate storage argument@x.cpp:1:1:function:Owner::Update\tx.cpp\t2\t1",
    ]
    if body:
        rows.append("HB\tx.cpp:1:1:function:Owner::Update")
    facts = read(scalar, tmp_path, *rows)
    assert [use.detail for use in facts.escapes] == ([] if body else ["aggregate storage argument"])
    result = property_report(scalar, facts, [claim("field")])
    assert result["status"] == ("blocked" if blocked else "candidate")


@pytest.mark.parametrize(
    "rows,escapes",
    [
        (["HB\tBase::Draw", "OV\tBase::Draw\tDerived::Draw", "HB\tDerived::Draw"], False),
        (["PV\tBase::Draw", "OV\tBase::Draw\tDerived::Draw", "HB\tDerived::Draw"], False),
        (["HB\tBase::Draw", "OV\tBase::Draw\tDerived::Draw"], True),
        ([], True),
    ],
)
def test_virtual_receiver_escape_requires_every_override_body(scalar, tmp_path, rows, escapes):
    facts = read(
        scalar,
        tmp_path,
        declaration("field", kind="field"),
        "A\tfield\taggregate storage argument@virtual:Base::Draw\tx.cpp\t2\t1",
        *rows,
    )
    assert bool(facts.escapes) == escapes


def test_signedness_conversion_is_not_signedness_equality(scalar, tmp_path):
    facts = read(
        scalar,
        tmp_path,
        declaration("count", kind="field"),
        declaration("Count", kind="function", signedness="unsigned", spelling="unsigned long"),
        "H\tCount\tbody\tx.cpp\t1\t1",
        "F\tCount\tcount\treturn\tx.cpp\t2\t1",
    )
    getter = independent_claim("Count", "signedness", "unsigned")
    rows = scalar.component_report(facts, [getter], "signedness")
    assert not any(row["changes"] for row in rows)
    assert {"count", "Count"} <= next(
        set(row["members"]) for row in scalar.component_report(facts, [], "width")
    )


def test_source_enum_patch_uses_existing_snapshot_and_shared_atom_guards(scalar, tmp_path):
    source = "int first;\nint second;\n"
    (tmp_path / "test.cpp").write_text(source)
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration("first"),
        declaration("second"),
        "F\tfirst\towner\tinitializer\ttest.cpp\t1\t1",
        "F\tsecond\tfirst\tassignment\ttest.cpp\t2\t1",
        span("first", "test.cpp", 0, "int", source),
        span("second", "test.cpp", source.index("int second"), "int", source),
    )
    patch = tmp_path / "recovery.patch"
    result = scalar.write_recovery_patch(
        facts, [], tmp_path, patch, propagate_enums=["W8Condition"]
    )
    assert result["changed_declarations"] == ["first", "second"]
    assert patch.read_text().count("+W8Condition") == 2
    assert (tmp_path / "test.cpp").read_text() == source
    (tmp_path / "test.cpp").write_text(source + "// new revision\n")
    with pytest.raises(ValueError, match="stale source facts"):
        scalar.write_recovery_patch(facts, [], tmp_path, patch, propagate_enums=["W8Condition"])
    (tmp_path / "test.cpp").write_text(source)
    facts.spans = {item for item in facts.spans if item[0] != "second"}
    result = scalar.write_recovery_patch(
        facts, [], tmp_path, patch, propagate_enums=["W8Condition"]
    )
    assert not result["changed_declarations"]
    assert "every changed declaration" in result["rejected"][0]["reason"]


def test_source_enum_patch_applies_to_filename_with_spaces(scalar, tmp_path):
    source = "int copy;\n"
    name = "Combat Range.cpp"
    (tmp_path / name).write_text(source)
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration("copy"),
        "F\tcopy\towner\tinitializer\ttest.cpp\t1\t1",
        span("copy", name, 0, "int", source),
    )
    patch = tmp_path / "recovery.patch"
    scalar.write_recovery_patch(facts, [], tmp_path, patch, propagate_enums=["W8Condition"])
    subprocess.run(
        ["patch", "--batch", "-p1", "-i", str(patch)],
        cwd=tmp_path,
        check=True,
        capture_output=True,
        text=True,
    )
    assert (tmp_path / name).read_text() == "W8Condition copy;\n"


@pytest.mark.parametrize("kind", ["sign", "wide-consumer"])
def test_source_enum_patches_preserve_numeric_conversion_behavior(scalar, tmp_path, kind):
    rows = [
        declaration("consumer", signedness="unsigned" if kind == "sign" else "signed"),
        "F\tconsumer\towner\tinitializer\tsrc/wiz8/test.cpp\t2\t1",
    ]
    if kind == "wide-consumer":
        rows += [
            declaration("wide", width=64),
            "F\twide\tconsumer\tassignment\tsrc/wiz8/test.cpp\t3\t1",
        ]
    facts = source_enum_facts(scalar, tmp_path, *rows)
    assert all(
        row["status"] == "blocked"
        for row in scalar.enum_propagation_report(facts, ["W8Condition"])["proposals"]
    )


@pytest.mark.parametrize("boundary", ["global", "overload"])
def test_source_enum_preserves_globals_and_overload_resolution(scalar, tmp_path, boundary):
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration("copy"),
        "F\tcopy\towner\tinitializer\tsrc/wiz8/test.cpp\t2\t1",
    )
    if boundary == "global":
        facts.linkage["copy"] = ("internal", "none")
    else:
        facts.declarations["callee:parameter:#0"] = scalar.DeclarationFact(
            "callee:parameter:#0",
            "test.cpp",
            1,
            1,
            "parameter",
            "p",
            False,
            False,
            32,
            "signed",
            "integer",
            "int",
        )
        facts.signatures.update(
            {"callee": ("Overload", 1, 0, False, "one"), "other": ("Overload", 1, 0, False, "two")}
        )
        facts.flows.add(scalar.Flow("callee:parameter:#0", "copy", "argument", "test.cpp", 2, 1))
    rows = scalar.enum_propagation_report(facts, ["W8Condition"])["proposals"]
    assert all(row["status"] == "blocked" for row in rows)


def test_source_enum_patch_rejects_stale_owner_even_when_local_is_unchanged(scalar, tmp_path):
    source = "int copy;\n"
    owner = "W8Condition owner;\n"
    (tmp_path / "test.cpp").write_text(source)
    (tmp_path / "owner.h").write_text("int owner;\n")
    facts = source_enum_facts(
        scalar,
        tmp_path,
        declaration("copy"),
        "F\tcopy\towner\tinitializer\ttest.cpp\t1\t1",
        span("copy", "test.cpp", 0, "int", source),
        span("owner", "owner.h", 0, "W8Condition", owner),
    )
    with pytest.raises(ValueError, match="stale source facts for owner.h"):
        scalar.write_recovery_patch(
            facts, [], tmp_path, tmp_path / "recovery.patch", propagate_enums=["W8Condition"]
        )


def boolean_expression_facts(scalar, tmp_path, text, observations, *, owner=None):
    path = tmp_path / "src/wiz8/test.cpp"
    path.parent.mkdir(parents=True)
    path.write_text(text)
    raw = path.read_bytes()
    facts = scalar.ScalarFacts()
    for name, domain, operation, value, occurrence in observations:
        facts.declarations[name] = scalar.DeclarationFact(
            name,
            "owner.h",
            1,
            1,
            "field",
            name,
            False,
            False,
            8,
            "irrelevant" if domain == "bool" else "unsigned",
            domain,
            "bool" if domain == "bool" else "unsigned char",
        )
        offsets = [m.start() for m in re.finditer(operation.encode(), raw)]
        offset = offsets[occurrence]
        line = raw[:offset].count(b"\n") + 1
        column = offset - raw.rfind(b"\n", 0, offset)
        facts.operands.add((name, operation, value, "src/wiz8/test.cpp", line, column))
    facts.spans.add(
        ("snapshot", "type", "src/wiz8/test.cpp", 0, 0, "", hashlib.sha256(raw).hexdigest())
    )
    if owner is not None:
        (tmp_path / "owner.h").write_text(owner)
        facts.spans.add(
            ("owner", "type", "owner.h", 0, 4, "bool", hashlib.sha256(owner.encode()).hexdigest())
        )
    return facts


def test_boolean_patch_uses_canonical_bool_fields_and_globals(scalar, tmp_path):
    text = "if (gXStatus.fCombatMode == 0 || g_preserve == 0 || bytes.fCombatMode == 0) {}\n"
    facts = boolean_expression_facts(
        scalar,
        tmp_path,
        text,
        [
            ("combat", "bool", "==", 0, 0),
            ("preserve", "bool", "==", 0, 1),
            ("byte", "character", "==", 0, 2),
        ],
    )
    patch = tmp_path / "recovery.patch"
    report = scalar.write_recovery_patch(facts, [], tmp_path, patch, boolean_expressions=True)
    assert (
        "+if (!gXStatus.fCombatMode || !g_preserve || bytes.fCombatMode == 0) {}"
        in patch.read_text()
    )
    assert len(report["changed_expressions"]) == 2
    assert (tmp_path / "src/wiz8/test.cpp").read_text() == text


@pytest.mark.parametrize(
    "operator,literal,value,expected",
    [
        ("==", "0", 0, "!obj.flag"),
        ("!=", "0u", 0, "obj.flag"),
        ("==", "true", 1, "obj.flag"),
        ("!=", "1", 1, "!obj.flag"),
    ],
)
def test_boolean_patch_preserves_truth_and_single_evaluation(
    scalar, tmp_path, operator, literal, value, expected
):
    text = f"if (obj.flag {operator} {literal}) {{}}\n"
    facts = boolean_expression_facts(scalar, tmp_path, text, [("flag", "bool", operator, value, 0)])
    patch = tmp_path / "recovery.patch"
    scalar.write_recovery_patch(facts, [], tmp_path, patch, boolean_expressions=True)
    assert f"+if ({expected}) {{}}" in patch.read_text()


def test_boolean_patch_handles_reversed_and_parenthesized_objects(scalar, tmp_path):
    text = "if (0 == array[i++].flag || (obj.flag) != false) {}\n"
    facts = boolean_expression_facts(scalar, tmp_path, text, [])
    facts.declarations["flag"] = scalar.DeclarationFact(
        "flag", "owner.h", 1, 1, "field", "flag", False, False, 8, "irrelevant", "bool", "bool"
    )
    facts.operands.update(
        {
            ("flag", "rhs:==", 0, "src/wiz8/test.cpp", 1, 7),
            ("flag", "!=", 0, "src/wiz8/test.cpp", 1, text.index("!=") + 1),
        }
    )
    patch = tmp_path / "recovery.patch"
    scalar.write_recovery_patch(facts, [], tmp_path, patch, boolean_expressions=True)
    assert "+if (!array[i++].flag || (obj.flag)) {}" in patch.read_text()


@pytest.mark.parametrize(
    "text,operation,value",
    [
        ("if (obj.flag == 0 * 2) {}\n", "==", 0),
        ("if (obj.flag == 0 + 1) {}\n", "==", 1),
        ("if (1 - 1 == obj.flag) {}\n", "rhs:==", 0),
        ("if (make()->flag == 0) {}\n", "==", 0),
        ("if (flags[i > 0] == 0) {}\n", "==", 0),
    ],
)
def test_boolean_patch_leaves_arithmetic_and_unfamiliar_spelling(
    scalar, tmp_path, text, operation, value
):
    plain_operator = operation.removeprefix("rhs:")
    facts = boolean_expression_facts(
        scalar, tmp_path, text, [("flag", "bool", plain_operator, value, 0)]
    )
    if operation.startswith("rhs:"):
        facts.operands = {
            (key, operation, v, f, line, col) for key, _, v, f, line, col in facts.operands
        }
    patch = tmp_path / "recovery.patch"
    scalar.write_recovery_patch(facts, [], tmp_path, patch, boolean_expressions=True)
    assert patch.read_text() == ""


def test_boolean_patch_rejects_stale_owner_header(scalar, tmp_path):
    facts = boolean_expression_facts(
        scalar,
        tmp_path,
        "if (obj.flag == 0) {}\n",
        [("flag", "bool", "==", 0, 0)],
        owner="bool flag;\n",
    )
    (tmp_path / "owner.h").write_text("unsigned char flag;\n")
    with pytest.raises(ValueError, match="stale source facts"):
        scalar.write_recovery_patch(
            facts, [], tmp_path, tmp_path / "patch", boolean_expressions=True
        )


def test_boolean_patch_ignores_unresolved_operand_identity(scalar, tmp_path):
    facts = boolean_expression_facts(scalar, tmp_path, "if (obj.flag == 0) {}\n", [])
    facts.operands.add(("", "==", 0, "src/wiz8/test.cpp", 1, 14))
    patch = tmp_path / "recovery.patch"
    scalar.write_recovery_patch(facts, [], tmp_path, patch, boolean_expressions=True)
    assert patch.read_text() == ""


@pytest.mark.parametrize("kind", ["parameter", "variable", "field", "function"])
def test_boolean_literal_patch_uses_destination_identity(scalar, tmp_path, kind):
    text = "SetEnabled(1); ByteEnabled(1); SetEnabled(0 + 1);\n"
    facts = boolean_expression_facts(scalar, tmp_path, text, [])
    facts.declarations["enabled"] = scalar.DeclarationFact(
        "enabled", "owner.h", 1, 1, kind, "enabled", False, False, 8, "unsigned", "bool", "bool"
    )
    facts.declarations["byte"] = scalar.DeclarationFact(
        "byte",
        "owner.h",
        2,
        1,
        kind,
        "enabled",
        False,
        False,
        8,
        "unsigned",
        "integer",
        "unsigned char",
    )
    for key, spelling in [
        ("enabled", "SetEnabled(1"),
        ("byte", "ByteEnabled(1"),
        ("enabled", "SetEnabled(0"),
    ]:
        offset = text.index(spelling) + len(spelling) - 1
        facts.constant_locations.add((key, 1, "src/wiz8/test.cpp", 1, offset + 1))
    report = scalar.write_recovery_patch(
        facts, [], tmp_path, tmp_path / "recovery.patch", boolean_expressions=True
    )
    patch = (tmp_path / "recovery.patch").read_text()
    assert "+SetEnabled(true); ByteEnabled(1); SetEnabled(0 + 1);" in patch
    assert len(report["changed_expressions"]) == 1


def test_scalar_reader_retains_constant_producer_locations(scalar, tmp_path):
    facts = read(
        scalar, tmp_path, declaration("enabled"), "K\tenabled\t1\tsrc/wiz8/test.cpp\t12\t9"
    )
    assert facts.constant_locations == {("enabled", 1, "src/wiz8/test.cpp", 12, 9)}


@pytest.mark.parametrize("expression,value", [("0", 0), ("1u", 1), ("1 + 0", 1), ("1.0", 1)])
def test_boolean_literal_patch_requires_complete_literal(scalar, tmp_path, expression, value):
    text = "SetEnabled(" + expression + ");\n"
    facts = boolean_expression_facts(scalar, tmp_path, text, [])
    facts.declarations["enabled"] = scalar.DeclarationFact(
        "enabled",
        "owner.h",
        1,
        1,
        "parameter",
        "enabled",
        False,
        False,
        8,
        "unsigned",
        "bool",
        "bool",
    )
    facts.constant_locations.add(("enabled", value, "src/wiz8/test.cpp", 1, 12))
    report = scalar.write_recovery_patch(
        facts, [], tmp_path, tmp_path / "recovery.patch", boolean_expressions=True
    )
    assert len(report["changed_expressions"]) == int(expression in {"0", "1u"})


def test_boolean_literal_patch_blocks_shared_nonbool_owner(scalar, tmp_path):
    facts = boolean_expression_facts(scalar, tmp_path, "SetEnabled(1);\n", [])
    for key, domain in [("boolean", "bool"), ("integer", "integer")]:
        facts.declarations[key] = scalar.DeclarationFact(
            key,
            "owner.h",
            1,
            1,
            "parameter",
            "enabled",
            False,
            False,
            8,
            "unsigned",
            domain,
            domain,
        )
        facts.constant_locations.add((key, 1, "src/wiz8/test.cpp", 1, 12))
    report = scalar.write_recovery_patch(
        facts, [], tmp_path, tmp_path / "recovery.patch", boolean_expressions=True
    )
    assert report["changed_expressions"] == []


@pytest.mark.parametrize(
    "kind,text,column,expected",
    [
        ("field", "obj.flag = 1;\n", 10, "obj.flag = true;"),
        ("function", "return 0;\n", 1, "return false;"),
        ("variable", "bool flag = 1;\n", 6, "bool flag = true;"),
    ],
)
def test_boolean_literal_patch_understands_producer_locations(
    scalar, tmp_path, kind, text, column, expected
):
    facts = boolean_expression_facts(scalar, tmp_path, text, [])
    value = int("1" in text)
    facts.declarations["flag"] = scalar.DeclarationFact(
        "flag", "owner.h", 1, 1, kind, "flag", False, False, 8, "unsigned", "bool", "bool"
    )
    facts.constant_locations.add(("flag", value, "src/wiz8/test.cpp", 1, column))
    report = scalar.write_recovery_patch(
        facts, [], tmp_path, tmp_path / "recovery.patch", boolean_expressions=True
    )
    assert len(report["changed_expressions"]) == 1
    assert "+" + expected in (tmp_path / "recovery.patch").read_text()


@pytest.mark.parametrize("text", ["obj.flag = 1 + 0;\n", "obj.flag = ONE;\n", "return (0);\n"])
def test_boolean_producer_patch_leaves_nonliteral_expressions(scalar, tmp_path, text):
    facts = boolean_expression_facts(scalar, tmp_path, text, [])
    kind = "function" if text.startswith("return") else "field"
    facts.declarations["flag"] = scalar.DeclarationFact(
        "flag", "owner.h", 1, 1, kind, "flag", False, False, 8, "unsigned", "bool", "bool"
    )
    facts.constant_locations.add(
        (
            "flag",
            int(not text.startswith("return")),
            "src/wiz8/test.cpp",
            1,
            1 if kind == "function" else 10,
        )
    )
    report = scalar.write_recovery_patch(
        facts, [], tmp_path, tmp_path / "recovery.patch", boolean_expressions=True
    )
    assert report["changed_expressions"] == []


@pytest.mark.parametrize(
    "text,operator,value,expected",
    [
        ("if (prop->HasSupportedItems() != 0) {}\n", "!=", 0, "prop->HasSupportedItems()"),
        ("if (IsReady() == 0) {}\n", "==", 0, "!IsReady()"),
        ("if ((obj.IsReady()) == 1) {}\n", "==", 1, "(obj.IsReady())"),
        ("if (0 == obj.IsReady()) {}\n", "rhs:==", 0, "!obj.IsReady()"),
    ],
)
def test_boolean_patch_handles_ast_resolved_no_argument_calls(
    scalar, tmp_path, text, operator, value, expected
):
    from dataclasses import replace

    facts = boolean_expression_facts(
        scalar, tmp_path, text, [("predicate", "bool", operator.removeprefix("rhs:"), value, 0)]
    )
    facts.declarations["predicate"] = replace(facts.declarations["predicate"], kind="function")
    if operator.startswith("rhs:"):
        facts.operands = {
            (key, operator, val, file, line, col) for key, _, val, file, line, col in facts.operands
        }
    edits = scalar.boolean_expression_edits(
        facts, lambda file, digest: (tmp_path / file).read_bytes()
    )
    assert len(edits) == 1
    assert edits[0][-1].decode() == expected


@pytest.mark.parametrize(
    "text,domain",
    [
        ("if (BytePredicate() != 0) {}\n", "character"),
        ('if (BoolPredicate("argument") != 0) {}\n', "bool"),
        ("if (BoolPredicate() != 0 + extra) {}\n", "bool"),
    ],
)
def test_boolean_call_patch_keeps_nonbool_and_unfamiliar_expressions(
    scalar, tmp_path, text, domain
):
    from dataclasses import replace

    facts = boolean_expression_facts(scalar, tmp_path, text, [("predicate", domain, "!=", 0, 0)])
    facts.declarations["predicate"] = replace(facts.declarations["predicate"], kind="function")
    edits = scalar.boolean_expression_edits(
        facts, lambda file, digest: (tmp_path / file).read_bytes()
    )
    assert edits == []


@pytest.mark.parametrize(
    "text,operation,value,expected",
    [
        ("if (obj.Contains(point) != 0) {}\n", "!=", 0, "obj.Contains(point)"),
        ("if (CanReach(&nodes[i++].position, target, radius) == 0) {}\n", "==", 0,
         "!CanReach(&nodes[i++].position, target, radius)"),
        ("if ((obj.Contains(GetPosition(index))) == 1) {}\n", "==", 1,
         "(obj.Contains(GetPosition(index)))"),
        ("if (0 == obj.Contains(GetPosition(index))) {}\n", "rhs:==", 0,
         "!obj.Contains(GetPosition(index))"),
        ("if (obj.Contains(static_cast<int>(value)) != 0) {}\n", "!=", 0,
         "obj.Contains(static_cast<int>(value))"),
    ],
)
def test_boolean_patch_preserves_arguments_of_resolved_calls(
    scalar, tmp_path, text, operation, value, expected
):
    from dataclasses import replace

    facts = boolean_expression_facts(
        scalar, tmp_path, text, [("predicate", "bool", operation.removeprefix("rhs:"), value, 0)]
    )
    facts.declarations["predicate"] = replace(facts.declarations["predicate"], kind="function")
    if operation.startswith("rhs:"):
        facts.operands = {
            (key, operation, val, file, line, col) for key, _, val, file, line, col in facts.operands
        }
    edits = scalar.boolean_expression_edits(facts, lambda file, digest: (tmp_path / file).read_bytes())
    assert len(edits) == 1
    assert edits[0][-1].decode() == expected


@pytest.mark.parametrize("arguments", ["/* comment */ point", "{1, 2}", "a / b", '"text"'])
def test_boolean_call_patch_rejects_unmodeled_argument_syntax(scalar, tmp_path, arguments):
    from dataclasses import replace

    text = f"if (BoolPredicate({arguments}) != 0) {{}}\n"
    facts = boolean_expression_facts(scalar, tmp_path, text, [("predicate", "bool", "!=", 0, 0)])
    facts.declarations["predicate"] = replace(facts.declarations["predicate"], kind="function")
    assert scalar.boolean_expression_edits(facts, lambda file, digest: (tmp_path / file).read_bytes()) == []
