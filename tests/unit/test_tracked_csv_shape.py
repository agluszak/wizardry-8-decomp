"""Every tracked CSV must parse back to exactly the columns it declares.

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
