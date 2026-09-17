from pathlib import Path

from wiz8decomp.clang_tidy_lines import added_line_filter, redundant_cast_line_filter


def test_added_line_filter_ignores_moved_lines() -> None:
    diff = """diff --git a/src/wiz8/example.cpp b/src/wiz8/example.cpp
--- a/src/wiz8/example.cpp
+++ b/src/wiz8/example.cpp
@@ -1,2 +1,5 @@
 old();
+first();
+moved();
+second();
 keep();
diff --git a/src/wiz8/old.cpp b/src/wiz8/old.cpp
--- a/src/wiz8/old.cpp
+++ b/src/wiz8/old.cpp
@@ -4 +3,0 @@
-moved();
diff --git a/include/surrender/example.h b/include/surrender/example.h
--- a/include/surrender/example.h
+++ b/include/surrender/example.h
@@ -0,0 +1 @@
+header_change();
"""
    assert added_line_filter(diff) == "include/surrender/example.h@1;src/wiz8/example.cpp@2,4"


def test_redundant_cast_line_filter_honors_explicit_env(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.setenv("WIZ8_REDUNDANT_CAST_LINES", "*")
    assert redundant_cast_line_filter(tmp_path) == "*"


def test_redundant_cast_line_filter_is_empty_without_vcs(monkeypatch, tmp_path: Path) -> None:
    monkeypatch.delenv("WIZ8_REDUNDANT_CAST_LINES", raising=False)
    assert redundant_cast_line_filter(tmp_path) == ""
