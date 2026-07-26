from __future__ import annotations

import csv
from collections import Counter
from pathlib import Path


def _snapshot(name: str) -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/snapshots/call-sites" / name).open(
        encoding="utf-8", newline=""
    ) as stream:
        return list(csv.DictReader(stream))


def test_assertion_snapshot_is_keyed_by_program_and_call_site() -> None:
    rows = _snapshot("assertions.csv")

    keys = [(row["program"], row["call_site"]) for row in rows]
    assert len(keys) == len(set(keys))


def test_recovered_assertions_carry_a_source_path_and_a_line() -> None:
    rows = _snapshot("assertions.csv")

    resolved = [row for row in rows if row["source_path"]]
    assert len(resolved) >= 0.99 * len(rows)
    assert all(row["source_path"].casefold().endswith((".cpp", ".c", ".hpp", ".h")) for row in resolved)
    assert all(row["line"].isdigit() for row in resolved if row["line"])


def test_inline_members_are_attributed_to_their_header() -> None:
    """Assertions inside header-declared inline members name the .hpp, not a .cpp.

    Those units never appear in the translation-unit path list, so they are only
    visible here.
    """
    rows = _snapshot("assertions.csv")

    headers = {row["source_path"] for row in rows if row["source_path"].casefold().endswith(".hpp")}
    assert headers


def test_register_indirect_sites_are_recovered_too() -> None:
    """A byte search for `call [slot]` cannot see these, and they are numerous."""
    rows = _snapshot("assertions.csv")

    kinds = Counter(row["call_kind"] for row in rows)
    assert kinds["register-indirect"] > 0
    assert kinds["direct"] > 0


def test_the_demo_contributes_source_units_retail_does_not() -> None:
    rows = _snapshot("assertions.csv")

    by_program: dict[str, set[str]] = {}
    for row in rows:
        if row["source_path"]:
            by_program.setdefault(row["program"], set()).add(row["source_path"])
    demo = next((paths for name, paths in by_program.items() if "--demo--" in name), set())
    retail = next((paths for name, paths in by_program.items() if "--gog-base--" in name), set())

    assert demo and retail
    # Paths differ by build root, so compare the unit names rather than the paths.
    demo_units = {path.rsplit("\\", 1)[-1].casefold() for path in demo}
    retail_units = {path.rsplit("\\", 1)[-1].casefold() for path in retail}
    assert demo_units - retail_units


def test_runtime_class_names_are_recorded_with_their_call_site() -> None:
    rows = _snapshot("runtime-class-names.csv")

    assert rows
    keys = [(row["program"], row["call_site"]) for row in rows]
    assert len(keys) == len(set(keys))
    assert any(row["name"] for row in rows)
