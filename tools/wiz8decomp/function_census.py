"""Enumerate candidate function starts and the static call graph.

Every other producer here keys on a function start, and nothing enumerated
them: exception records pin about six hundred exactly, assertion call sites
imply a few hundred more, and the rest of the image had no inventory at all.

Four independent things attest a start, and they fail in different ways, which
is why the census records which ones agree rather than merging them:

* an exception record reaches its function through a handler thunk whose frame
  setup is the entry point - read, not inferred, and the only exact source;
* a direct ``call`` names its target;
* a code address stored in data is a vtable slot or a callback;
* the byte after inter-function padding is usually an entry point, and is by far
  the noisiest of the four because alignment padding also appears inside
  functions and after data.

Byte scanning proposes candidates; the disassembler disposes of them. A
candidate has to be an instruction boundary in a resynchronising linear decode,
which is what rejects an address that is really data or a misread. The call
graph is built from decoded ``call`` instructions rather than from a byte search
for 0xE8, because a byte search is acceptable for proposing a target - every one
is validated afterwards - but would invent edges from an 0xE8 inside an
immediate, and nothing downstream would catch that.

A recognised prologue is recorded but is deliberately **not** required. Measured
against the vtable slots the polymorphism census already proves are entry
points, the prologue shapes cover about two thirds: a leaf that opens by reading
``[esp+4]``, or a body that opens with ``mov eax, fs:[0]``, has no frame to set
up. Gating on one would reject a third of the real functions.

Acceptance is deliberately not done here. This writes candidates with their
evidence and a verdict; promoting one to a canonical identity is a reviewed
decision that lives in ``evidence/reviewed`` and must not be overwritten when
candidates are regenerated.
"""

from __future__ import annotations

import bisect
import csv
import io
import struct
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .binary.code import (
    covering_index,
    disassembler,
    lookup_covering,
    relocation_sites,
    sweep_text,
)
from .binary.image import PeImage
from .binary.inventory import is_first_party, representative_modules
from .config import Settings
from .ghidra.project import program_name
from .paths import atomic_write

_SNAPSHOT_NAME = "functions"
_REPORT_FILES = ("candidates.csv", "calls.csv")
_FUNC_INFO_MAGIC = 0x19930520

# Prologue shapes VC6 emits. `push -1` is the C++ exception frame setup, and
# `mov edi, edi` is the hot-patch pad some library code carries.
_PROLOGUES: tuple[tuple[bytes, str], ...] = (
    (b"\x55\x8b\xec", "push-ebp-mov-esp"),
    (b"\x6a\xff", "eh-frame-setup"),
    (b"\x83\xec", "sub-esp-imm8"),
    (b"\x81\xec", "sub-esp-imm32"),
    (b"\x8b\xff", "hot-patch-pad"),
    (b"\x53", "push-ebx"),
    (b"\x55", "push-ebp"),
    (b"\x56", "push-esi"),
    (b"\x57", "push-edi"),
)


_TERMINATORS = {"ret", "retn", "jmp", "int3", "iret", "ud2"}


@dataclass
class Candidate:
    address: int
    sources: set[str] = field(default_factory=set)
    prologue: str = ""
    aligned: bool = False
    after_boundary: bool = False
    verdict: str = "rejected"
    size: int | None = None
    callers: int = 0
    callees: int = 0


def _eh_starts(image: PeImage) -> set[int]:
    """Entry points pinned by an exception record, via its handler thunk."""
    text = image.text
    data = image.data
    low, high = text.raw_offset, text.raw_offset + text.raw_size
    records = set(image.find_all(struct.pack("<I", _FUNC_INFO_MAGIC)))
    if not records:
        return set()
    thunks: dict[int, int] = {}
    for offset in range(low, max(low, high - 5)):
        if data[offset] != 0xB8:
            continue
        immediate = struct.unpack_from("<I", data, offset + 1)[0]
        if immediate in records:
            address = image.virtual_address(offset)
            if address is not None:
                thunks[address] = immediate
    starts: set[int] = set()
    for offset in range(low, max(low, high - 5)):
        if data[offset] != 0x68:
            continue
        immediate = struct.unpack_from("<I", data, offset + 1)[0]
        if immediate not in thunks:
            continue
        site = image.virtual_address(offset)
        # The setup is `push -1; push <thunk>` and VC6 emits it first.
        if site is not None and image.read(site - 2, 2) == b"\x6a\xff":
            starts.add(site - 2)
    return starts


def _call_targets(image: PeImage, decoded: list[Any]) -> tuple[Counter, list[tuple[int, int]]]:
    """Direct call targets, and every (site, target) edge.

    Taken from decoded instructions rather than from a byte search for 0xE8.
    The byte search is fine for proposing a target, because every candidate is
    validated against the decode afterwards, but it is not fine for an edge: an
    0xE8 occurring inside an immediate or a displacement would invent a call
    that is not there, and nothing downstream would catch it.
    """
    from capstone import CS_OP_IMM

    targets: Counter = Counter()
    edges: list[tuple[int, int]] = []
    for instruction in decoded:
        if instruction.mnemonic != "call":
            continue
        operands = instruction.operands
        if len(operands) != 1 or operands[0].type != CS_OP_IMM:
            continue
        target = operands[0].imm
        if image.is_code(target):
            targets[target] += 1
            edges.append((instruction.address, target))
    return targets, edges


def _data_pointers(image: PeImage) -> set[int]:
    """Code addresses stored in data: vtable slots and callbacks."""
    text = image.text
    low = text.virtual_address
    high = low + text.raw_size
    found: set[int] = set()
    for site in relocation_sites(image):
        if low <= site < high:
            continue
        value = image.read_u32(site)
        if value and image.is_code(value):
            found.add(value)
    return found


def _padding_starts(image: PeImage) -> set[int]:
    """Bytes following a run of int3 or nop inside `.text`."""
    text = image.text
    data = image.data
    low, high = text.raw_offset, text.raw_offset + text.raw_size
    found: set[int] = set()
    cursor = low
    while cursor < high:
        byte = data[cursor]
        if byte not in (0xCC, 0x90):
            cursor += 1
            continue
        run = cursor
        while run < high and data[run] == byte:
            run += 1
        if run < high:
            address = image.virtual_address(run)
            if address is not None:
                found.add(address)
        cursor = run
    return found


def _prologue(image: PeImage, address: int) -> str:
    head = image.read(address, 3)
    for pattern, name in _PROLOGUES:
        if head.startswith(pattern):
            return name
    return ""


def analyse_image(path: Path) -> dict[str, Any]:
    image = PeImage(path)
    engine = disassembler()
    decoded = sweep_text(image, engine)
    starts_index, ordered = covering_index(decoded)
    boundaries = set(starts_index)

    eh = _eh_starts(image)
    call_targets, edges = _call_targets(image, decoded)
    pointers = _data_pointers(image)
    padding = _padding_starts(image)

    candidates: dict[int, Candidate] = {}

    def note(address: int, source: str) -> None:
        entry = candidates.get(address)
        if entry is None:
            entry = Candidate(address=address)
            candidates[address] = entry
        entry.sources.add(source)

    for address in eh:
        note(address, "eh-record")
    for address in call_targets:
        note(address, "call-target")
    for address in pointers:
        note(address, "data-pointer")
    for address in padding:
        note(address, "padding")

    def after_boundary(address: int) -> bool:
        """Whether something ends immediately before ``address``.

        A function entry follows padding or the previous function's terminator.
        A switch-case label, which a jump table also stores as a code address,
        follows an ordinary instruction.
        """
        if image.read(address - 1, 1) in (b"\xcc", b"\x90"):
            return True
        for back in range(1, 10):
            instruction = lookup_covering(starts_index, ordered, address - back, 1)
            if instruction is None:
                continue
            if instruction.address + instruction.size == address:
                return instruction.mnemonic in _TERMINATORS
        return False

    for entry in candidates.values():
        entry.aligned = entry.address in boundaries
        entry.prologue = _prologue(image, entry.address)
        entry.after_boundary = after_boundary(entry.address)
        # An exception record reaches its function through the frame setup, so
        # its entry point is read rather than guessed.
        attested = bool(entry.sources & {"call-target", "data-pointer"})
        if "eh-record" in entry.sources:
            entry.verdict = "exact"
        elif attested and not entry.aligned:
            # Something really refers to this address, but the linear decode did
            # not land on it. The sweep resynchronises after embedded data and
            # can stay out of phase for a stretch, so the disagreement is worth
            # surfacing rather than filing under "no evidence" - a handful of
            # vtable slots the polymorphism census resolved land here.
            entry.verdict = "decode-disagrees"
        elif not entry.aligned:
            # Nothing refers to it and it is not an instruction boundary.
            entry.verdict = "rejected"
        elif attested:
            # A prologue is deliberately NOT required. Measured against the
            # vtable slots the polymorphism census already proves are entry
            # points, the recognised prologue shapes cover only two thirds:
            # a leaf reading `[esp+4]`, or one opening with `mov eax, fs:[0]`,
            # has no frame to set up. Requiring one would reject a third of the
            # real functions.
            entry.verdict = "strong"
        else:
            # Padding is the only thing attesting this, and padding also appears
            # inside functions and after data. Keep it visible, do not accept it.
            entry.verdict = "padding-only"

    accepted = sorted(
        entry.address for entry in candidates.values() if entry.verdict in {"exact", "strong"}
    )
    for index, address in enumerate(accepted):
        entry = candidates[address]
        entry.size = accepted[index + 1] - address if index + 1 < len(accepted) else None

    # Attribute each call site to the accepted function containing it.
    graph: dict[tuple[int, int], int] = defaultdict(int)
    for site, target in edges:
        position = bisect.bisect_right(accepted, site) - 1
        if position < 0:
            continue
        graph[(accepted[position], target)] += 1
    callers: Counter = Counter()
    callees: Counter = Counter()
    for caller, callee in graph:
        callees[caller] += 1
        callers[callee] += 1
    for address, entry in candidates.items():
        entry.callers = callers.get(address, 0)
        entry.callees = callees.get(address, 0)

    return {"candidates": candidates, "graph": graph, "accepted": accepted}


def _csv_text(fields: list[str], rows: list[dict[str, Any]]) -> str:
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return stream.getvalue()


def _hex(value: Any) -> str:
    return f"{value:08x}" if isinstance(value, int) else ""


def _snapshot_readme() -> str:
    return """# Function-candidate and call-graph snapshot

Candidate function starts and the static call graph for every first-party Wizardry executable whose
code is readable. Tracked because reproduction needs the proprietary binaries.

The producer is `wiz8decomp.function_census`. Normal runs write the same CSVs under
`build/reports/functions/` and fail when they differ from this snapshot:

```sh
uv run wiz8 function-census                  # verify against the snapshot
uv run wiz8 function-census --update-snapshot
```

These are candidates, not identities. Promoting a start to a canonical function is a reviewed
decision and belongs in `evidence/reviewed/wiz8/function-provenance.csv`; regenerating this snapshot cannot
overwrite one, because nothing here writes there.

`sources` lists which independent things attest the address, and they are not equal. `eh-record` is
read from the exception tables through the handler thunk and is exact. `call-target` and
`data-pointer` are strong when the address also begins a prologue. `padding` is by far the noisiest,
because alignment padding also appears inside functions and after data, so a padding-only candidate
is only kept when it both lands on an instruction boundary and begins a prologue.

`aligned` records whether the address is an instruction boundary in a resynchronising linear decode.
A candidate that is not is data or a misread and is rejected whatever else attests it, short of an
exception record.

`verdict` is `exact`, `strong`, `padding-prologue`, `called-no-prologue`, `referenced-no-prologue`
or `rejected`. The two `no-prologue` verdicts are deliberately not accepted and deliberately not
discarded: a tail-merged or hand-written body reached by a real call is exactly what they look like,
and so is a jump-table entry.

`size` is the distance to the next accepted start, so it is an upper bound rather than the body
length.

`calls.csv` is the deduplicated call graph: one row per `(caller, callee)` with the number of sites.
`caller` is the accepted function containing the call site, so an edge is only as good as the
boundary beneath it - which is the reason this producer emits both tables rather than either alone.
"""


def sweep_function_census(settings: Settings, *, update_snapshot: bool = False) -> dict[str, Any]:
    modules, aliases = representative_modules(settings, is_first_party)
    candidate_rows: list[dict[str, Any]] = []
    call_rows: list[dict[str, Any]] = []
    per_program: dict[str, dict[str, int]] = {}
    skipped: list[str] = []

    for module in modules:
        program = program_name(module)
        path = settings.work_dir / "variants" / module["variant"] / module["relative_path"]
        if not path.is_file():
            raise RuntimeError(f"module payload is missing: {path}")
        if module.get("packed"):
            skipped.append(program)
            continue
        result = analyse_image(path)
        candidates: dict[int, Candidate] = result["candidates"]
        verdicts = Counter(entry.verdict for entry in candidates.values())
        per_program[program] = {
            "candidates": len(candidates),
            "accepted": len(result["accepted"]),
            "call_edges": len(result["graph"]),
            **{f"verdict_{name}": count for name, count in sorted(verdicts.items())},
        }
        for address in sorted(candidates):
            entry = candidates[address]
            candidate_rows.append(
                {
                    "program": program,
                    "address": _hex(entry.address),
                    "sources": " ".join(sorted(entry.sources)),
                    "aligned": "yes" if entry.aligned else "",
                    "after_boundary": "yes" if entry.after_boundary else "",
                    "prologue": entry.prologue,
                    "verdict": entry.verdict,
                    "size": entry.size if entry.size is not None else "",
                    "caller_count": entry.callers,
                    "callee_count": entry.callees,
                }
            )
        for (caller, callee), count in sorted(result["graph"].items()):
            call_rows.append(
                {
                    "program": program,
                    "caller": _hex(caller),
                    "callee": _hex(callee),
                    "call_sites": count,
                }
            )

    candidate_rows.sort(key=lambda row: (row["program"], row["address"]))
    call_rows.sort(key=lambda row: (row["program"], row["caller"], row["callee"]))
    outputs = {
        "candidates.csv": _csv_text(
            [
                "program",
                "address",
                "sources",
                "aligned",
                "after_boundary",
                "prologue",
                "verdict",
                "size",
                "caller_count",
                "callee_count",
            ],
            candidate_rows,
        ),
        "calls.csv": _csv_text(["program", "caller", "callee", "call_sites"], call_rows),
    }

    report_dir = settings.build_dir / "reports" / _SNAPSHOT_NAME
    snapshot_dir = settings.repo_dir / "evidence" / "snapshots" / _SNAPSHOT_NAME
    for name, value in outputs.items():
        atomic_write(report_dir / name, value)
    if update_snapshot:
        for name, value in outputs.items():
            atomic_write(snapshot_dir / name, value)
        atomic_write(snapshot_dir / "README.md", _snapshot_readme())
    snapshot_fresh = all(
        (snapshot_dir / name).is_file()
        and (snapshot_dir / name).read_text(encoding="utf-8") == outputs[name]
        for name in _REPORT_FILES
    )
    if not update_snapshot and not snapshot_fresh:
        raise RuntimeError(
            "function-census report differs from the tracked snapshot; review "
            f"build/reports/{_SNAPSHOT_NAME} and rerun with --update-snapshot"
        )

    accepted_rows = [row for row in candidate_rows if row["verdict"] in {"exact", "strong"}]
    return {
        "schema": "wiz8.function-census",
        "programs": per_program,
        "byte_identical_aliases": aliases,
        "programs_without_readable_code": skipped,
        "candidates": len(candidate_rows),
        "accepted": len(accepted_rows),
        "by_verdict": {
            verdict: sum(1 for row in candidate_rows if row["verdict"] == verdict)
            for verdict in sorted({row["verdict"] for row in candidate_rows})
        },
        "attested_by_multiple_sources": sum(
            1 for row in accepted_rows if len(row["sources"].split()) > 1
        ),
        "call_edges": len(call_rows),
        "report": str(report_dir.relative_to(settings.repo_dir)),
        "snapshot": str(snapshot_dir.relative_to(settings.repo_dir)),
        "snapshot_fresh": snapshot_fresh,
        "snapshot_updated": update_snapshot,
    }
