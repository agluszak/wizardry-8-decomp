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
    _sgp_notice_violations,
    added_lines_without_marker,
    validate_cast_markers,
)


def _added_casts(diff: str) -> list[dict[str, object]]:
    return added_lines_without_marker(diff, _CAST, _MARKER)


def _diff(path: str, *body: str) -> str:
    return "\n".join([f"diff --git a/{path} b/{path}", f"--- a/{path}", f"+++ b/{path}", *body])


def test_unmarked_added_cast_is_reported() -> None:
    diff = _diff(
        "src/wiz8/example.cpp",
        "@@ -1,1 +1,2 @@",
        " int f() {",
        "+    return reinterpret_cast<int>(value);",
    )

    assert _added_casts(diff) == [
        {"file": "src/wiz8/example.cpp", "line": 2, "text": "return reinterpret_cast<int>(value);"}
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
