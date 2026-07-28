import csv
from pathlib import Path

import pytest
from wiz8decomp.evidence_merge import (
    EvidenceMergeConflict,
    key_columns,
    merge_rows,
    resolve_evidence_conflict,
    split_conflict,
    stronger,
)

_HEADER = "address,size,symbol,owner,confidence,relocation_masked_sha256,evidence"


def _row(address: str, confidence: str, digest: str = "", evidence: str = "e") -> dict[str, str]:
    return {
        "address": address,
        "size": "10",
        "symbol": "C::m",
        "owner": "wiz8-local-code",
        "confidence": confidence,
        "relocation_masked_sha256": digest,
        "evidence": evidence,
    }


def test_stronger_prefers_confidence_then_a_recorded_hash() -> None:
    assert stronger(_row("1", "exact"), _row("1", "structurally-strong"))["confidence"] == "exact"
    assert stronger(_row("1", "structurally-strong"), _row("1", "exact"))["confidence"] == "exact"
    # Equal confidence: the row that carries a hash is the one somebody verified.
    kept = stronger(_row("1", "exact", ""), _row("1", "exact", "abc"))
    assert kept["relocation_masked_sha256"] == "abc"


def test_merge_keeps_both_sides_and_never_duplicates_an_identity() -> None:
    theirs = [_row("aaa", "exact", "h1"), _row("bbb", "structurally-strong")]
    mine = [_row("ccc", "exact", "h3")]
    rows, summary = merge_rows(theirs, mine, ("address", "symbol"))
    assert [row["address"] for row in rows] == ["aaa", "bbb", "ccc"]
    assert summary["reconciled"] == []


def test_merge_does_not_demote_a_row_the_other_side_promoted() -> None:
    """The regression this module exists for.

    Resolving by appending both halves put a promoted boundary row back to
    structurally-strong and dropped its hash; only verify-boundaries noticed.
    """

    theirs = [_row("004f3d90", "structurally-strong", "")]
    mine = [_row("004f3d90", "exact", "fcc96af2")]
    rows, summary = merge_rows(theirs, mine, ("address", "symbol"))
    assert len(rows) == 1
    assert rows[0]["confidence"] == "exact"
    assert rows[0]["relocation_masked_sha256"] == "fcc96af2"
    assert summary["reconciled"] == ["004f3d90:C::m"]


@pytest.mark.parametrize(
    ("field", "left", "right"),
    [
        ("size", "10", "11"),
        ("symbol", "C::m", "C::renamed"),
        ("source_path", "A.cpp", "B.cpp"),
        ("evidence", "first proof", "second proof"),
        ("relocation_masked_sha256", "abc", "def"),
    ],
)
def test_equal_strength_semantic_disagreement_is_a_field_conflict(
    field: str, left: str, right: str
) -> None:
    first = _row("004f3d90", "exact", "abc")
    second = dict(first)
    first[field] = left
    second[field] = right

    with pytest.raises(EvidenceMergeConflict, match=field):
        merge_rows([first], [second], ("address",))


def test_empty_fields_are_filled_without_replacing_other_values() -> None:
    first = _row("004f3d90", "structurally-strong", "", evidence="")
    second = _row("004f3d90", "exact", "abc", evidence="byte proof")
    rows, _ = merge_rows([first], [second], ("address",))

    assert rows[0]["confidence"] == "exact"
    assert rows[0]["relocation_masked_sha256"] == "abc"
    assert rows[0]["evidence"] == "byte proof"


def test_split_conflict_reads_both_jj_sides() -> None:
    text = (
        f"{_HEADER}\n"
        "0001,10,A::a,o,exact,h,e\n"
        "<<<<<<< conflict 1 of 1\n"
        "+++++++ destination\n"
        "0002,10,B::b,o,exact,h2,e\n"
        "%%%%%%% diff from: parent\n"
        "\\\\\\\\        to: rebased\n"
        "+0003,10,C::c,o,exact,h3,e\n"
        ">>>>>>> conflict 1 of 1 ends\n"
    )
    common, theirs, mine = split_conflict(text)
    # The untouched rows stay out of both hunks, so the summary can report only
    # identities that genuinely appeared on both sides.
    assert "0001" in common and "0002" not in common and "0003" not in common
    assert theirs == ["0002,10,B::b,o,exact,h2,e"]
    assert mine == ["0003,10,C::c,o,exact,h3,e"]
    assert split_conflict(f"{_HEADER}\n0001,10,A::a,o,exact,h,e\n") is None


def test_resolving_a_file_writes_a_well_formed_table(tmp_path: Path) -> None:
    path = tmp_path / "wiz8-gameplay-boundaries.csv"
    path.write_text(
        f"{_HEADER}\n"
        "0001,10,A::a,o,exact,h1,kept\n"
        "<<<<<<< conflict 1 of 1\n"
        "+++++++ destination\n"
        "0002,10,B::b,o,structurally-strong,,theirs only\n"
        "0003,10,C::c,o,structurally-strong,,\n"
        "%%%%%%% diff from: parent\n"
        "\\\\\\\\        to: rebased\n"
        "+0003,10,C::c,o,exact,h3,promoted\n"
        "+0004,10,D::d,o,exact,h4,mine only\n"
        ">>>>>>> conflict 1 of 1 ends\n",
        encoding="utf-8",
    )
    summary = resolve_evidence_conflict(path)
    assert summary["conflicted"] is True

    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    assert not any(None in row for row in rows)
    by_address = {row["address"]: row for row in rows}
    assert set(by_address) == {"0001", "0002", "0003", "0004"}
    # The promoted copy of 0003 survived the merge.
    assert by_address["0003"]["confidence"] == "exact"
    assert by_address["0003"]["relocation_masked_sha256"] == "h3"

    # Re-running on the resolved file is a no-op rather than an error.
    assert resolve_evidence_conflict(path)["conflicted"] is False


def test_boundary_map_identity_is_address_not_mutable_symbol() -> None:
    assert key_columns(Path("wiz8-gameplay-boundaries.csv")) == ("address",)


def test_unknown_table_refuses_to_merge(tmp_path: Path) -> None:
    path = tmp_path / "something-else.csv"
    path.write_text("a,b\n<<<<<<< conflict\n+++++++ x\n1,2\n>>>>>>> ends\n", encoding="utf-8")
    with pytest.raises(ValueError, match="no identity columns are known"):
        resolve_evidence_conflict(path)


def test_every_evidence_table_in_the_repo_has_known_identity_columns() -> None:
    repository = Path(__file__).resolve().parents[2]
    tables = [
        *(repository / "evidence/reviewed/wiz8").glob("*.csv"),
        repository / "config/reccmp/wiz8-gameplay-boundaries.csv",
    ]
    for table in tables:
        columns = key_columns(table)
        with table.open(newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream)
            header = reader.fieldnames or []
            rows = list(reader)
        assert set(columns) <= set(header), f"{table.name} lacks {columns}"
        keys = [tuple(row[column] for column in columns) for row in rows]
        assert len(keys) == len(set(keys)), f"{table.name} identity columns are not unique"
