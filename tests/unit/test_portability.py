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
