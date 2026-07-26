"""Verify recovered bodies against the reviewed boundary hashes.

`just compare` answers a different question than this repository's matching
criterion. reccmp diffs the *linked* image, so every operand that names a global
or a call target counts as a difference simply because our globals do not sit at
the original addresses. A byte-exact body therefore scores well under 100%:
`AddLinesToMessageBox` is byte-identical under relocation masking and reccmp
reports 75%. reccmp is still the right tool for whole-image progress, but it
cannot tell a byte-exact body from a near miss, and its percentage must never be
used to choose between two candidate sources.

This module applies the criterion the reviewed rows actually record: mask the
relocated operands, then compare the SHA-256 of the remaining bytes. Regressions
in an already-exact body are otherwise invisible.

Two details of COMDAT layout matter:

* A COMDAT holds the switch jump tables and alignment padding that follow the
  body, while the reviewed `size` counts only the function. Comparing the whole
  section reports a false mismatch on every function with a dense `switch` --
  `MinimumCasterLevelForSpellLevel` carries 24 bytes of table plus 3 of padding.
  Bodies are therefore truncated to the reviewed size, and relocations beyond it
  dropped with them.
* Methods reach the object file decorated. The reviewed rows name them
  `Class::Method` against an implementation class called `W8Class`, so the
  decorated name is reduced to the candidates a row may plausibly use.
"""

from __future__ import annotations

import csv
import hashlib
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .sgp_oracle import (
    CoffFunction,
    _stable_ranges,
    mask_relocations,
    parse_coff_functions,
)

DECORATED = re.compile(r"^\?(?P<method>[^@]+)@(?P<owner>[^@]+)@@")
CLASS_PREFIX = "W8"


def symbol_candidates(decorated: str) -> tuple[str, ...]:
    """Names a reviewed row could use for this COFF symbol, most specific first."""

    match = DECORATED.match(decorated)
    if match is None:
        return (decorated.lstrip("_").split("@")[0],)
    method = match.group("method")
    owner = match.group("owner")
    names = [f"{owner}::{method}"]
    if owner.startswith(CLASS_PREFIX):
        names.append(f"{owner[len(CLASS_PREFIX) :]}::{method}")
    # Free-standing helpers are reviewed under the bare name even when the
    # implementation parks them in a class.
    names.append(method)
    return tuple(dict.fromkeys(names))


def collect_object_functions(root: Path) -> dict[str, CoffFunction]:
    """Index every external `.text` function under `root` by reviewable name."""

    functions: dict[str, CoffFunction] = {}
    for obj in sorted(root.rglob("*.obj")):
        try:
            parsed = parse_coff_functions(obj)
        except RuntimeError:
            # An object with no external .text function -- a runtime shim or a
            # unit that contributed only data. Nothing to verify.
            continue
        for function in parsed:
            for name in symbol_candidates(function.name):
                functions.setdefault(name, function)
    return functions


def masked_digest(function: CoffFunction, size: int) -> str:
    body = function.body[:size]
    offsets = tuple(offset for offset in function.relocation_offsets if offset + 4 <= size)
    return hashlib.sha256(mask_relocations(body, offsets)).hexdigest()


def read_canonical_body(image: Path, address: int, size: int) -> bytes:
    """Read `size` bytes of the original image at virtual address `address`."""

    import pefile

    binary = pefile.PE(str(image), fast_load=True)
    try:
        return binary.get_data(address - binary.OPTIONAL_HEADER.ImageBase, size)
    finally:
        binary.close()


def matches_canonical(function: CoffFunction, canonical: bytes) -> bool:
    """Compare against the original on every byte a relocation does not cover.

    The linked original has its relocated operands already resolved, so those
    bytes carry link-time addresses that our object cannot reproduce and must
    not be compared. Everything outside them has to agree exactly.
    """

    size = len(canonical)
    if len(function.body) < size:
        return False
    offsets = tuple(offset for offset in function.relocation_offsets if offset + 4 <= size)
    body = function.body[:size]
    return all(body[start:end] == canonical[start:end] for start, end in _stable_ranges(size, offsets))


@dataclass(frozen=True)
class BoundaryResult:
    address: str
    symbol: str
    confidence: str
    state: str
    reviewed_size: int
    comdat_size: int | None


def verify_boundaries(
    mapping_path: Path,
    object_root: Path,
    image: Path | None = None,
) -> dict[str, Any]:
    """Check every reviewed row against the objects built from our sources.

    A row is `regressed` when it is reviewed as exact and no longer matches; that
    is the failure this exists to catch. A row reviewed as a near miss that *does*
    match is `promotable`: the body became exact and the review has not caught up.

    The original image is the authority when it is available. The recorded hash
    is only a cache, and it is recorded solely on exact rows -- so hash
    comparison alone can never notice a near miss becoming exact, which is
    precisely the direction this work moves in.
    """

    if not object_root.is_dir():
        raise RuntimeError(f"no built objects to verify against: {object_root}")
    with mapping_path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    if not rows:
        raise RuntimeError(f"boundary map is empty: {mapping_path}")

    functions = collect_object_functions(object_root)
    results: list[BoundaryResult] = []
    for row in rows:
        symbol = row["symbol"].strip()
        size = int(row["size"])
        function = functions.get(symbol)
        if function is None:
            state = "not-built"
        else:
            recorded = row["relocation_masked_sha256"].strip()
            if image is not None:
                canonical = read_canonical_body(image, int(row["address"], 16), size)
                matched = matches_canonical(function, canonical)
            elif recorded:
                matched = masked_digest(function, size) == recorded
            else:
                # No image and no recorded hash: nothing to compare against.
                matched = False
            if row["confidence"].strip() == "exact":
                state = "exact" if matched else "regressed"
            else:
                state = "promotable" if matched else "near-miss"
        results.append(
            BoundaryResult(
                address=row["address"].strip(),
                symbol=symbol,
                confidence=row["confidence"].strip(),
                state=state,
                reviewed_size=size,
                comdat_size=len(function.body) if function is not None else None,
            )
        )

    counts: dict[str, int] = {}
    for result in results:
        counts[result.state] = counts.get(result.state, 0) + 1
    failures = [result for result in results if result.state in {"regressed", "promotable"}]
    summary = {
        "mapping": str(mapping_path),
        "objects": str(object_root),
        "rows": len(results),
        "states": dict(sorted(counts.items())),
        "results": [result.__dict__ for result in results],
    }
    if failures:
        detail = "; ".join(
            f"{result.address} {result.symbol} is reviewed {result.confidence} but verifies "
            f"{result.state}"
            for result in failures
        )
        raise RuntimeError(f"reviewed boundaries disagree with the built objects: {detail}")
    return summary
