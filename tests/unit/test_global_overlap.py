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
        "// GLOBAL: WIZ8 0x00685170\nW8GlobalStatus g_status_685170;\n",
    )
    _write(
        tmp_path,
        "src/wiz8/items.cpp",
        "// GLOBAL: WIZ8 0x00686901\nunsigned int g_shared_item_pool_count;\n",
    )

    violations = overlapping_globals(parse_global_definitions(tmp_path))
    assert any(
        item["detail"] == "g_shared_item_pool_count @ 0x686901 overlaps g_status_685170 + 0x1791."
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
        "extern W8GlobalStatus g_status_685170;\n",
    )
    _write(
        tmp_path,
        "src/wiz8/status.cpp",
        "// GLOBAL: WIZ8 0x00685170\nW8GlobalStatus g_status_685170;\n",
    )
    _write(
        tmp_path,
        "include/wiz8/items.h",
        "// GLOBAL: WIZ8 0x00685170\nextern int g_status_685170;\n",
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


def test_overlap_gate_raises(tmp_path: Path) -> None:
    _write(tmp_path, "src/wiz8/a.cpp", "// GLOBAL: WIZ8 0x100\nint g_a;\n")
    _write(tmp_path, "src/wiz8/b.cpp", "// GLOBAL: WIZ8 0x100\nint g_b;\n")
    try:
        validate_global_ownership(tmp_path)
    except GlobalOverlapError as error:
        assert "g_b" in str(error) or "g_a" in str(error)
    else:
        raise AssertionError("expected GlobalOverlapError")
