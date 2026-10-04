from __future__ import annotations

import json
from pathlib import Path

from wiz8decomp.reports.portability import portability_queues, strip_recovery_markers
from wiz8decomp.reports.semantic_debt import _ADDRESS_SUFFIX


def test_scan_preserves_locations_and_ignores_quoted_and_commented_api_names(tmp_path: Path):
    source = tmp_path / "include/wiz8/test.h"
    source.parent.mkdir(parents=True)
    source.write_text(
        "/*\nHWND fake; __asm { cpuid }\n*/\n"
        'const char* url = "http://HWND/__asm";\n'
        "long actual;\nHWND window;\n"
        "#pragma pack(push, 1)\n"
        'static_assert(sizeof(Record) == 8, "record");\n'
        "void body() { __asm { fistp result } }\n"
    )
    result = portability_queues(tmp_path, {})
    assert [(row["kind"], row["line"]) for row in result["abi_width_sites"]] == [("long", 5)]
    assert [(row["kind"], row["line"]) for row in result["platform_dependency_sites"]] == [
        ("window_events", 6)
    ]
    assert result["public_headers"][0]["platform_tokens"] == ["HWND"]
    assert result["inline_assembly_blocks"][0]["instruction_families"] == [
        "x87_rounding_or_control_word"
    ]
    assert result["inline_assembly_blocks"][0]["line"] == 9
    assert [(row["record"], row["line"]) for row in result["layout_candidates"]] == [("Record", 8)]


def test_review_does_not_attach_to_a_same_named_type_at_another_owner(tmp_path: Path):
    config = tmp_path / "config/pre-portability.json"
    config.parent.mkdir()
    config.write_text(
        json.dumps(
            {
                "layouts": [
                    {
                        "record": "Record",
                        "source_file": "include/wiz8/canonical.h",
                        "classification": "disk-format",
                    }
                ]
            }
        )
    )
    source = tmp_path / "include/wiz8/other.h"
    source.parent.mkdir(parents=True)
    source.write_text('static_assert(sizeof(Record) == 8, "size");\n')
    result = portability_queues(tmp_path, {})
    assert result["layout_candidates"][0]["classification"] == "unclassified"
    assert result["layout_candidates"][0]["review"] is None


def test_pointer_fields_are_candidates_and_duplicate_tu_observations_are_deduplicated(
    tmp_path: Path,
):
    record = {
        "qualified_name": "Owner",
        "fields": [
            {"name": "value", "type": "Item *", "source_file": "include/wiz8/item.h", "line": 2}
        ],
    }
    result = portability_queues(tmp_path, {"classes": [record, record]})
    assert len(result["pointer_fields"]) == 1
    assert result["pointer_fields"][0]["ownership"] == "requires_review"


def test_included_layout_contract_uses_projected_declaration_owner(tmp_path: Path):
    config = tmp_path / "config/pre-portability.json"
    config.parent.mkdir()
    config.write_text(
        json.dumps(
            {
                "layouts": [
                    {
                        "record": "Record",
                        "source_file": "include/wiz8/canonical.h",
                        "classification": "mixed",
                    }
                ]
            }
        )
    )
    record = {
        "qualified_name": "Record",
        "source_file": "include/wiz8/canonical.h",
        "line": 3,
        "asserted_size": 12,
        "fields": [],
    }
    other = {**record, "source_file": "include/wiz8/other.h"}
    unasserted = {**record, "qualified_name": "Unasserted", "asserted_size": None}
    result = portability_queues(tmp_path, {"classes": [record, record, other, unasserted]})
    layouts = result["layout_candidates"]
    assert len(layouts) == 2
    assert layouts[0]["classification"] == "mixed"
    assert layouts[0]["evidence"] == "source_index_layout_contract"
    assert layouts[1]["classification"] == "unclassified"
    assert layouts[1]["review"] is None


def test_marker_stripping_preserves_behavior_prose_and_string_literals():
    text = '// FUNCTION: WIZ8 0x00401000\n// Retail bug: 0x00401000 leaks its allocation.\nconst char* s = "// GLOBAL: WIZ8 0x00600000";\nint value; // GLOBAL: WIZ8 0x00600000\n// reinterpret-ok: required external ABI storage\n'
    stripped = strip_recovery_markers(text)
    assert stripped == text[text.index("\n") :]


def test_semantic_hex_letters_are_not_address_suffixes():
    for name in ["ReadFace", "stSurface2D::stSurface2D", "TraceFace"]:
        assert not _ADDRESS_SUFFIX.fullmatch(name)
    assert _ADDRESS_SUFFIX.fullmatch("ReadThing_00401000")


def test_stripping_uses_reccmp_secondary_base_and_line_marker_grammar():
    text = "// VTABLE: SURRENDER 0x10076A40 srBinStream\n// LINE: WIZ8 0x00401000\n// Retail: secondary base has a separate virtual table.\n"
    assert (
        strip_recovery_markers(text)
        == "\n\n// Retail: secondary base has a separate virtual table.\n"
    )


def _lifetime_fixture(tmp_path: Path, *, field_type: str = "Item *"):
    import hashlib

    header = tmp_path / "include/wiz8/owner.h"
    header.parent.mkdir(parents=True)
    header.write_text("struct Owner { Item *value; };\n")
    implementation = tmp_path / "src/wiz8/owner.cpp"
    implementation.parent.mkdir(parents=True)
    implementation.write_text("void release(Owner *owner) { delete owner->value; }\n")
    config = {
        "lifetime_fields": [
            {
                "record": "Owner",
                "field": "value",
                "source_file": "include/wiz8/owner.h",
                "type": field_type,
                "ownership": "owned",
                "allocator": "new",
                "release": "delete",
                "contract": "Scalar-delete the owned object.",
                "source_evidence": ["include/wiz8/owner.h", "src/wiz8/owner.cpp"],
            }
        ],
        "lifetime_source_snapshots": {
            path.relative_to(tmp_path).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in [header, implementation]
        },
    }
    config_path = tmp_path / "config/pre-portability.json"
    config_path.parent.mkdir()
    config_path.write_text(json.dumps(config))
    record = {
        "qualified_name": "Owner",
        "fields": [
            {"name": "value", "type": field_type, "source_file": "include/wiz8/owner.h", "line": 1}
        ],
    }
    return config_path, record


def test_lifetime_contract_binds_to_exact_field_and_source_snapshot(tmp_path: Path):
    _, record = _lifetime_fixture(tmp_path)
    other = {**record, "qualified_name": "Other"}
    result = portability_queues(tmp_path, {"classes": [record, record, other]})
    assert len(result["pointer_fields"]) == 2
    owned, unresolved = result["pointer_fields"]
    assert owned["ownership"] == "owned"
    assert owned["lifetime_review"]["release"] == "delete"
    assert owned["lifetime_review"]["retail_equivalence"] == "not_established_by_this_report"
    assert unresolved["ownership"] == "requires_review"
    assert result["summary"]["pointer_fields_requiring_review"] == 1
    assert result["fork_status"] == "blocked"


def test_same_named_field_at_another_source_owner_is_not_reviewed(tmp_path: Path):
    _, record = _lifetime_fixture(tmp_path)
    record["fields"][0]["source_file"] = "include/wiz8/other.h"
    result = portability_queues(tmp_path, {"classes": [record]})
    assert result["pointer_fields"][0]["ownership"] == "requires_review"
    assert result["lifetime_field_reviews"][0]["status"] == "declaration_not_observed"
    assert result["fork_blockers"]["inactive_lifetime_reviews"] == 1


def test_lifetime_review_reopens_after_type_or_implementation_changes(tmp_path: Path):
    _, record = _lifetime_fixture(tmp_path)
    record["fields"][0]["type"] = "Other *"
    result = portability_queues(tmp_path, {"classes": [record]})
    assert result["lifetime_field_reviews"][0]["status"] == "declaration_changed"
    assert result["pointer_fields"][0]["ownership"] == "requires_review"
    record["fields"][0]["type"] = "Item *"
    (tmp_path / "src/wiz8/owner.cpp").write_text("void release(Owner *owner) {}\n")
    result = portability_queues(tmp_path, {"classes": [record]})
    assert result["lifetime_field_reviews"][0]["status"] == "source_evidence_changed"
    assert result["pointer_fields"][0]["ownership"] == "requires_review"


def test_conflicting_tu_declarations_do_not_select_first_lifetime_observation(tmp_path: Path):
    _, record = _lifetime_fixture(tmp_path)
    conflicting = {**record, "fields": [{**record["fields"][0], "type": "Other *"}]}
    for observations in [[record, conflicting], [conflicting, record]]:
        result = portability_queues(tmp_path, {"classes": observations})
        review = result["lifetime_field_reviews"][0]
        assert review["status"] == "conflicting_declarations"
        assert review["observed_types"] == ["Item *", "Other *"]
        assert result["pointer_fields"][0]["ownership"] == "requires_review"


def test_lifetime_review_can_cover_container_without_marking_elements_owned(tmp_path: Path):
    config_path, record = _lifetime_fixture(tmp_path, field_type="List")
    config = json.loads(config_path.read_text())
    config["lifetime_fields"][0]["ownership"] = "owned_container_borrowed_elements"
    config_path.write_text(json.dumps(config))
    result = portability_queues(tmp_path, {"classes": [record]})
    assert not result["pointer_fields"]
    review = result["lifetime_field_reviews"][0]
    assert review["status"] == "source_model_review"
    assert review["ownership"] == "owned_container_borrowed_elements"


def test_duplicate_lifetime_review_is_an_error(tmp_path: Path):
    import pytest

    config_path, record = _lifetime_fixture(tmp_path)
    config = json.loads(config_path.read_text())
    config["lifetime_fields"] *= 2
    config_path.write_text(json.dumps(config))
    with pytest.raises(ValueError, match="Duplicate lifetime field review"):
        portability_queues(tmp_path, {"classes": [record]})


def test_missing_or_unfrozen_source_evidence_keeps_review_open(tmp_path: Path):
    config_path, record = _lifetime_fixture(tmp_path)
    config = json.loads(config_path.read_text())
    del config["lifetime_source_snapshots"]["src/wiz8/owner.cpp"]
    config_path.write_text(json.dumps(config))
    result = portability_queues(tmp_path, {"classes": [record]})
    assert result["lifetime_field_reviews"][0]["status"] == "source_evidence_changed"
    assert result["pointer_fields"][0]["ownership"] == "requires_review"


def test_lifetime_review_must_freeze_its_declaration_owner(tmp_path: Path):
    config_path, record = _lifetime_fixture(tmp_path)
    config = json.loads(config_path.read_text())
    config["lifetime_fields"][0]["source_evidence"] = ["src/wiz8/owner.cpp"]
    config_path.write_text(json.dumps(config))
    result = portability_queues(tmp_path, {"classes": [record]})
    assert result["lifetime_field_reviews"][0]["status"] == "source_evidence_changed"
    assert result["pointer_fields"][0]["ownership"] == "requires_review"


def test_layout_reviews_with_same_name_keep_both_canonical_owners(tmp_path: Path):
    config = tmp_path / "config/pre-portability.json"
    config.parent.mkdir()
    reviews = [
        {"record": "Record", "source_file": "include/wiz8/a.h", "classification": "disk-format"},
        {"record": "Record", "source_file": "include/wiz8/b.h", "classification": "external ABI"},
    ]
    config.write_text(json.dumps({"layouts": reviews}))
    header = tmp_path / "include/wiz8/a.h"
    header.parent.mkdir(parents=True)
    header.write_text('static_assert(sizeof(Record) == 8, "size");\n')
    (header.parent / "b.h").write_text('static_assert(sizeof(Record) == 12, "size");\n')
    result = portability_queues(
        tmp_path,
        {
            "classes": [
                {
                    "qualified_name": "Record",
                    "source_file": row["source_file"],
                    "asserted_size": 8,
                    "fields": [],
                }
                for row in reviews
            ]
        },
    )
    assert [row["classification"] for row in result["layout_candidates"]] == [
        "disk-format",
        "external ABI",
    ]
