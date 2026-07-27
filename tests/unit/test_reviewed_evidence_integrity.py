"""Structural guards on the tracked evidence, one per way it has been broken.

Every check here exists because the failure it catches actually happened and
nothing else noticed. They fall into two groups: the tracked CSVs must parse
back to the columns they declare, and a reviewed row that points at an address
must point at one the evidence itself supports.

Every tracked CSV must parse back to exactly the columns it declares.

This exists because the failure it catches is silent. A field whose text
contains a comma but is written unquoted does not raise: `csv.DictReader`
simply splits it, hands the reader the fragment up to the first comma, and
drops the rest into the `None` key that nothing looks at. Where the field is
last the evidence text is merely truncated on read; where it is not, as with
`evidence` in `classes.csv`, every following column shifts by one and a
`layout_proof` silently becomes someone else's prose.

Sixty-six rows of the boundary map and one class row were in that state before
this check existed, and the tests passed throughout, because every assertion
happened to read a column left of the first stray comma.
"""

import csv
from pathlib import Path

import pytest

_ROOTS = ("evidence", "config")


def _tracked_csvs() -> list[Path]:
    repository = Path(__file__).resolve().parents[2]
    paths: list[Path] = []
    for root in _ROOTS:
        paths.extend(sorted((repository / root).rglob("*.csv")))
    assert paths, "no tracked CSVs found; the roots moved"
    return paths


@pytest.mark.parametrize("path", _tracked_csvs(), ids=lambda p: p.name)
def test_tracked_csv_rows_match_their_header(path: Path) -> None:
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream)
        header = reader.fieldnames
        assert header, f"{path} has no header"
        for line, row in enumerate(reader, start=2):
            extra = row.get(None)
            assert extra is None, (
                f"{path}:{line} has more fields than the header declares, which means a "
                f"value containing a comma was written unquoted. Stray fragments: {extra}. "
                "Write the file through csv.writer/DictWriter rather than by string joining."
            )
            assert len(row) == len(header), (
                f"{path}:{line} has {len(row)} fields against {len(header)} in the header"
            )


def test_reviewed_function_addresses_are_unique() -> None:
    """A merge that keeps both sides of an append must not duplicate an identity.

    Function identity is unique by (program, address), so a resolved conflict
    that accidentally kept a row twice is a real corruption rather than a
    cosmetic one.
    """

    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    keys = [(row["program"], row["address"]) for row in rows]
    duplicates = {key for key in keys if keys.count(key) > 1}
    assert not duplicates, f"duplicate function identities: {sorted(duplicates)}"


def test_reviewed_vtable_slots_agree_with_the_census() -> None:
    """A reviewed slot target must be what is actually in memory.

    The polymorphism census reads slot targets straight out of the image, so
    it is ground truth for what a table contains -- the reviewed model may
    legitimately correct a slot *count*, but never a target. Recording a
    target by hand instead of reading it is otherwise caught only when the
    replay tries to create a function at the invented address and fails, which
    is late, noisy, and leaves a half-materialized project behind.
    """

    repository = Path(__file__).resolve().parents[2]

    def rows(path: str) -> list[dict[str, str]]:
        with (repository / path).open(newline="", encoding="utf-8") as stream:
            return list(csv.DictReader(stream))

    census = {
        (row["vtable"], int(row["slot_index"])): row["target"]
        for row in rows("evidence/snapshots/polymorphism/slots.csv")
        if "--gog-base--" in row["program"]
    }
    addresses = {
        row["vtable_id"]: row["address"] for row in rows("evidence/reviewed/wiz8/vtables.csv")
    }

    checked = 0
    for slot in rows("evidence/reviewed/wiz8/vtable-slots.csv"):
        key = (addresses.get(slot["vtable_id"]), int(slot["slot_index"]))
        expected = census.get(key)
        if expected is None:
            continue
        checked += 1
        assert slot["target"] == expected, (
            f"{slot['vtable_id']} slot {slot['slot_index']} is recorded as {slot['target']} "
            f"but the image holds {expected}"
        )
    assert checked >= 100, f"only {checked} reviewed slots were covered by the census"


def _reviewed(name: str) -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8" / name).open(
        newline="", encoding="utf-8"
    ) as stream:
        return list(csv.DictReader(stream))


def _census(name: str) -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/snapshots/polymorphism" / name).open(
        newline="", encoding="utf-8"
    ) as stream:
        return [row for row in csv.DictReader(stream) if "--gog-base--" in row["program"]]


def test_reviewed_vtable_addresses_are_census_vftables() -> None:
    """A reviewed vtable must be a table the image actually has.

    Nothing else checks the address itself: the loader validates ids, offsets
    and slot counts against each other, so a wrong address stays internally
    consistent and only surfaces when a replay reads garbage from it.
    """

    tables = {row["address"]: row for row in _census("vtables.csv")}
    for vtable in _reviewed("vtables.csv"):
        found = tables.get(vtable["address"])
        assert found is not None, (
            f"{vtable['vtable_id']} records address {vtable['address']}, which the "
            "polymorphism census does not report as a table at all"
        )
        assert found["kind"] == "vftable", (
            f"{vtable['vtable_id']} points at a {found['kind']}, not a vftable"
        )


def test_reviewed_scalar_deleting_destructors_sit_in_their_own_vtable() -> None:
    """The recorded deleting destructor must be one of the class's own slots.

    It need not be slot 0 -- MonsterLight's is slot 5, because it derives from
    an srLight whose destructor is not first -- but a destructor that appears
    nowhere in the class's table is an address that came from somewhere other
    than the evidence.
    """

    targets: dict[str, set[str]] = {}
    for slot in _reviewed("vtable-slots.csv"):
        targets.setdefault(slot["vtable_id"], set()).add(slot["target"])

    checked = 0
    for reviewed_class in _reviewed("classes.csv"):
        vtable_id = reviewed_class["primary_vtable_id"]
        deleting = reviewed_class["scalar_deleting_destructor"]
        if not (vtable_id and deleting and vtable_id in targets):
            continue
        checked += 1
        assert deleting in targets[vtable_id], (
            f"{reviewed_class['class_name']} records {deleting} as its scalar deleting "
            f"destructor, but no slot of {vtable_id} points there"
        )
    assert checked >= 10, f"only {checked} classes were covered"


def test_reviewed_class_and_vtable_references_resolve() -> None:
    """Referential integrity across the reviewed class tables."""

    vtables = {row["vtable_id"]: row for row in _reviewed("vtables.csv")}

    for slot in _reviewed("vtable-slots.csv"):
        assert slot["vtable_id"] in vtables, (
            f"vtable-slots.csv references unknown vtable {slot['vtable_id']}"
        )

    for reviewed_class in _reviewed("classes.csv"):
        primary = reviewed_class["primary_vtable_id"]
        if not primary:
            continue
        assert primary in vtables, (
            f"{reviewed_class['class_name']} names unknown primary vtable {primary}"
        )
        assert vtables[primary]["class_name"] == reviewed_class["class_name"], (
            f"{primary} is claimed by {reviewed_class['class_name']} but belongs to "
            f"{vtables[primary]['class_name']}"
        )
