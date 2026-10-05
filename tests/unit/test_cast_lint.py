from __future__ import annotations

import subprocess
from pathlib import Path

import pytest
from wiz8decomp.cast_lint import (
    _CAST,
    _MARKER,
    CastGateError,
    _added_c_style_casts,
    _added_format_off,
    _added_uninit_suppressions,
    _raw_offset_violations,
    _sgp_notice_violations,
    added_lines_without_marker,
    validate_cast_markers,
)


def _added_casts(diff: str) -> list[dict[str, object]]:
    return added_lines_without_marker(diff, _CAST, _MARKER)


def _diff(path: str, *body: str) -> str:
    return "\n".join([f"diff --git a/{path} b/{path}", f"--- a/{path}", f"+++ b/{path}", *body])


@pytest.mark.parametrize("root", ["src/wiz8", "include/wiz8", "src/surrender", "include/surrender"])
def test_unmarked_added_cast_is_reported(root: str) -> None:
    diff = _diff(
        f"{root}/example.cpp",
        "@@ -1,1 +1,2 @@",
        " int f() {",
        "+    return reinterpret_cast<int>(value);",
    )

    assert _added_casts(diff) == [
        {"file": f"{root}/example.cpp", "line": 2, "text": "return reinterpret_cast<int>(value);"}
    ]


def test_c_style_cast_gate_ignores_cast_spelling_inside_string_literals() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -0,0 +1,2 @@",
        '+    srAssertFail("(usTemp < (UINT16)ubNumFrames)", source, line, message);',
        "+    return (int)value;",
    )

    assert _added_c_style_casts(diff) == [
        {"file": "src/wiz8/example.cpp", "line": 2, "text": "return (int)value;"}
    ]


def test_marker_with_reason_passes() -> None:
    diff = _diff(
        "include/wiz8/example.h",
        "@@ -1,1 +1,2 @@",
        " int f();",
        "+inline int* g() { return reinterpret_cast<int*>(raw); } // reinterpret-ok: ABI slot",
    )

    assert _added_casts(diff) == []


def test_increment_line_is_not_mistaken_for_a_file_header() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,2 +1,3 @@",
        " int f() {",
        "+    ++counter;",
        "+    return reinterpret_cast<int>(value);",
    )

    assert [item["line"] for item in _added_casts(diff)] == [3]


def test_moved_cast_is_not_a_new_cast() -> None:
    diff = (
        "diff --git a/src/wiz8/new.cpp b/src/wiz8/new.cpp\n"
        "--- a/src/wiz8/new.cpp\n"
        "+++ b/src/wiz8/new.cpp\n"
        "@@ -0,0 +1 @@\n"
        "+    return reinterpret_cast<int>(value);\n"
        "diff --git a/src/wiz8/old.cpp b/src/wiz8/old.cpp\n"
        "--- a/src/wiz8/old.cpp\n"
        "+++ b/src/wiz8/old.cpp\n"
        "@@ -1 +0,0 @@\n"
        "-    return reinterpret_cast<int>(value);\n"
    )

    assert _added_casts(diff) == []


def test_renamed_operand_cast_is_not_a_new_cast() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,1 @@",
        "-    return (float)fog_start_150;",
        "+    return (float)fog_start;",
    )

    assert _added_c_style_casts(diff) == []


def test_renamed_operand_reinterpret_cast_is_not_a_new_cast() -> None:
    diff = _diff(
        "src/surrender/example.cpp",
        "@@ -1,1 +1,1 @@",
        "-    key.texture0 = reinterpret_cast<srTextureIFace* const*>(scratch->dir_90 + pipe.sub_batch_offset_88);",
        "+    key.texture0 = reinterpret_cast<srTextureIFace* const*>(scratch->dir + pipe.sub_batch_offset);",
    )

    assert _added_casts(diff) == []


def test_retyped_cast_on_renamed_line_is_still_reported() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,1 @@",
        "-    return (float)fog_start_150;",
        "+    return (double)fog_start;",
    )

    assert _added_c_style_casts(diff) == [
        {"file": "src/wiz8/example.cpp", "line": 1, "text": "return (double)fog_start;"}
    ]


def test_same_hunk_operand_rename_is_not_a_new_cast() -> None:
    # Same-hunk, same-shape operand changes are treated as renames, not new
    # casts: cast hygiene watches the cast, while operand choice is covered
    # by comparison review. A differently-shaped change is still reported
    # (see test_retyped_cast_on_renamed_line_is_still_reported).
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,1 @@",
        "-    return (float)fog_density;",
        "+    return (float)mist_density;",
    )

    assert _added_c_style_casts(diff) == []


def test_descriptive_rename_on_cast_line_is_not_a_new_cast() -> None:
    diff = _diff(
        "src/surrender/example.cpp",
        "@@ -4,1 +4,1 @@",
        "-                            reinterpret_cast<const unsigned long*>(pass->texture_array_0c), 1,",
        "+                            reinterpret_cast<const unsigned long*>(pass->tex_table_0), 1,",
    )

    assert _added_casts(diff) == []


def test_retyped_cast_with_new_operand_is_still_reported() -> None:
    diff = _diff(
        "src/surrender/example.cpp",
        "@@ -4,1 +4,1 @@",
        "-                            reinterpret_cast<const unsigned long*>(pass->texture_array_0c), 1,",
        "+                            reinterpret_cast<const double*>(pass->tex_table_0), 1,",
    )

    assert _added_casts(diff) == [
        {
            "file": "src/surrender/example.cpp",
            "line": 4,
            "text": "reinterpret_cast<const double*>(pass->tex_table_0), 1,",
        }
    ]


def test_new_cast_shaped_like_a_removed_one_is_still_reported() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,2 +1,2 @@",
        "-    old = (float)previous;",
        "+    old = (float)previous;",
        "-    return 0;",
        "+    return (float)unrelated;",
    )

    assert _added_c_style_casts(diff) == [
        {"file": "src/wiz8/example.cpp", "line": 2, "text": "return (float)unrelated;"}
    ]


def test_renamed_file_reports_the_new_path() -> None:
    diff = (
        "diff --git a/src/wiz8/old.cpp b/src/wiz8/new.cpp\n"
        "similarity index 90%\n"
        "rename from src/wiz8/old.cpp\n"
        "rename to src/wiz8/new.cpp\n"
        "--- a/src/wiz8/old.cpp\n"
        "+++ b/src/wiz8/new.cpp\n"
        "@@ -1,1 +1,2 @@\n"
        " int f();\n"
        "+int g() { return reinterpret_cast<int>(x); }\n"
    )

    assert _added_casts(diff) == [
        {
            "file": "src/wiz8/new.cpp",
            "line": 2,
            "text": "int g() { return reinterpret_cast<int>(x); }",
        }
    ]


def test_deleted_file_contributes_nothing() -> None:
    diff = (
        "diff --git a/src/wiz8/gone.cpp b/src/wiz8/gone.cpp\n"
        "deleted file mode 100644\n"
        "--- a/src/wiz8/gone.cpp\n"
        "+++ /dev/null\n"
        "@@ -1 +0,0 @@\n"
        "-int f() { return reinterpret_cast<int>(x); }\n"
    )

    assert _added_casts(diff) == []


def test_new_builtin_c_style_cast_is_reported() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,2 @@",
        " int f() {",
        "+    return (unsigned int)value;",
    )

    assert _added_c_style_casts(diff) == [
        {"file": "src/wiz8/example.cpp", "line": 2, "text": "return (unsigned int)value;"}
    ]


def test_new_project_pointer_c_style_cast_is_reported() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,2 @@",
        " int f() {",
        "+    W8Monster* monster = (W8Monster*)raw;",
    )

    assert _added_c_style_casts(diff) == [
        {
            "file": "src/wiz8/example.cpp",
            "line": 2,
            "text": "W8Monster* monster = (W8Monster*)raw;",
        }
    ]


def test_c_style_cast_marker_with_reason_passes() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,2 @@",
        " int f() {",
        "+    return (HWFILE)raw; // c-style-cast-ok: historical C callback ABI",
    )

    assert _added_c_style_casts(diff) == []


def test_cpp_cast_parentheses_sizeof_and_constants_are_not_c_style_casts() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,6 @@",
        " int f() {",
        "+    int value = static_cast<int>(raw);",
        "+    int bytes = sizeof(int) * count;",
        "+    if ((enabled) && value) return value;",
        "+    if ((W8_MAX_MONSTERS) && value) return value;",
        "+    return value;",
    )

    assert _added_c_style_casts(diff) == []


def test_c_source_is_outside_c_style_cast_gate() -> None:
    diff = _diff(
        "src/wiz8/example.c",
        "@@ -1,1 +1,2 @@",
        " int f() {",
        "+    return (int)value;",
    )

    assert _added_c_style_casts(diff) == []


def test_new_format_off_requires_reason() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,2 @@",
        " int f() {",
        "+    // clang-format off",
    )

    assert _added_format_off(diff) == [
        {"file": "src/wiz8/example.cpp", "line": 2, "text": "// clang-format off"}
    ]


def test_moved_format_off_is_reaudited() -> None:
    diff = (
        "diff --git a/src/wiz8/new.cpp b/src/wiz8/new.cpp\n"
        "--- a/src/wiz8/new.cpp\n"
        "+++ b/src/wiz8/new.cpp\n"
        "@@ -0,0 +1 @@\n"
        "+// clang-format off\n"
        "diff --git a/src/wiz8/old.cpp b/src/wiz8/old.cpp\n"
        "--- a/src/wiz8/old.cpp\n"
        "+++ b/src/wiz8/old.cpp\n"
        "@@ -1 +0,0 @@\n"
        "-// clang-format off\n"
    )

    assert _added_format_off(diff) == [
        {"file": "src/wiz8/new.cpp", "line": 1, "text": "// clang-format off"}
    ]


def test_format_off_marker_with_reason_passes() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,2 @@",
        " int f() {",
        "+    // clang-format off // format-off-ok: preserve compact evidence table",
    )

    assert _added_format_off(diff) == []


@pytest.mark.parametrize(
    "path",
    [
        "src/wiz8/example.cpp",
        "src/wiz8/CMakeLists.txt",
        "src/surrender/CMakeLists.txt",
        "cmake/Warnings.cmake",
    ],
)
@pytest.mark.parametrize("marked", [False, True])
def test_uninitialized_suppression_requires_retail_evidence(path: str, marked: bool) -> None:
    line = '"-Wno-error=sometimes-uninitialized"'
    if marked:
        line += " # uninit-ok: retail named-entity path lacks a destination assignment"
    diff = _diff(path, "@@ -0,0 +1 @@", "+" + line)
    assert bool(_added_uninit_suppressions(diff)) is not marked


def test_moved_uninitialized_suppression_is_reaudited() -> None:
    line = '"-Wno-error=sometimes-uninitialized"'
    diff = _diff("src/wiz8/CMakeLists.txt", "@@ -0,0 +1 @@", "+" + line)
    diff += "\n" + _diff("cmake/Warnings.cmake", "@@ -1 +0,0 @@", "-" + line)
    assert _added_uninit_suppressions(diff) == [
        {"file": "src/wiz8/CMakeLists.txt", "line": 1, "text": line}
    ]


def test_literal_byte_offset_into_typed_object_is_reported(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "int f(W8Record* record) {\n"
        "    return *reinterpret_cast<int*>(\n"
        "        reinterpret_cast<char*>(record) + 0x24);\n"
        "}\n"
    )
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -0,0 +1,4 @@",
        "+int f(W8Record* record) {",
        "+    return *reinterpret_cast<int*>(",
        "+        reinterpret_cast<char*>(record) + 0x24);",
        "+}",
    )

    assert _raw_offset_violations(tmp_path, diff) == [
        {
            "file": "src/wiz8/example.cpp",
            "line": 3,
            "text": "reinterpret_cast<char*>(record) + 0x24",
        }
    ]


def test_raw_offset_marker_allows_unresolved_layout(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "int f(W8Record* record) {\n"
        "    return *reinterpret_cast<int*>(\n"
        "        reinterpret_cast<char*>(record) + 0x24); "
        "// raw-offset-ok: unresolved vendor tail\n"
        "}\n"
    )
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -0,0 +1,4 @@",
        "+int f(W8Record* record) {",
        "+    return *reinterpret_cast<int*>(",
        "+        reinterpret_cast<char*>(record) + 0x24); // raw-offset-ok: unresolved vendor tail",
        "+}",
    )

    assert _raw_offset_violations(tmp_path, diff) == []


def test_variable_byte_offset_is_not_a_layout_escape(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "char* advance(W8Record* record, int offset) {\n"
        "    return reinterpret_cast<char*>(record) + offset;\n"
        "}\n"
    )
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -0,0 +1,3 @@",
        "+char* advance(W8Record* record, int offset) {",
        "+    return reinterpret_cast<char*>(record) + offset;",
        "+}",
    )

    assert _raw_offset_violations(tmp_path, diff) == []


def test_changed_pristine_sgp_source_is_reported(tmp_path: Path) -> None:
    source = tmp_path / "src/sgp/LibraryDataBase.c"
    source.parent.mkdir(parents=True)
    source.write_text("int changed;\n")
    diff = _diff(
        "src/sgp/LibraryDataBase.c",
        "@@ -1,1 +1,1 @@",
        "-int original;",
        "+int changed;",
    )

    assert _sgp_notice_violations(tmp_path, diff) == [
        {
            "file": "src/sgp/LibraryDataBase.c",
            "line": 1,
            "text": "changed pristine SGP source lacks a dated Wizardry reconstruction notice",
        }
    ]


def test_sgp_derivative_notice_allows_change(tmp_path: Path) -> None:
    source = tmp_path / "src/sgp/input.c"
    source.parent.mkdir(parents=True)
    source.write_text(
        "/* Modified for the Wizardry 8 reconstruction, 2026-09-14. */\nint changed;\n"
    )
    diff = _diff(
        "src/sgp/input.c",
        "@@ -1,1 +1,2 @@",
        "+/* Modified for the Wizardry 8 reconstruction, 2026-09-14. */",
        "+int changed;",
    )

    assert _sgp_notice_violations(tmp_path, diff) == []


def test_git_checkout_enforces_the_gate(tmp_path: Path) -> None:
    subprocess.run(["git", "init", "-q", "-b", "main", str(tmp_path)], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "config", "user.email", "test@invalid"], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "config", "user.name", "test"], check=True)
    (tmp_path / "src/wiz8").mkdir(parents=True)
    source = tmp_path / "src/wiz8/example.cpp"
    source.write_text("int f() { return 0; }\n")
    subprocess.run(["git", "-C", str(tmp_path), "add", "."], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "commit", "-qm", "base"], check=True)

    source.write_text("int f() { return reinterpret_cast<int>(g); }\n")
    with pytest.raises(CastGateError, match="reinterpret-ok"):
        validate_cast_markers(tmp_path)

    source.write_text(
        "int f() { return reinterpret_cast<int>(g); } // reinterpret-ok: test boundary\n"
    )
    assert validate_cast_markers(tmp_path)["ok"] is True


def test_new_layout_union_needs_positive_evidence(tmp_path: Path) -> None:
    subprocess.run(["git", "init", "-q", "-b", "main", str(tmp_path)], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "config", "user.email", "test@invalid"], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "config", "user.name", "test"], check=True)
    header = tmp_path / "include/surrender/example.h"
    header.parent.mkdir(parents=True)
    header.write_text("struct Example { int value; };\n")
    subprocess.run(["git", "-C", str(tmp_path), "add", "."], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "commit", "-qm", "base"], check=True)

    header.write_text("union Example { int value; float other; };\n")
    with pytest.raises(CastGateError, match="union-ok"):
        validate_cast_markers(tmp_path)

    header.write_text(
        "// union-ok: retail writes both alternatives under distinct tags\n"
        "union Example { int value; float other; };\n"
    )
    assert validate_cast_markers(tmp_path)["ok"] is True


def test_git_checkout_enforces_raw_offset_gate(tmp_path: Path) -> None:
    subprocess.run(["git", "init", "-q", "-b", "main", str(tmp_path)], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "config", "user.email", "test@invalid"], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "config", "user.name", "test"], check=True)
    (tmp_path / "src/wiz8").mkdir(parents=True)
    source = tmp_path / "src/wiz8/example.cpp"
    source.write_text("int f() { return 0; }\n")
    subprocess.run(["git", "-C", str(tmp_path), "add", "."], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "commit", "-qm", "base"], check=True)

    source.write_text(
        "int f(W8Record* record) {\n"
        "    return *reinterpret_cast<int*>(\n"
        "        reinterpret_cast<char*>(record) + 0x24); "
        "// reinterpret-ok: unresolved object storage\n"
        "}\n"
    )
    with pytest.raises(CastGateError, match="raw-offset-ok"):
        validate_cast_markers(tmp_path)

    source.write_text(
        "int f(W8Record* record) {\n"
        "    return *reinterpret_cast<int*>(\n"
        "        reinterpret_cast<char*>(record) + 0x24); "
        "// reinterpret-ok: unresolved object storage; raw-offset-ok: unresolved vendor tail\n"
        "}\n"
    )
    assert validate_cast_markers(tmp_path)["ok"] is True


def test_git_checkout_enforces_c_style_and_format_gates(tmp_path: Path) -> None:
    subprocess.run(["git", "init", "-q", "-b", "main", str(tmp_path)], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "config", "user.email", "test@invalid"], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "config", "user.name", "test"], check=True)
    (tmp_path / "src/wiz8").mkdir(parents=True)
    source = tmp_path / "src/wiz8/example.cpp"
    source.write_text("int f() { return 0; }\n")
    subprocess.run(["git", "-C", str(tmp_path), "add", "."], check=True)
    subprocess.run(["git", "-C", str(tmp_path), "commit", "-qm", "base"], check=True)

    source.write_text("int f() { return (int)g; }\n// clang-format off\n")
    with pytest.raises(CastGateError, match="c-style-cast-ok"):
        validate_cast_markers(tmp_path)

    source.write_text(
        "int f() { return (int)g; } // c-style-cast-ok: test C ABI\n"
        "// clang-format off // format-off-ok: test table\n"
    )
    assert validate_cast_markers(tmp_path)["ok"] is True


@pytest.mark.parametrize(
    ("statement", "accepted"),
    [
        (
            "auto p = reinterpret_cast<char*>(\n    address); // reinterpret-ok: external buffer\n",
            True,
        ),
        ("// reinterpret-ok: external buffer\nauto p = reinterpret_cast<char*>(address);\n", True),
        ("auto p = (char*)\n    address; // c-style-cast-ok: external ABI\n", True),
        (
            "auto p = reinterpret_cast<char*>(\n    address);\nauto q = other; // reinterpret-ok: other buffer\n",
            False,
        ),
        ('auto p = reinterpret_cast<char*>(\n    "reinterpret-ok: not a comment");\n', False),
        (
            'auto p = reinterpret_cast<char*>(\n    find(";")); // reinterpret-ok: external buffer\n',
            True,
        ),
    ],
)
def test_formatter_wrapped_cast_markers(
    tmp_path: Path, monkeypatch, statement: str, accepted: bool
) -> None:
    source = tmp_path / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(statement)
    lines = statement.splitlines()
    diff = _diff(
        "src/wiz8/example.cpp", f"@@ -0,0 +1,{len(lines)} @@", *("+" + line for line in lines)
    )
    monkeypatch.setattr("wiz8decomp.cast_lint.baseline_diff", lambda repository: ("base", diff))
    if accepted:
        assert validate_cast_markers(tmp_path)["ok"] is True
    else:
        with pytest.raises(CastGateError):
            validate_cast_markers(tmp_path)


@pytest.mark.parametrize(
    ("old_target", "new_target"),
    [
        ("CustomType", "OtherType"),
        ("W8Type::First", "W8Type::Second"),
        ("srPtr<CustomType>", "srPtr<OtherType>"),
    ],
)
def test_descriptive_rename_preserves_entire_cast_target(old_target: str, new_target: str) -> None:
    diff = _diff(
        "src/surrender/example.cpp",
        "@@ -1,1 +1,1 @@",
        f"-    return reinterpret_cast<{old_target}*>(old_name);",
        f"+    return reinterpret_cast<{new_target}*>(new_name);",
    )
    assert len(_added_casts(diff)) == 1


@pytest.mark.parametrize("removed_operand, added_operand", [("old", "old"), ("fog_150", "fog")])
def test_removed_cast_can_only_be_consumed_once(removed_operand: str, added_operand: str) -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,2 @@",
        f"-    return (float){removed_operand};",
        f"+    return (float){added_operand};",
        "+    return (float)new_operand;",
    )
    assert len(_added_c_style_casts(diff)) == 1


def test_cast_matching_uses_full_line_before_truncating_diagnostic() -> None:
    prefix = "    result = " + "padding + " * 25
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,1 @@",
        f"-{prefix}reinterpret_cast<First*>(old);",
        f"+{prefix}reinterpret_cast<Second*>(new);",
    )
    assert len(_added_casts(diff)) == 1


def test_descriptive_rename_does_not_erase_string_literal_changes() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,1 @@",
        '-    call("old", reinterpret_cast<int*>(old));',
        '+    call("new", reinterpret_cast<int*>(new));',
    )
    assert len(_added_casts(diff)) == 1


@pytest.mark.parametrize("target", ["(float)", "reinterpret_cast<int*>"])
def test_member_qualification_preserves_existing_cast(target: str) -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,1 @@",
        f"-    return {target}(old_20);",
        f"+    return {target}(this->old);",
    )
    assert not _added_casts(diff)
    assert not _added_c_style_casts(diff)


def test_member_qualification_keeps_literal_changes_visible() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,1 @@",
        '-    call("this->old", reinterpret_cast<int*>(old_20));',
        '+    call("old", reinterpret_cast<int*>(this->old));',
    )
    assert len(_added_casts(diff)) == 1


def test_raw_offset_rename_consumes_existing_occurrence_once(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True)
    line = "    block->name = reinterpret_cast<char*>(block) + 0x20;"
    source.write_text(line + "\n" + line + "\n")
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,2 @@",
        "-    block->name_0c = reinterpret_cast<char*>(block) + 0x20;",
        "+" + line,
        "+" + line,
    )
    assert len(_raw_offset_violations(tmp_path, diff)) == 1


def test_raw_offset_rename_preserves_offset_changes(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/example.cpp"
    source.parent.mkdir(parents=True)
    line = "    block->name = reinterpret_cast<char*>(block) + 0x24;"
    source.write_text(line + "\n")
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,1 @@",
        "-    block->name_0c = reinterpret_cast<char*>(block) + 0x20;",
        "+" + line,
    )
    assert len(_raw_offset_violations(tmp_path, diff)) == 1


@pytest.mark.parametrize(
    ("before", "after"),
    [
        (
            "return reinterpret_cast<const unsigned long *>(raw + offset);",
            "return reinterpret_cast<const unsigned long*>(\n    raw + offset);",
        ),
        (
            "return (float)(left +\n    right);",
            "return (float)(left + right);",
        ),
    ],
)
def test_formatter_wrapped_cast_is_not_new(before: str, after: str) -> None:
    old = before.splitlines()
    new = after.splitlines()
    diff = _diff(
        "src/wiz8/example.cpp",
        f"@@ -1,{len(old)} +1,{len(new)} @@",
        *(f"-{line}" for line in old),
        *(f"+{line}" for line in new),
    )
    assert _added_casts(diff) == []
    assert _added_c_style_casts(diff) == []


@pytest.mark.parametrize(("replacement", "offset"), [("double", 4), ("float", 8)])
def test_wrapped_cast_preserves_types_and_literals(replacement: str, offset: int) -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,2 @@",
        "-return reinterpret_cast<float*>(raw + 4);",
        f"+return reinterpret_cast<{replacement}*>(\n+    raw + {offset});",
    )
    assert len(_added_casts(diff)) == 1


def test_wrapped_cast_does_not_reuse_removed_occurrence() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,4 @@",
        "-consume(reinterpret_cast<int*>(raw));",
        "+consume(reinterpret_cast<int*>(\n+    raw));",
        "+consume(reinterpret_cast<int*>(\n+    raw));",
    )
    assert len(_added_casts(diff)) == 1
