from pathlib import Path

from wiz8decomp.reports.semantic_debt import (
    _DISCLAIMED_SEMANTICS,
    _adjacent_comment,
    _adjacent_comment_text,
    _byte_strides,
    _duplicate_layouts,
    _enum_literal_arguments,
    _source_shaping_directives,
    _storage_debt,
    _unresolved_functions,
    _Usage,
)


def test_placeholder_body_remains_debt_and_definition_owns_the_location() -> None:
    declaration = {
        "qualified_name": "Function100035620",
        "semantic_id": "allocator",
        "source_file": "include/surrender/allocator.h",
        "line": 3,
        "target": "SURRENDER",
        "is_definition": False,
    }
    definition = {
        **declaration,
        "source_file": "src/surrender/global_recycler.cpp",
        "line": 54,
        "is_definition": True,
    }
    resolved = {**definition, "qualified_name": "AllocateRecyclerStorage", "semantic_id": "named"}
    for entries in ([declaration, definition], [definition, declaration]):
        rows = _unresolved_functions({"declarations": [*entries, resolved]}, "SURRENDER")
        assert len(rows) == 1
        assert rows[0]["source_file"] == definition["source_file"]
        assert rows[0]["is_definition"] is True
        assert _unresolved_functions({"declarations": entries}, "WIZ8") == []


def test_source_shaping_directives_are_target_scoped_and_ignore_plain_inline(
    tmp_path: Path,
) -> None:
    wiz8 = tmp_path / "src" / "wiz8"
    surrender = tmp_path / "src" / "surrender"
    wiz8.mkdir(parents=True)
    surrender.mkdir(parents=True)

    (wiz8 / "model.cpp").write_text(
        """
inline int ordinary() { return 1; }
__forceinline int forced() { return 2; }
__declspec(noinline) int blocked() { return 3; }
#pragma optimize("t", on)
int tuned() { return 4; }
""".lstrip(),
        encoding="utf-8",
    )
    (surrender / "renderer.cpp").write_text(
        "__forceinline int provider_only() { return 5; }\n",
        encoding="utf-8",
    )

    rows = _source_shaping_directives(tmp_path, "WIZ8")

    assert [row["kind"] for row in rows] == [
        "forceinline",
        "noinline",
        "optimizer_pragma",
    ]
    assert {row["source_file"] for row in rows} == {"src/wiz8/model.cpp"}
    assert all(row["status"] == "investigation_candidate" for row in rows)


def test_storage_debt_ranks_address_named_storage_and_accessed_padding(tmp_path: Path) -> None:
    from wiz8decomp.reports.semantic_debt import _storage_debt, _Usage, _void_storage

    sources = {
        "src/wiz8/a.cpp": (
            "int g_dword_69ca28;\n"
            "/* g_dword_69ca28 is mentioned only in prose */\n"
            "void Make() { ++g_dword_69ca28; }\n"
            "void Kill() { --g_dword_69ca28; }\n"
            "int Count() { return g_dword_69ca28; }\n"
            "Light::Light() : m_padding_238(0) {}\n"
            "void Copy(Light* l) { l->m_padding_238 = 1; }\n"
        ),
        "include/wiz8/a.h": "struct Light { int m_padding_238; int m_padding_23c; void* list; };\n",
    }
    index = {
        "variables": [
            {
                "qualified_name": "g_dword_69ca28",
                "target": "WIZ8",
                "source_file": "src/wiz8/a.cpp",
                "line": 1,
                "type": "int",
            },
            {
                "qualified_name": "g_live_count",
                "target": "WIZ8",
                "source_file": "src/wiz8/a.cpp",
                "line": 1,
                "type": "int",
            },
        ],
        "classes": [
            {
                "qualified_name": "Light",
                "fields": [
                    {
                        "name": "m_padding_238",
                        "source_file": "include/wiz8/a.h",
                        "line": 3,
                        "offset": 0x238,
                        "type": "int",
                    },
                    {
                        "name": "m_padding_23c",
                        "source_file": "include/wiz8/a.h",
                        "line": 4,
                        "offset": 0x23C,
                        "type": "int",
                    },
                    {
                        "name": "list",
                        "source_file": "include/wiz8/a.h",
                        "line": 5,
                        "offset": 0x240,
                        "type": "void *",
                    },
                ],
            }
        ],
    }

    storage = _storage_debt(index, _Usage(sources))

    (counter,) = storage["address_named_globals"]
    assert counter["name"] == "g_dword_69ca28"
    assert (counter["references"], counter["writes"], counter["reads"]) == (3, 2, 1)
    (padding,) = storage["accessed_padding_members"]
    assert padding["name"] == "m_padding_238"
    assert padding["writes"] == 2
    assert padding["usage_scope"] == "identifier_spelling"
    assert padding["receiver_verified"] is False
    assert padding["possible_nonmember_matches"] is True
    assert [row["field"] for row in _void_storage(index)] == ["list"]


def test_storage_counts_do_not_certify_receivers_for_shared_or_unique_names() -> None:
    index = {
        "classes": [
            {
                "qualified_name": owner,
                "fields": [{"name": name, "source_file": "include/wiz8/a.h", "line": 1}],
            }
            for owner, name in [("A", "unknown_04"), ("B", "unknown_04"), ("C", "unknown_08")]
        ]
    }
    sources = {
        "include/wiz8/a.h": "struct A { int unknown_04; }; struct B { int unknown_04; };"
        "struct C { int unknown_08; };",
        "src/wiz8/a.cpp": "void f(Vendor* v) { int unknown_04 = 0; v->unknown_08 = 1; }",
    }
    rows = _storage_debt(index, _Usage(sources))["address_named_members"]
    assert {row["name"] for row in rows} == {"unknown_04", "unknown_08"}
    for row in rows:
        assert row["receiver_verified"] is False
        assert row["usage_scope"] == "identifier_spelling"
        assert row["possible_nonmember_matches"] is True
    shared = next(row for row in rows if row["name"] == "unknown_04")
    assert {field["record"] for field in shared["fields"]} == {"A", "B"}


def test_byte_strides_report_literals_equal_to_asserted_record_sizes() -> None:
    sources = {
        "include/wiz8/record.h": 'static_assert(sizeof(W8Record) == 0x1c, "size");\n',
        "src/wiz8/record.cpp": (
            "void f(int count) {\n"
            "    p = malloc(0x1c);\n"
            "    q = malloc(count * 0x1c);\n"
            "    // r = malloc(0x1c);\n"
            "    memset(p, 0, 0x1c);\n"
            "    s = malloc(count * 4);\n"
            "    left = (index % 4) * 0x1c + 3;\n"
            "}\n"
        ),
    }

    rows = _byte_strides(sources)

    assert [row["location"] for row in rows] == [
        "src/wiz8/record.cpp:2",
        "src/wiz8/record.cpp:3",
        "src/wiz8/record.cpp:5",
    ]
    assert all(row["candidate_types"] == ["W8Record"] for row in rows)


def test_enum_literal_arguments_report_literals_at_enum_parameters() -> None:
    index = {
        "declarations": [
            {"qualified_name": "W8Thing::SetMode", "parameter_types": ["int", "enum W8Mode"]},
            {"qualified_name": "W8Other::SetKind", "parameter_types": ["enum W8Kind"]},
            {"qualified_name": "W8Model::SetKind", "parameter_types": ["int"]},
        ]
    }
    sources = {
        "src/wiz8/thing.cpp": (
            "void f(W8Thing* t) {\n"
            "    t->SetMode(3, 2);\n"
            "    t->SetMode(3, W8_MODE_IDLE);\n"
            "    t->SetKind(1);\n"
            "    // t->SetMode(1, 1);\n"
            "}\n"
        )
    }

    rows = _enum_literal_arguments(index, sources)

    assert rows == [
        {"location": "src/wiz8/thing.cpp:2", "callee": "SetMode", "argument": 1, "literal": "2"}
    ]


def test_duplicate_layouts_group_identical_unrelated_records() -> None:
    def record(name: str, path: str) -> dict[str, object]:
        fields = [
            {"type": "int", "offset": offset, "size": 4, "source_file": path, "line": 1}
            for offset in (0, 4, 8, 12)
        ]
        return {"qualified_name": name, "bases": [], "fields": fields}

    index = {
        "classes": [
            record("W8RectA", "include/wiz8/a.h"),
            record("W8RectB", "include/wiz8/b.h"),
            record("W8Derived", "include/wiz8/c.h") | {"bases": ["W8RectA"]},
        ]
    }

    rows = _duplicate_layouts(index)

    assert rows == [
        {
            "field_count": 4,
            "records": {"W8RectA": "include/wiz8/a.h:1", "W8RectB": "include/wiz8/b.h:1"},
        }
    ]


def test_adjacent_comment_ignores_previous_declaration_trailing_comment() -> None:
    lines = [
        "    unsigned char auto_release; /* release node when playback ends */",
        "    unsigned char unknown_14d[3];",
        "    /* Multi-line note",
        "       about the next field. */",
        "    int unknown_150;",
    ]
    assert _adjacent_comment(lines, 2) == ""
    assert _adjacent_comment(lines, 5) == "about next field"


def test_comment_disclaiming_semantics_is_not_known_semantics() -> None:
    lines = [
        "    int value_15; /* serialized; no reader consumer */",
        "    unsigned int unknown_08; /* 0x08: serialized; no recovered consumer */",
        "    unsigned char value_48; /* 0x48: max-combined, effect id 0x21 */",
    ]
    assert _DISCLAIMED_SEMANTICS.search(_adjacent_comment_text(lines, 2))
    assert not _DISCLAIMED_SEMANTICS.search(_adjacent_comment_text(lines, 3))
