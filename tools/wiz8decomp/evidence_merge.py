"""Resolve a conflicted evidence CSV without losing a stronger row.

Concurrent agents append to the same tables constantly, so these files
conflict as a matter of routine and the resolution is nearly always "keep both
sides". Doing that by hand is where it goes wrong: when both sides carry a row
for the *same* key, appending both duplicates an identity, and picking
whichever half came last can silently demote a row somebody already promoted.
That happened - a boundary row went back to ``structurally-strong`` and lost
its recorded hash, and only ``just verify-boundaries`` noticed.

So the merge is mechanical here instead. Rows only one side has are kept.
Rows both sides have are reconciled field by field. Confidence may only move
upward, and an empty field may be filled, but two different non-empty semantic
values are a conflict. The destination is not written when that happens.
"""

from __future__ import annotations

import csv
import io
import os
import tempfile
from pathlib import Path
from typing import Any

# Ordered weakest to strongest. Reviewed evidence and the boundary map use
# overlapping vocabularies, so one table covers both.
_CONFIDENCE_RANK = {
    "": 0,
    "not-built": 0,
    "structurally-strong": 1,
    "strong": 2,
    "high": 3,
    "exact": 4,
}

# The identity of a row, per table. A merge cannot be safe without knowing
# which columns name the thing the row is about.
_KEYS: dict[str, tuple[str, ...]] = {
    # Original address is the identity. A symbol is reviewed metadata and may
    # be renamed; including it here allowed two names for one function to
    # survive a merge as distinct rows.
    "wiz8-gameplay-boundaries.csv": ("address",),
    "srext-jpegimporter.csv": ("address",),
    "srext-unzip.csv": ("address",),
    "functions.csv": ("program", "address"),
    "function-evidence.csv": ("evidence_id",),
    "classes.csv": ("program", "class_name"),
    "vtables.csv": ("program", "vtable_id"),
    "vtable-slots.csv": ("program", "vtable_id", "slot_index"),
    "fields.csv": ("program", "class_name", "offset"),
    "signatures.csv": ("program", "address"),
    "imported-vftable-sites.csv": ("program", "site"),
    "allocator-layers.csv": ("address",),
    # No program column, and the ordering column repeats where two nodes share
    # a step, so the spine needs the role alongside it.
    "startup-spine.csv": ("order", "role"),
}


def key_columns(path: Path) -> tuple[str, ...]:
    try:
        return _KEYS[path.name]
    except KeyError as error:
        raise ValueError(
            f"no identity columns are known for {path.name}; add them to _KEYS "
            "rather than merging a table whose rows cannot be told apart"
        ) from error


def split_conflict(text: str) -> tuple[str, list[str], list[str]] | None:
    """Common text and the two conflicting hunks, or None when clean.

    jj writes the destination side under ``+++++++`` and the rebased side as a
    diff under ``%%%%%%%``, where added lines carry a leading ``+``. The parts
    are kept separate so the summary can say which identities genuinely
    appeared on both sides instead of counting every untouched row twice.
    """

    if "<<<<<<<" not in text:
        return None
    common: list[str] = []
    theirs: list[str] = []
    mine: list[str] = []
    section = None
    for line in text.splitlines():
        if line.startswith("<<<<<<<"):
            section = "conflict"
            continue
        if line.startswith(">>>>>>>"):
            section = None
            continue
        if section is None:
            common.append(line)
            continue
        if line.startswith("+++++++"):
            section = "theirs"
        elif line.startswith(("%%%%%%%", "\\\\\\\\")):
            section = "mine"
        elif section == "theirs":
            theirs.append(line)
        elif section == "mine" and line.startswith("+"):
            mine.append(line[1:])
    return "\n".join(common), theirs, mine


def _rows(text: str) -> tuple[list[str], list[dict[str, str]]]:
    reader = csv.DictReader(io.StringIO(text, newline=""))
    fields = list(reader.fieldnames or [])
    rows = []
    for row in reader:
        if None in row:
            raise ValueError(
                "a row has more fields than the header declares, which means a "
                "value containing a comma was written unquoted"
            )
        rows.append(dict(row))
    return fields, rows


def _hunk_rows(fields: list[str], lines: list[str]) -> list[dict[str, str]]:
    if not lines:
        return []
    _, rows = _rows("\n".join([",".join(fields), *lines]))
    return rows


class EvidenceMergeConflict(ValueError):
    """Two non-empty values claim the same field of one evidence identity."""


def stronger(left: dict[str, str], right: dict[str, str]) -> dict[str, str]:
    """Merge one identity monotonically, refusing semantic disagreement.

    Kept as the small public operation used by callers and tests; unlike the
    old implementation it does not select a whole winner row.
    """

    fields = tuple(dict.fromkeys((*left, *right)))
    merged: dict[str, str] = {}
    conflicts: list[str] = []
    for field in fields:
        left_value = left.get(field, "")
        right_value = right.get(field, "")
        if field == "confidence":
            left_rank = _CONFIDENCE_RANK.get(left_value, 0)
            right_rank = _CONFIDENCE_RANK.get(right_value, 0)
            merged[field] = left_value if left_rank >= right_rank else right_value
        elif not left_value:
            merged[field] = right_value
        elif not right_value or left_value == right_value:
            merged[field] = left_value
        else:
            conflicts.append(f"{field}: {left_value!r} != {right_value!r}")
    if conflicts:
        raise EvidenceMergeConflict("semantic evidence conflict: " + "; ".join(conflicts))
    return merged


def merge_rows(
    theirs: list[dict[str, str]],
    mine: list[dict[str, str]],
    keys: tuple[str, ...],
) -> tuple[list[dict[str, str]], dict[str, Any]]:
    def identity(row: dict[str, str]) -> tuple[str, ...]:
        return tuple(row.get(column, "") for column in keys)

    merged: dict[tuple[str, ...], dict[str, str]] = {}
    order: list[tuple[str, ...]] = []
    reconciled: list[str] = []
    for source in (theirs, mine):
        for row in source:
            token = identity(row)
            if token not in merged:
                merged[token] = row
                order.append(token)
                continue
            # Both sides describe this identity, so one copy is dropped:
            # say so rather than let a demotion pass unremarked.
            try:
                merged[token] = stronger(merged[token], row)
            except EvidenceMergeConflict as error:
                label = ":".join(token)
                raise EvidenceMergeConflict(f"identity {label}: {error}") from error
            reconciled.append(":".join(token))
    rows = [merged[token] for token in order]
    return rows, {
        "rows": len(rows),
        "only_one_side": len(order) - len(reconciled),
        "reconciled": sorted(set(reconciled)),
    }


def resolve_evidence_conflict(path: Path) -> dict[str, Any]:
    text = path.read_text(encoding="utf-8")
    sides = split_conflict(text)
    if sides is None:
        return {"path": str(path), "conflicted": False}

    keys = key_columns(path)
    common_text, their_lines, my_lines = sides
    fields, common_rows = _rows(common_text)
    if not fields:
        raise ValueError(f"{path} has no header outside the conflict")
    their_rows = _hunk_rows(fields, their_lines)
    my_rows = _hunk_rows(fields, my_lines)

    rows, summary = merge_rows(common_rows + their_rows, my_rows, keys)
    their_fields = fields
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=their_fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)

    handle, temporary = tempfile.mkstemp(dir=str(path.parent), suffix=".csv")
    with os.fdopen(handle, "w", newline="", encoding="utf-8") as file:
        file.write(stream.getvalue())
    os.replace(temporary, path)
    return {"path": str(path), "conflicted": True, "keys": list(keys), **summary}
