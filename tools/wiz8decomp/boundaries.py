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

from .binary.demangle import demangle
from .sgp_oracle import (
    CoffFunction,
    _stable_ranges,
    mask_relocations,
    parse_coff_functions,
)

DECORATED = re.compile(r"^\?(?P<method>[^@]+)@(?P<owner>[^@]*)@@")
SPECIAL_MEMBER = re.compile(r"^\?\?(?P<kind>0|1|_G|_E)(?P<owner>[^@]+)@@")
TEMPLATE_OWNER = "?$"
CLASS_PREFIX = "W8"
# Element-type keywords and the backtick-quoted compiler-generated method names
# `llvm-undname` prints, mapped onto the spellings a reviewed row uses.
ARGUMENT_KEYWORDS = ("class ", "struct ", "enum ", "union ")
GENERATED_METHODS = {
    "`scalar deleting dtor'": "scalar_deleting_destructor",
    "`vector deleting dtor'": "vector_deleting_destructor",
}


def template_member_candidates(signature: str) -> tuple[str, ...]:
    """`Owner<Args>::method` names for a demangled template-member signature.

    The vector template emits one COMDAT set per element type, so its members
    arrive decorated with a full template-argument list that the class regex
    above cannot split on `@`. Rather than approximate that grammar here, the
    owner is read back out of `llvm-undname`'s output: everything after the
    last top-level space, with `<>` nesting keeping the spaces inside an
    argument list from ending the name early.
    """

    qualified = _trailing_qualified_name(_without_parameter_list(signature))
    if "::" not in qualified:
        return ()
    owner, _, method = qualified.rpartition("::")
    owner = _element_type_spelling(owner)
    method = GENERATED_METHODS.get(method, _element_type_spelling(method))
    # A template's own constructor and destructor repeat the whole argument
    # list; a reviewed row names the class once.
    base = owner.partition("<")[0]
    if method == owner:
        return (f"{owner}::{base}",)
    if method.lstrip("~") == owner:
        return (f"{owner}::~{base}",)
    if method in GENERATED_METHODS.values():
        return (f"{owner}::{method}",)
    # An ordinary member: reviewable under the bare name too, the way a
    # non-template method is, since the two srVector3T rows already are.
    return (f"{owner}::{method}", method)


def _without_parameter_list(signature: str) -> str:
    """`signature` up to the parameter list every demangled function ends with."""

    if not signature.endswith(")"):
        return signature
    depth = 0
    for index in range(len(signature) - 1, -1, -1):
        if signature[index] == ")":
            depth += 1
        elif signature[index] == "(":
            depth -= 1
            if depth == 0:
                return signature[:index]
    return signature


def _trailing_qualified_name(text: str) -> str:
    """The qualified name `text` ends with, dropping the return type before it.

    Spaces are only a boundary at the top level: they also separate template
    arguments and appear inside the backtick-quoted names the demangler prints
    for compiler-generated methods.
    """

    depth = 0
    quoted = False
    for index in range(len(text) - 1, -1, -1):
        character = text[index]
        if character == "'":
            quoted = True
        elif character == "`":
            quoted = False
        elif quoted:
            continue
        elif character == ">":
            depth += 1
        elif character == "<":
            depth -= 1
        elif character == " " and depth == 0:
            return text[index + 1 :]
    return text


def _element_type_spelling(name: str) -> str:
    """The demangler's argument spelling reduced to the header's own."""

    for keyword in ARGUMENT_KEYWORDS:
        name = name.replace(keyword, "")
    return name.replace(" *", "*").replace(" &", "&")


def symbol_candidates(decorated: str, signature: str = "") -> tuple[str, ...]:
    """Names a reviewed row could use for this COFF symbol, most specific first.

    `signature` is this symbol's demangled form, which only a template owner
    needs; `collect_object_candidates` demangles those in one batch.
    """

    if TEMPLATE_OWNER in decorated and signature:
        # `?$` also appears in a free function that merely takes a template
        # instantiation as a parameter, so the demangled owner decides: a
        # signature with no qualified name falls through to the rules below.
        template = template_member_candidates(signature)
        if template:
            return template

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


class AmbiguousBoundarySymbol(RuntimeError):
    """A reviewed row cannot be tied to one of the bodies claiming its name."""


class BoundariesDisagree(RuntimeError):
    """The verdict is a failure, but the full report is still the answer.

    Raising a bare error here used to swallow the report: one regressed row
    printed a sentence and discarded the states of the other three hundred,
    which every investigation then had to regenerate by hand. The report rides
    along so the CLI can emit it before failing.
    """

    def __init__(self, message: str, report: dict[str, Any]) -> None:
        super().__init__(message)
        self.report = report


def collect_object_candidates(root: Path) -> dict[str, tuple[CoffFunction, ...]]:
    """Index every external `.text` function under `root`, keeping every claimant.

    One name can legitimately have more than one body here. The vendored SGP
    tree and the recovered first-party tree both define WinMain: SGP's is the
    upstream body, Wizardry shipped a modified one, and only the second is what
    the image contains. CMake does not prune stale target folders, so its
    current target manifest filters those before candidate resolution.

    So the name alone cannot choose. The reviewed row does, through the address
    and size it states, which is what `resolve_boundary_function` matches on.
    """

    target_manifest = root / "TargetDirectories.txt"
    active_targets: set[str] | None = None
    if target_manifest.is_file():
        active_targets = {
            Path(line.strip().replace("\\", "/")).name
            for line in target_manifest.read_text(encoding="utf-8").splitlines()
            if line.strip()
        }

    bodies: list[CoffFunction] = []
    for obj in sorted(root.rglob("*.obj")):
        # CMake compiles its own probes -- compiler identification, the
        # /showIncludes check -- under CMakeFiles but outside any target
        # directory, and several of them define main. They are not build output
        # of ours and must not reach the index.
        if not any(part.endswith(".dir") for part in obj.parts):
            continue
        if active_targets is not None:
            target = next(part for part in obj.relative_to(root).parts if part.endswith(".dir"))
            if target not in active_targets:
                continue
        try:
            bodies.extend(parse_coff_functions(obj))
        except RuntimeError:
            # An object with no external .text function -- a runtime shim or a
            # unit that contributed only data. Nothing to verify.
            continue

    # Only template members need the demangler, so an object tree without one
    # never requires the tool to be installed.
    signatures = demangle([f.name for f in bodies if TEMPLATE_OWNER in f.name])

    functions: dict[str, list[CoffFunction]] = {}
    for function in bodies:
        for name in symbol_candidates(function.name, signatures.get(function.name, "")):
            claimants = functions.setdefault(name, [])
            if not any(bytes(seen.body) == bytes(function.body) for seen in claimants):
                claimants.append(function)
    return {name: tuple(claimants) for name, claimants in functions.items()}


def resolve_boundary_function(
    claimants: tuple[CoffFunction, ...], size: int, canonical: bytes | None
) -> CoffFunction | None:
    """Pick the body a reviewed row of this size is claiming.

    Matching the original outright settles it. Failing that -- which is the
    normal case for a row that is not yet exact -- the reviewed size does, since
    a row states the extent of the body it describes. A name whose claimants
    cannot be told apart that way resolves to nothing rather than to an
    arbitrary one, so the row reports as not-built instead of being measured
    against a body that is not its own.
    """

    if not claimants:
        return None
    if len(claimants) == 1:
        return claimants[0]
    if canonical is not None:
        matching = [f for f in claimants if matches_canonical(f, canonical)]
        if len(matching) == 1:
            return matching[0]
    sized = [f for f in claimants if len(f.body) == size]
    if len(sized) == 1:
        return sized[0]
    return None


def collect_object_functions(root: Path) -> dict[str, CoffFunction]:
    """Index by name, keeping only the names exactly one body claims."""

    return {
        name: claimants[0]
        for name, claimants in collect_object_candidates(root).items()
        if len(claimants) == 1
    }


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
    address = int(row["address"], 16)
    size = int(row["size"])
    canonical_body = read_canonical_body(image, address, size)
    function = resolve_boundary_function(
        collect_object_candidates(object_root).get(symbol, ()), size, canonical_body
    )
    if function is None:
        raise RuntimeError(f"{symbol} was not found in the objects under {object_root}")

    canonical = disassemble(canonical_body, address)
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
    # The object whose body was measured. A stale object left under the build
    # tree claims a symbol exactly like a fresh one and turns the report into a
    # false regression or a false exact; naming the claimant makes the next
    # such failure a one-look diagnosis instead of a build-directory wipe.
    object: str | None = None


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

    claimants_by_name = collect_object_candidates(object_root)
    results: list[BoundaryResult] = []
    for row in rows:
        symbol = row["symbol"].strip()
        size = int(row["size"])
        canonical = (
            read_canonical_body(image, int(row["address"], 16), size)
            if image is not None
            else None
        )
        function = resolve_boundary_function(
            claimants_by_name.get(symbol, ()), size, canonical
        )
        if function is None:
            state = "not-built"
        else:
            recorded = row["relocation_masked_sha256"].strip()
            if canonical is not None:
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
                object=(
                    str(Path(function.source_object).relative_to(object_root))
                    if function is not None and function.source_object
                    else None
                ),
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
        raise BoundariesDisagree(
            f"reviewed boundaries disagree with the built objects: {detail}", summary
        )
    return summary
