from __future__ import annotations

import subprocess
from pathlib import Path

import pytest
from wiz8decomp.cast_lint import CastGateError, _added_casts, validate_cast_markers


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
