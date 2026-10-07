"""Storage ownership uses compiler facts; no test needs a parallel C++ parser."""

import json

import pytest
from wiz8decomp.global_model import (
    global_violations,
    indexed_global_definitions,
    overlapping_globals,
)
from wiz8decomp.source_index import SourceIndexError


def block(name, address=0x1000, size=16, *, target="WIZ8", definition=True, comments=()):
    return {
        "source_file": f"src/wiz8/{name}.cpp",
        "comments": [
            {"text": text, "line": i + 1, "column": 1, "offset": i * 40}
            for i, text in enumerate((f"// GLOBAL: {target} 0x{address:x}", *comments))
        ],
        "anchor": {
            "line": 4,
            "column": 1,
            "candidates": [
                {
                    "kind": "variable",
                    "semantic_id": name,
                    "qualified_name": name,
                    "is_definition": definition,
                    "line": 4,
                    "end_line": 4,
                    "type": "unsigned char[16]",
                    "size": size,
                }
            ],
        },
    }


def document(*blocks, variables=()):
    return {"marker_blocks": list(blocks), "variables": list(variables)}


def check(tmp_path, doc):
    path = tmp_path / "build/source-index.json"
    path.parent.mkdir(exist_ok=True)
    path.write_text(json.dumps(doc))
    return global_violations(tmp_path)


@pytest.mark.parametrize(
    "offset,size,expected",
    [(0, 4, True), (8, 32, True), (15, 1, True), (16, 4, False), (20, 4, False), (0, None, True)],
)
def test_overlap_uses_compiler_extent(offset, size, expected):
    rows = indexed_global_definitions(
        document(block("outer"), block("inner", 0x1000 + offset, size))
    )
    assert bool(overlapping_globals(rows)) == expected


def test_targets_externs_aliases_and_repeated_header_observations_are_distinct():
    owner = block("owner")
    rows = indexed_global_definitions(
        document(
            owner,
            owner,
            block("other", target="SURRENDER"),
            block("extern", definition=False),
            block("alias", comments=("// offset alias of owner",)),
        )
    )
    assert len(rows) == 2
    assert not overlapping_globals(rows)


def test_unknown_size_does_not_invent_an_extent():
    rows = indexed_global_definitions(
        document(block("unknown", size=None), block("nearby", 0x1001))
    )
    assert not overlapping_globals(rows)


def test_index_must_have_compiler_storage_facts():
    owner = block("owner")
    del owner["anchor"]["candidates"][0]["type"]
    with pytest.raises(SourceIndexError, match="storage facts"):
        indexed_global_definitions(document(owner))


@pytest.mark.parametrize(
    "text,definition,expected",
    [
        ("// GLOBAL", True, True),
        ("// GLOBAL: WIZ8 unresolved", True, False),
        ("// GLOBAL", False, False),
        ("// Global variables", True, False),
    ],
)
def test_unaddressed_markers_use_compiler_definition_status(tmp_path, text, definition, expected):
    owner = block("owner", definition=definition)
    owner["comments"][0]["text"] = text
    assert bool(check(tmp_path, document(owner))) == expected


def test_unmarked_internal_address_owner_is_rejected(tmp_path):
    variable = {
        "qualified_name": "hidden_00001000",
        "source_file": "src/wiz8/hidden.cpp",
        "line": 9,
        "target": "WIZ8",
        "definition_kind": "definition",
        "linkage": "internal",
    }
    violations = check(tmp_path, document(block("owner"), variables=[variable, variable]))
    assert len(violations) == 1
    assert violations[0]["kind"] == "global-address-shadow"
