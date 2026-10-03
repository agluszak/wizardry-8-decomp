"""Contracts for shared facts and report-only, evidence-seeded type recovery."""

from __future__ import annotations

import importlib
import json
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
    return scalar.integer_report(facts, list(claims))["components"][0]["properties"][property_]


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
        "width-changing conversion",
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
        declaration("API", kind="function", signedness="unsigned"),
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
    assert result["changes"] == ["local"]


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
    assert report["components"][0]["properties"]["signedness"]["status"] == "unknown"


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
