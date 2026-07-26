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
* Symbols reach the object file decorated, because the game is C++ and its
  translation units are `.cpp`. Methods arrive as `?Method@W8Class@@...` while
  the reviewed rows name them `Class::Method`, and free functions arrive as
  `?Name@@...` with no owner at all, so the decorated name is reduced to the
  candidates a row may plausibly use.
"""

from __future__ import annotations

import csv
import hashlib
import re
from dataclasses import dataclass
from difflib import SequenceMatcher
from pathlib import Path
from typing import Any

from .sgp_oracle import (
    CoffFunction,
    _stable_ranges,
    mask_relocations,
    parse_coff_functions,
)

DECORATED = re.compile(r"^\?(?P<method>[^@]+)@(?P<owner>[^@]*)@@")
INT_TEMPLATE_MEMBER = re.compile(
    r"^\?(?P<method>[^@]+)@\?\$(?P<owner>[^@]+)@H@@"
)
SPECIAL_MEMBER = re.compile(r"^\?\?(?P<kind>0|1|_G|_E)(?P<owner>[^@]+)@@")
CLASS_PREFIX = "W8"


def symbol_candidates(decorated: str) -> tuple[str, ...]:
    """Names a reviewed row could use for this COFF symbol, most specific first."""

    int_template = INT_TEMPLATE_MEMBER.match(decorated)
    if int_template is not None:
        method = int_template.group("method")
        owner = int_template.group("owner")
        return (f"{owner}<int>::{method}", method)

    special = SPECIAL_MEMBER.match(decorated)
    if special is not None:
        owner = special.group("owner")
        kind = special.group("kind")
        method = {
            "0": owner,
            "1": f"~{owner}",
            "_G": "scalar_deleting_destructor",
            "_E": "vector_deleting_destructor",
        }[kind]
        names = [f"{owner}::{method}"]
        if owner.startswith(CLASS_PREFIX):
            short_owner = owner[len(CLASS_PREFIX) :]
            short_method = (
                short_owner
                if kind == "0"
                else f"~{short_owner}"
                if kind == "1"
                else method
            )
            names.append(f"{short_owner}::{short_method}")
        return tuple(names)

    match = DECORATED.match(decorated)
    if match is None:
        # Template owners carry additional @-delimited arguments, so the
        # simple class regex above cannot recover a qualified owner. The bare
        # member name is still unambiguous in the reviewed boundary map.
        return (decorated.lstrip("_?").split("@")[0],)
    method = match.group("method")
    owner = match.group("owner")
    if not owner:
        # A free function in a C++ unit: ?Name@@YA... carries no owner.
        return (method,)
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
    return all(
        body[start:end] == canonical[start:end] for start, end in _stable_ranges(size, offsets)
    )


def disassemble(body: bytes, address: int) -> list[tuple[int, int, str]]:
    """Decode `body` as 32-bit x86: `(offset, size, text)` per instruction."""

    import capstone

    engine = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    return [
        (
            instruction.address - address,
            instruction.size,
            f"{instruction.mnemonic} {instruction.op_str}".strip(),
        )
        for instruction in engine.disasm(body, address)
    ]


def diff_boundary(
    mapping_path: Path,
    object_root: Path,
    image: Path,
    symbol: str,
) -> dict[str, Any]:
    """Align our body against the original instruction by instruction.

    Relocated operands are reported as `reloc` rather than as differences: our
    object cannot carry the original's link-time addresses, and treating them as
    mismatches buries the real ones. Branch displacements shift whenever the
    bodies differ in size, so a branch whose target lands at the same *index* on
    both sides is not a difference either.
    """

    with mapping_path.open(newline="", encoding="utf-8") as stream:
        rows = {row["symbol"].strip(): row for row in csv.DictReader(stream)}
    row = rows.get(symbol)
    if row is None:
        raise RuntimeError(f"{symbol} is not a reviewed boundary in {mapping_path}")
    function = collect_object_functions(object_root).get(symbol)
    if function is None:
        raise RuntimeError(f"{symbol} was not found in the objects under {object_root}")

    address = int(row["address"], 16)
    size = int(row["size"])
    canonical = disassemble(read_canonical_body(image, address, size), address)
    ours = disassemble(
        function.body[:size] if len(function.body) > size else function.body, address
    )
    relocated = set(function.relocation_offsets)

    def is_relocated(offset: int, length: int) -> bool:
        return any(offset <= point < offset + length for point in relocated)

    # Align on normalised text so one extra or missing instruction shifts a
    # single line rather than every line after it.
    left_keys = [_alignment_key(item, canonical, address) for item in canonical]
    right_keys = [
        _alignment_key(item, ours, address, relocated=is_relocated(item[0], item[1]))
        for item in ours
    ]
    pairs: list[tuple[tuple[int, int, str] | None, tuple[int, int, str] | None]] = []
    matcher = SequenceMatcher(a=left_keys, b=right_keys, autojunk=False)
    for tag, left_start, left_end, right_start, right_end in matcher.get_opcodes():
        if tag == "equal":
            pairs += list(zip(canonical[left_start:left_end], ours[right_start:right_end]))
            continue
        left_run = canonical[left_start:left_end]
        right_run = ours[right_start:right_end]
        for offset in range(max(len(left_run), len(right_run))):
            pairs.append(
                (
                    left_run[offset] if offset < len(left_run) else None,
                    right_run[offset] if offset < len(right_run) else None,
                )
            )

    lines: list[dict[str, Any]] = []
    differing = 0
    for index, (left, right) in enumerate(pairs):
        left_text = left[2] if left else ""
        right_text = right[2] if right else ""
        if (
            left is not None
            and right is not None
            and left_text == right_text
            and left[1] == right[1]
        ):
            state = "same"
        elif right is not None and is_relocated(right[0], right[1]):
            state = "reloc"
        elif (
            left is not None
            and right is not None
            and left_text.split()[0] == right_text.split()[0]
            and left_text.startswith("j")
            and _branch_index(canonical, left, address) == _branch_index(ours, right, address)
        ):
            # A branch whose displacement moved only because the bodies differ
            # in size. Compare where it lands, by index, not by address.
            state = "same"
        else:
            state = "differ"
        if state == "differ":
            differing += 1
        lines.append(
            {
                "index": index,
                "state": state,
                "canonical": f"{left[1]}B {left_text}" if left else "",
                "ours": f"{right[1]}B {right_text}" if right else "",
            }
        )
    return {
        "symbol": symbol,
        "address": row["address"],
        "confidence": row["confidence"].strip(),
        "canonical_size": size,
        "our_size": len(function.body),
        "canonical_instructions": len(canonical),
        "our_instructions": len(ours),
        "differing": differing,
        "lines": lines,
    }


ADDRESS_LIKE = re.compile(r"0x[0-9a-f]{5,}")


def _alignment_key(
    instruction: tuple[int, int, str],
    listing: list[tuple[int, int, str]],
    base: int,
    *,
    relocated: bool = False,
) -> str:
    """Text to align on, with what cannot agree between the two sides removed.

    Our object carries zeros where the original carries link-time addresses, and
    branch displacements move whenever the bodies differ in size. Both would
    otherwise defeat the alignment and hide the real difference.
    """

    text = instruction[2]
    if text.startswith(("j", "call ")):
        landing = _branch_index(listing, instruction, base)
        if landing is not None:
            return f"{text.split()[0]} ->{landing}"
    if relocated:
        return ADDRESS_LIKE.sub("<abs>", text.replace(" 0", " <abs>"))
    return ADDRESS_LIKE.sub("<abs>", text)


def _branch_index(
    listing: list[tuple[int, int, str]],
    instruction: tuple[int, int, str],
    base: int,
) -> int | None:
    """Which instruction index a branch lands on, or None if it leaves the body."""

    try:
        target = int(instruction[2].split()[-1], 0) - base
    except ValueError:
        return None
    for index, (offset, _size, _text) in enumerate(listing):
        if offset == target:
            return index
    return None


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
