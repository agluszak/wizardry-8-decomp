#!/usr/bin/env python3
"""Load Wizardry's clang-tidy plugin and post-process whole-program recovery facts."""

from __future__ import annotations

import os
import re
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path

REAL_CLANG_TIDY = "/usr/bin/clang-tidy-19"
PLUGIN = "/usr/local/lib/wiz8-clang-tidy.so"
# Historical name: project-specific AST debt checks use the same changed-line scope.
FILTER_ENV = "WIZ8_REDUNDANT_CAST_LINES"
BOOL_FACTS_ENV = "WIZ8_BOOL_FACTS_DIR"
BOOL_MARKER = re.compile(r"bool-byte-ok:\s*\S", re.IGNORECASE)
SCOPES = (
    "src/wiz8/",
    "include/wiz8/",
    "src/surrender/",
    "include/surrender/",
    "src/srext_jpegimporter/",
    "src/srext_unzip/",
)
CPP_SUFFIXES = (".cpp", ".cc", ".cxx", ".h", ".hpp")
HUNK = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")


@dataclass(frozen=True)
class DeclarationFact:
    key: str
    file: str
    line: int
    column: int
    kind: str
    name: str
    name_bool: bool


@dataclass(frozen=True)
class Diagnostic:
    declaration: DeclarationFact
    proven: bool


def _git(repository: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", "-C", str(repository), *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )


def _baseline(repository: Path) -> str | None:
    for candidate in ("origin/main", "origin/master", "main", "master"):
        probe = _git(repository, "rev-parse", "--verify", "--quiet", f"{candidate}^{{commit}}")
        if probe.returncode != 0 or not probe.stdout.strip():
            continue
        merge = _git(repository, "merge-base", candidate, "HEAD")
        if merge.returncode == 0 and merge.stdout.strip():
            return merge.stdout.strip()
        return probe.stdout.strip()
    return None


def _target_path(header: str) -> str | None:
    target = header[4:].strip()
    if target == "/dev/null":
        return None
    if target.startswith(("a/", "b/")):
        target = target[2:]
    if not target.startswith(SCOPES) or not target.lower().endswith(CPP_SUFFIXES):
        return None
    return target


def _added_line_filter(diff: str) -> str:
    additions: list[tuple[str, int, str]] = []
    removals: Counter[str] = Counter()
    current: str | None = None
    line_number = 0
    in_hunk = False

    for raw in diff.splitlines():
        if raw.startswith("diff --git "):
            current = None
            in_hunk = False
            continue
        if raw.startswith("+++ "):
            current = _target_path(raw)
            continue
        if raw.startswith("@@ "):
            match = HUNK.match(raw)
            in_hunk = match is not None
            if match is not None:
                line_number = int(match.group(1))
            continue
        if not in_hunk:
            continue
        if raw.startswith("+"):
            if current is not None:
                additions.append((current, line_number, raw[1:].strip()))
            line_number += 1
        elif raw.startswith("-"):
            removals[raw[1:].strip()] += 1
        elif raw.startswith(" "):
            line_number += 1
        elif raw.startswith("\\"):
            continue
        else:
            in_hunk = False

    lines: dict[str, list[int]] = defaultdict(list)
    for filename, line, text in additions:
        if removals[text]:
            removals[text] -= 1
            continue
        lines[filename].append(line)

    encoded: list[str] = []
    for filename in sorted(lines):
        numbers = sorted(set(lines[filename]))
        ranges: list[tuple[int, int]] = []
        for number in numbers:
            if ranges and number == ranges[-1][1] + 1:
                ranges[-1] = (ranges[-1][0], number)
            else:
                ranges.append((number, number))
        encoded_ranges = ",".join(
            str(first) if first == last else f"{first}-{last}" for first, last in ranges
        )
        encoded.append(f"{filename}@{encoded_ranges}")
    return ";".join(encoded)


def _changed_line_filter(repository: Path) -> str:
    base = _baseline(repository)
    if base is None:
        return ""
    diff = _git(
        repository,
        "diff",
        "--no-color",
        "--no-ext-diff",
        "--unified=0",
        base,
        "--",
        *SCOPES,
    )
    if diff.returncode != 0:
        return ""
    return _added_line_filter(diff.stdout)


def _in_scope(filename: str) -> bool:
    normalized = filename.replace("\\", "/")
    return any(normalized.startswith(scope) or f"/{scope}" in normalized for scope in SCOPES)


def _parse_line_filter(raw: str) -> dict[str, list[tuple[int, int]]]:
    parsed: dict[str, list[tuple[int, int]]] = defaultdict(list)
    if not raw or raw == "*":
        return parsed
    for entry in raw.split(";"):
        filename, separator, encoded_ranges = entry.rpartition("@")
        if not separator or not filename or not encoded_ranges:
            continue
        for encoded_range in encoded_ranges.split(","):
            first_text, separator, last_text = encoded_range.partition("-")
            try:
                first = int(first_text)
                last = int(last_text) if separator else first
            except ValueError:
                continue
            if first > 0 and last >= first:
                parsed[filename].append((first, last))
    return parsed


def _line_selected(
    raw_filter: str,
    parsed_filter: dict[str, list[tuple[int, int]]],
    filename: str,
    line: int,
) -> bool:
    if raw_filter == "*":
        return _in_scope(filename)
    return any(first <= line <= last for first, last in parsed_filter.get(filename, ()))


def _source_path(filename: str) -> Path:
    path = Path(filename)
    if path.is_absolute():
        return path
    return Path("/repo") / path


def _bool_suppressed(declaration: DeclarationFact) -> bool:
    path = _source_path(declaration.file)
    try:
        lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    except OSError:
        return False
    index = declaration.line - 1
    if index < 0 or index >= len(lines):
        return False
    if BOOL_MARKER.search(lines[index]):
        return True
    return index > 0 and BOOL_MARKER.search(lines[index - 1]) is not None


def _read_bool_facts(
    facts_dir: Path,
) -> tuple[
    dict[str, DeclarationFact],
    dict[str, list[tuple[str, tuple[str, ...]]]],
    set[str],
    set[str],
    set[str],
    list[tuple[str, str, int]],
]:
    declarations: dict[str, DeclarationFact] = {}
    writes: dict[str, list[tuple[str, tuple[str, ...]]]] = defaultdict(list)
    invalid: set[str] = set()
    escaped: set[str] = set()
    supported: set[str] = set()
    locations: list[tuple[str, str, int]] = []

    for path in sorted(facts_dir.glob("facts-*.tsv")):
        for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
            fields = raw.split("\t")
            if not fields:
                continue
            tag = fields[0]
            try:
                if tag == "D" and len(fields) >= 8:
                    fact = DeclarationFact(
                        key=fields[1],
                        file=fields[2],
                        line=int(fields[3]),
                        column=int(fields[4]),
                        kind=fields[5],
                        name=fields[6],
                        name_bool=fields[7] == "1",
                    )
                    declarations.setdefault(fact.key, fact)
                    locations.append((fact.key, fact.file, fact.line))
                elif tag == "W" and len(fields) >= 7:
                    dependencies = tuple(filter(None, fields[3].split(",")))
                    write = (fields[2], dependencies)
                    if write not in writes[fields[1]]:
                        writes[fields[1]].append(write)
                    use_line = int(fields[5])
                    locations.append((fields[1], fields[4], use_line))
                    # A changed copy/return is also a changed use of every source
                    # candidate. This matters for declaration-only predicates:
                    # touching `return HasFoo()` should surface HasFoo itself.
                    for dependency in dependencies:
                        locations.append((dependency, fields[4], use_line))
                elif tag in {"X", "E", "S"} and len(fields) >= 5:
                    key = fields[1]
                    if tag == "X":
                        invalid.add(key)
                    elif tag == "E":
                        escaped.add(key)
                    else:
                        supported.add(key)
                    locations.append((key, fields[2], int(fields[3])))
            except ValueError:
                continue

    return declarations, writes, invalid, escaped, supported, locations


def _bool_diagnostics(
    facts_dir: Path,
    raw_filter: str,
    *,
    honor_suppressions: bool = True,
) -> list[Diagnostic]:
    declarations, writes, invalid, escaped, supported, locations = _read_bool_facts(facts_dir)
    parsed_filter = _parse_line_filter(raw_filter)

    survivors = {
        key for key in declarations if writes.get(key) and key not in invalid and key not in escaped
    }
    changed = True
    while changed:
        changed = False
        for key in tuple(survivors):
            if any(
                dependency not in survivors
                for _mode, dependencies in writes[key]
                for dependency in dependencies
            ):
                survivors.remove(key)
                changed = True

    anchored: set[str] = set()
    changed = True
    while changed:
        changed = False
        for key in survivors - anchored:
            if any(
                mode == "D" or all(dependency in anchored for dependency in dependencies)
                for mode, dependencies in writes[key]
            ):
                anchored.add(key)
                changed = True

    touched: set[str] = set()
    for key, filename, line in locations:
        if _line_selected(raw_filter, parsed_filter, filename, line):
            touched.add(key)

    diagnostics: list[Diagnostic] = []
    proven = survivors & anchored
    for key in sorted(proven & touched):
        declaration = declarations[key]
        # Dependencies may cross into retained/vendor headers. A changed Wizardry
        # use must never cause a diagnostic on a declaration outside the recovered
        # source roots. The explicit-file exception exists for the plugin self-test.
        if not _in_scope(declaration.file) and declaration.file not in parsed_filter:
            continue
        if not declaration.name_bool:
            continue
        if honor_suppressions and _bool_suppressed(declaration):
            continue
        diagnostics.append(Diagnostic(declaration, proven=True))

    referenced = {
        dependency
        for entries in writes.values()
        for _mode, dependencies in entries
        for dependency in dependencies
    }
    # Missing bodies are common in the reconstruction. Do not turn name shape
    # alone into a hard error, but do surface declaration-only byte predicates
    # when recovered code actually consumes them as a predicate/domain source.
    # These warnings are inventory for recovery work and do not fail lint.
    probable = {
        key
        for key, declaration in declarations.items()
        if declaration.kind == "function"
        and declaration.name_bool
        and not writes.get(key)
        and key not in invalid
        and key not in escaped
        and (key in referenced or key in supported)
    }
    for key in sorted((probable - proven) & touched):
        declaration = declarations[key]
        if not _in_scope(declaration.file) and declaration.file not in parsed_filter:
            continue
        if honor_suppressions and _bool_suppressed(declaration):
            continue
        diagnostics.append(Diagnostic(declaration, proven=False))

    return diagnostics


def _render_bool_diagnostic(diagnostic: Diagnostic) -> str:
    declaration = diagnostic.declaration
    if diagnostic.proven:
        subject = (
            "byte-returning function" if declaration.kind == "function" else "byte declaration"
        )
        return (
            f"{declaration.file}:{declaration.line}:{declaration.column}: error: "
            f"{subject} '{declaration.name}' stays in the boolean domain across all recovered "
            "typed writes; use bool, or add 'bool-byte-ok: reason' if byte storage is intentional "
            "[wiz8-bool-like-byte]"
        )

    return (
        f"{declaration.file}:{declaration.line}:{declaration.column}: warning: "
        f"byte-returning predicate '{declaration.name}' has no recovered body, but recovered "
        "code consumes it as a boolean-domain source; review whether its return type is bool "
        "[wiz8-bool-like-byte]"
    )


def _self_test() -> None:
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
    actual = _added_line_filter(diff)
    expected = "include/surrender/example.h@1;src/wiz8/example.cpp@2,4"
    if actual != expected:
        raise SystemExit(f"changed-line filter self-test failed: {actual!r} != {expected!r}")

    with tempfile.TemporaryDirectory(prefix="wiz8-bool-self-test-") as temporary:
        root = Path(temporary)
        source = root / "bool_test.cpp"
        source.write_text("\n".join(["// test"] * 26) + "\n", encoding="utf-8")
        facts = root / "facts-1.tsv"
        rows = [
            # Direct anchor and dependency propagation.
            f"D\tA\t{source}\t1\t1\tvariable\tready\t1",
            f"W\tA\tD\t\t{source}\t2\t1",
            f"D\tB\t{source}\t3\t1\tvariable\tactive\t1",
            f"W\tB\tR\tA\t{source}\t4\t1",
            # Pure dependency cycle has no boolean anchor.
            f"D\tC\t{source}\t5\t1\tvariable\tready\t1",
            f"W\tC\tR\tD\t{source}\t6\t1",
            f"D\tD\t{source}\t7\t1\tvariable\tready\t1",
            f"W\tD\tR\tC\t{source}\t8\t1",
            # Invalidated and escaped declarations are rejected.
            f"D\tE\t{source}\t9\t1\tvariable\tready\t1",
            f"W\tE\tD\t\t{source}\t10\t1",
            f"X\tE\t{source}\t11\t1",
            f"D\tF\t{source}\t12\t1\tvariable\tready\t1",
            f"W\tF\tD\t\t{source}\t13\t1",
            f"E\tF\t{source}\t14\t1",
            # Semantic naming is required even when a value is truth-tested.
            f"D\tG\t{source}\t15\t1\tvariable\tvisible\t1",
            f"W\tG\tD\t\t{source}\t16\t1",
            f"S\tG\t{source}\t17\t1",
            # Boolean context alone must not turn an opaque byte into bool.
            f"D\tH\t{source}\t18\t1\tvariable\tvalue\t0",
            f"W\tH\tD\t\t{source}\t19\t1",
            # A declaration-only predicate used by a recovered boolean-domain
            # return is useful recovery evidence, but not a proof.
            f"D\tI\t{source}\t20\t1\tfunction\tHasLineOfSight\t1",
            f"D\tJ\t{source}\t21\t1\tfunction\tIsVisible\t1",
            f"W\tJ\tD\t\t{source}\t22\t1",
            f"W\tJ\tR\tI\t{source}\t23\t1",
        ]
        facts.write_text("\n".join(rows) + "\n", encoding="utf-8")
        selected = f"{source}@1-26"
        found = [(item.declaration.key, item.proven) for item in _bool_diagnostics(root, selected)]
        expected_found = [("A", True), ("B", True), ("G", True), ("I", False)]
        if found != expected_found:
            raise SystemExit(f"boolean fact self-test failed: {found!r}")
        if _bool_diagnostics(root, "*"):
            raise SystemExit("full audit leaked a declaration outside recovered source roots")


def main() -> None:
    if sys.argv[1:] == ["--wiz8-wrapper-self-test"]:
        _self_test()
        return

    if FILTER_ENV not in os.environ:
        repository = Path("/repo")
        # Prefer an explicit empty filter when VCS metadata is unavailable so the
        # plugin does not fall back to a silent no-op without the host having
        # decided the scope. Host-side `wiz8 lint` normally injects the filter.
        if (repository / ".git").exists():
            os.environ[FILTER_ENV] = _changed_line_filter(repository)
        else:
            os.environ[FILTER_ENV] = ""

    arguments = list(sys.argv[1:])
    if os.environ.get(FILTER_ENV) == "*":
        # Full-corpus audits intentionally produce more than Clang's default
        # diagnostic error limit. Do not truncate the cleanup inventory.
        arguments.insert(0, "--extra-arg=-ferror-limit=0")

    with tempfile.TemporaryDirectory(prefix="wiz8-bool-facts-") as facts:
        environment = os.environ.copy()
        environment[BOOL_FACTS_ENV] = facts
        result = subprocess.run(
            [REAL_CLANG_TIDY, f"--load={PLUGIN}", *arguments],
            env=environment,
            check=False,
        )
        if result.returncode != 0:
            raise SystemExit(result.returncode)

        diagnostics = _bool_diagnostics(Path(facts), environment.get(FILTER_ENV, ""))
        for diagnostic in diagnostics:
            print(_render_bool_diagnostic(diagnostic), file=sys.stderr)
        if any(diagnostic.proven for diagnostic in diagnostics):
            raise SystemExit(1)


if __name__ == "__main__":
    main()
