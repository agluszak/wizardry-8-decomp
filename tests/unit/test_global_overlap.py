from __future__ import annotations

from pathlib import Path

from wiz8decomp.global_model import (
    GlobalOverlapError,
    overlapping_globals,
    parse_global_definitions,
    type_consistency_violations,
    validate_global_ownership,
)


def _write(repo: Path, relative: str, text: str) -> None:
    path = repo / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def test_qualified_static_members_keep_owner_out_of_storage_type(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/surrender/globals.cpp",
        "// GLOBAL: SURRENDER 0x100a4778\nint srCore::initialized;\n"
        "// GLOBAL: SURRENDER 0x1009c714\n"
        'const char* srTimer::default_storage = "hkcu:SOFTWARE/Hybrid";\n'
        "// GLOBAL: SURRENDER 0x100a8a30\nchar srTimer::RegKeyName[0x400];\n"
        "// GLOBAL: SURRENDER 0x100a45b0\n"
        "srClass::Update* srClass::_firstUpdate;\n"
        "// GLOBAL: SURRENDER 0x100a1aa8\nint srPalette::Quantizer::initialized = 0;\n"
        "// GLOBAL: SURRENDER 0x100a8a31\nunsigned char interior;\n",
    )
    definitions = parse_global_definitions(tmp_path)
    by_address = {row["address"]: row for row in definitions}
    for address, name, storage_type, size in (
        (0x100A4778, "initialized", "int", 4),
        (0x1009C714, "default_storage", "const char*", 4),
        (0x100A8A30, "RegKeyName", "char[0x400]", 0x400),
        (0x100A45B0, "_firstUpdate", "srClass::Update*", 4),
        (0x100A1AA8, "initialized", "int", 4),
    ):
        row = by_address[address]
        assert (row["name"], row["type"], row["size"]) == (name, storage_type, size)
    assert any(row["container"] == "RegKeyName" for row in overlapping_globals(definitions))


def test_exact_start_collision(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/wiz8/a.cpp",
        "// GLOBAL: WIZ8 0x00685170\nint g_status;\n",
    )
    _write(
        tmp_path,
        "src/wiz8/b.cpp",
        "// GLOBAL: WIZ8 0x00685170\nint g_alias;\n",
    )

    violations = overlapping_globals(parse_global_definitions(tmp_path))
    details = [item["detail"] for item in violations]
    assert any("g_alias" in item and "g_status" in item for item in details)


def test_constructed_and_callback_globals_keep_type_and_extent(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/wiz8/globals.cpp",
        "// GLOBAL: WIZ8 0x00650000\n"
        "W8Vector<stLight*> g_lights(5);\n"
        "// GLOBAL: WIZ8 0x00650010\n"
        "void (*g_callback)(void);\n"
        "// GLOBAL: WIZ8 0x00650014\n"
        "W8Vector<stLight*>* g_lights_pointer;\n"
        "// GLOBAL: WIZ8 0x00650020\n"
        "void (*g_callbacks[16])(void) = {};\n"
        "// GLOBAL: WIZ8 0x00650060\n"
        "srHeapBuffer<unsigned char> g_bytes;\n",
    )

    definitions = {row["name"]: row for row in parse_global_definitions(tmp_path)}
    assert definitions["g_lights"]["type"] == "W8Vector<stLight*>"
    assert definitions["g_lights"]["size"] == 16
    assert definitions["g_callback"]["type"] == "void (*)(void)"
    assert definitions["g_callback"]["size"] == 4
    assert definitions["g_lights_pointer"]["size"] == 4
    assert definitions["g_callbacks"]["type"] == "void (*)(void)[16]"
    assert definitions["g_callbacks"]["size"] == 64
    assert definitions["g_bytes"]["size"] == 8


def test_sgp_pointer_typedef_array_has_pointer_extent(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/sgp/globals.c",
        "// GLOBAL: WIZ8 0x00650000\nHVOBJECT g_pictures[40];\n",
    )
    definitions = parse_global_definitions(tmp_path)
    assert definitions[0]["size"] == 160


def test_sgp_boolean_array_covers_interior_global_marker(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/sgp/input.c",
        "// GLOBAL: WIZ8 0x006f0520\nBOOLEAN gfKeyState[256];\n",
    )
    _write(
        tmp_path,
        "src/wiz8/keys.cpp",
        "// GLOBAL: WIZ8 0x006f0531\nbool g_control_alias;\n",
    )
    definitions = parse_global_definitions(tmp_path)
    assert next(row for row in definitions if row["name"] == "gfKeyState")["size"] == 256
    assert any(row["container"] == "gfKeyState" for row in overlapping_globals(definitions))


def test_same_file_integer_define_sizes_global_without_changing_source_type(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/sgp/globals.c",
        "#define COUNT 7\n"
        "// GLOBAL: WIZ8 0x00650000\nunsigned char g_bytes[COUNT];\n"
        "#if 0\n#define UNCERTAIN 3\n#endif\n"
        "// GLOBAL: WIZ8 0x00650020\nint g_unknown[UNCERTAIN];\n",
    )
    definitions = {row["name"]: row for row in parse_global_definitions(tmp_path)}
    assert definitions["g_bytes"]["type"] == "unsigned char[COUNT]"
    assert definitions["g_bytes"]["projected_type"] == "unsigned char[7]"
    assert definitions["g_bytes"]["size"] == 7
    assert definitions["g_unknown"]["size"] is None


def test_inferred_array_extent_counts_outer_initializer_elements(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/wiz8/globals.cpp",
        '// GLOBAL: WIZ8 0x00650000\nconst char* g_names[] = {"one,two", "three",};\n'
        "// GLOBAL: WIZ8 0x00650010\n"
        "unsigned short g_rows[][3] = {{1, 2, 3}, /* row */ {4, 5, 6}};\n"
        "// GLOBAL: WIZ8 0x0065001c\n"
        "int g_sparse[] = {[4] = 1};\n"
        "// GLOBAL: WIZ8 0x00650030\n"
        "int g_conditional[] = {\n#if CHOICE\n1,\n#else\n1, 2,\n#endif\n};\n",
    )
    definitions = {row["name"]: row for row in parse_global_definitions(tmp_path)}
    assert definitions["g_names"]["projected_type"] == "const char*[2]"
    assert definitions["g_names"]["size"] == 8
    assert definitions["g_rows"]["projected_type"] == "unsigned short[2][3]"
    assert definitions["g_rows"]["size"] == 12
    assert definitions["g_sparse"]["size"] is None
    assert definitions["g_conditional"]["size"] is None


def test_direct_header_integer_bounds_reject_overrides_and_conditionals(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "include/wiz8/counts.h",
        "#ifndef WIZ8_COUNTS_H\n#define WIZ8_COUNTS_H\n"
        "enum { W8_ITEM_COUNT = 7, W8_ROW_COUNT = 3 };\n"
        "#define W8_EXTRA_COUNT 4\n"
        "/* enum { W8_COMMENT_COUNT = 99 }; */\n"
        "#if CHOICE\n#define W8_UNCERTAIN_COUNT 5\n#endif\n"
        "#endif\n",
    )
    _write(
        tmp_path,
        "src/wiz8/globals.cpp",
        '#include "wiz8/counts.h"\n'
        "// GLOBAL: WIZ8 0x00650000\nint g_items[W8_ITEM_COUNT];\n"
        "// GLOBAL: WIZ8 0x00650020\nchar g_rows[W8_ROW_COUNT+W8_EXTRA_COUNT];\n"
        "// GLOBAL: WIZ8 0x00650030\nint g_uncertain[W8_UNCERTAIN_COUNT];\n"
        "// GLOBAL: WIZ8 0x00650040\nint g_commented[W8_COMMENT_COUNT];\n"
        "#undef W8_ITEM_COUNT\n#define W8_ITEM_COUNT 9\n"
        "// GLOBAL: WIZ8 0x00650050\nint g_override[W8_ITEM_COUNT];\n",
    )
    definitions = {row["name"]: row for row in parse_global_definitions(tmp_path)}
    assert definitions["g_items"]["projected_type"] == "int[7]"
    assert definitions["g_items"]["size"] == 28
    assert definitions["g_rows"]["projected_type"] == "char[3+4]"
    assert definitions["g_rows"]["size"] == 7
    assert definitions["g_uncertain"]["size"] is None
    assert definitions["g_commented"]["size"] is None
    assert definitions["g_override"]["projected_type"] == "int[9]"


def test_inner_member_collision(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "include/wiz8/status.h",
        "struct W8GlobalStatus { unsigned char bytes[0x49c2]; };\n"
        'static_assert(sizeof(W8GlobalStatus) == 0x49c2, "size");\n',
    )
    _write(
        tmp_path,
        "src/wiz8/status.cpp",
        "// GLOBAL: WIZ8 0x00685170\nW8GlobalStatus g_status;\n",
    )
    _write(
        tmp_path,
        "src/wiz8/items.cpp",
        "// GLOBAL: WIZ8 0x00686901\nunsigned int g_shared_item_pool_count;\n",
    )

    violations = overlapping_globals(parse_global_definitions(tmp_path))
    assert any(
        item["detail"] == "g_shared_item_pool_count @ 0x686901 overlaps g_status + 0x1791."
        for item in violations
    )


def test_partial_overlap(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/wiz8/a.cpp",
        "// GLOBAL: WIZ8 0x00685000\nunsigned char g_left[8];\n",
    )
    _write(
        tmp_path,
        "src/wiz8/b.cpp",
        "// GLOBAL: WIZ8 0x00685004\nunsigned char g_right[8];\n",
    )

    violations = overlapping_globals(parse_global_definitions(tmp_path))
    assert any("g_right" in item["detail"] and "g_left" in item["detail"] for item in violations)


def test_legitimate_extern_is_not_an_overlap(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "include/wiz8/status.h",
        "struct W8GlobalStatus { unsigned char bytes[4]; };\n"
        'static_assert(sizeof(W8GlobalStatus) == 4, "size");\n'
        "extern W8GlobalStatus g_status;\n",
    )
    _write(
        tmp_path,
        "src/wiz8/status.cpp",
        "// GLOBAL: WIZ8 0x00685170\nW8GlobalStatus g_status;\n",
    )
    _write(
        tmp_path,
        "include/wiz8/items.h",
        "// GLOBAL: WIZ8 0x00685170\nextern int g_status;\n",
    )

    assert overlapping_globals(parse_global_definitions(tmp_path)) == []
    assert validate_global_ownership(tmp_path)["ok"]


def test_unknown_size_global_only_collides_at_its_start(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/wiz8/a.cpp",
        "// GLOBAL: WIZ8 0x00683f78\nW8XStatus gXStatus;\n",
    )
    _write(
        tmp_path,
        "src/wiz8/b.cpp",
        "// GLOBAL: WIZ8 0x00683f94\nunsigned char g_combat_mode;\n",
    )

    assert overlapping_globals(parse_global_definitions(tmp_path)) == []


def test_incompatible_types_at_the_same_address(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/wiz8/a.cpp",
        "// GLOBAL: WIZ8 0x00686901\nunsigned int g_count;\n",
    )
    _write(
        tmp_path,
        "src/wiz8/b.cpp",
        "// GLOBAL: WIZ8 0x00686901\nunsigned char g_flag;\n",
    )

    violations = type_consistency_violations(parse_global_definitions(tmp_path))
    assert violations
    assert violations[0]["kind"] == "type-consistency"


def test_unaddressed_global_is_a_violation(tmp_path: Path) -> None:
    _write(tmp_path, "src/wiz8/a.cpp", "// GLOBAL\nint g_orphan;\n")

    from wiz8decomp.global_model import unaddressed_globals

    assert any(item["kind"] == "unaddressed-global" for item in unaddressed_globals(tmp_path))


def test_explicit_unresolved_global_is_allowed(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/wiz8/a.cpp",
        "// GLOBAL: WIZ8 unresolved\nint g_orphan;\n",
    )

    from wiz8decomp.global_model import unaddressed_globals

    assert unaddressed_globals(tmp_path) == []
    assert overlapping_globals(parse_global_definitions(tmp_path)) == []


def test_extern_with_initializer_is_a_definition(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/wiz8/a.cpp",
        "// GLOBAL: WIZ8 0x0061eefc\nextern const int g_ai_kind_table[32][2] = {};\n",
    )
    _write(
        tmp_path,
        "src/wiz8/b.cpp",
        "// GLOBAL: WIZ8 0x0061eefc\nint g_ai_kind_table_alias;\n",
    )

    definitions = parse_global_definitions(tmp_path)
    assert any(item["name"] == "g_ai_kind_table" for item in definitions)
    violations = overlapping_globals(definitions)
    assert any("g_ai_kind_table_alias" in item["detail"] for item in violations)


def test_extern_definition_without_address_is_a_violation(tmp_path: Path) -> None:
    _write(
        tmp_path,
        "src/wiz8/a.cpp",
        "// GLOBAL\nextern const int g_table[2] = {};\n",
    )

    from wiz8decomp.global_model import unaddressed_globals

    assert any(item["kind"] == "unaddressed-global" for item in unaddressed_globals(tmp_path))


def test_overlap_gate_raises(tmp_path: Path) -> None:
    _write(tmp_path, "src/wiz8/a.cpp", "// GLOBAL: WIZ8 0x100\nint g_a;\n")
    _write(tmp_path, "src/wiz8/b.cpp", "// GLOBAL: WIZ8 0x100\nint g_b;\n")
    try:
        validate_global_ownership(tmp_path)
    except GlobalOverlapError as error:
        assert "g_b" in str(error) or "g_a" in str(error)
    else:
        raise AssertionError("expected GlobalOverlapError")
