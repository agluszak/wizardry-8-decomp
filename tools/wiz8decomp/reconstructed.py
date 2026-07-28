"""Carry the reconstructed build's own debug information back to the original.

Every byte-exact body is a two-sided fact: the reviewed row says *this original
address holds this function*, and the build says *this function has this
signature, these frame variables, and came from this object file*. Joining them
gives the original program type information that no amount of decompiling it
would recover, and it costs nothing to produce - the compiler already wrote it.

Three rules keep this from becoming laundering:

* **Types and structure only, never naming authority.** The reviewed model
  already decides what a function is called; the source spelling that reaches a
  transfer is a join key, not a proposed rename. Nothing here emits a name.
* **The extent must be able to hold the body.** A signature is a claim about a
  specific body, so a compiled body shorter than the reviewed one is a
  different function and is refused. A longer one is ordinary - a COMDAT
  carries the switch tables and padding that follow the code.
* **`exact` and everything else are different claims.** An exact row's bytes
  are the original's bytes, so its signature describes the original function;
  a structurally-strong row's do not, so its signature is a hypothesis and goes
  to the candidate overlay only. The tier travels with every record.

The reviewed program is not written here. Exact transfers are emitted as
proposed `signatures.csv` rows for review, because the reviewed Ghidra database
is rebuilt from the tracked ledger and a fact that skips the ledger does not
survive the next rebuild.
"""

from __future__ import annotations

import csv
import json
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .reconstructed_pdb import Procedure, ProgramDatabase, Signature, TypeStream, load, load_object

# `llvm-undname` and the reviewed rows spell compiler-generated members with
# underscores; VC6's debug information quotes them.
GENERATED_MEMBERS = {
    "`scalar deleting destructor'": "scalar_deleting_destructor",
    "`vector deleting destructor'": "vector_deleting_destructor",
}
_TEMPLATE_SPACING = re.compile(r"\s+(?=[*&>])")

REVIEWED_TIER = "reviewed-program"
OVERLAY_TIER = "candidate-overlay"


@dataclass(frozen=True)
class Body:
    """One compiled body, whichever debug source it was read from."""

    name: str
    length: int
    object_file: str
    signature: Signature | None
    frame_variables: tuple[tuple[str, int, str], ...]

    def shape(self) -> tuple[Any, ...]:
        signature = self.signature
        return (
            self.length,
            signature.spelling(self.name) if signature else "",
            self.frame_variables,
        )


@dataclass(frozen=True)
class Transfer:
    """One reviewed body paired with what the build knows about it."""

    address: str
    symbol: str
    confidence: str
    tier: str
    object_file: str
    signature: Signature | None
    frame_variables: tuple[tuple[str, int, str], ...] = ()
    blocked: str = ""

    def row(self) -> dict[str, Any]:
        signature = self.signature
        return {
            "address": self.address,
            "symbol": self.symbol,
            "confidence": self.confidence,
            "tier": self.tier,
            "object_file": self.object_file,
            "calling_convention": signature.convention if signature else "",
            "return_type": signature.return_type if signature else "",
            "this_type": f"{signature.owner} *" if signature and signature.owner else "",
            "parameters": "; ".join(signature.parameters) if signature else "",
            "frame_variables": len(self.frame_variables),
            "blocked": self.blocked,
        }


@dataclass
class TransferPlan:
    """What the build can say about the reviewed boundary rows, and what not."""

    transfers: list[Transfer] = field(default_factory=list)
    unmatched: list[dict[str, str]] = field(default_factory=list)
    verified_exact: set[str] = field(default_factory=set)

    def summary(self) -> dict[str, Any]:
        blocked = [item for item in self.transfers if item.blocked]
        return {
            "reviewed_rows": len(self.transfers) + len(self.unmatched),
            "transferable": len([item for item in self.transfers if not item.blocked]),
            "reviewed_tier": len([item for item in self.transfers if item.tier == REVIEWED_TIER]),
            "overlay_tier": len([item for item in self.transfers if item.tier == OVERLAY_TIER]),
            # The reasons themselves are per-row and live in transfers.csv;
            # summarising them here produced one line per byte-count pair.
            "blocked": len(blocked),
            "unmatched": len(self.unmatched),
            "object_files": len({item.object_file for item in self.transfers}),
            "freshly_verified_exact": len(self.verified_exact),
        }


def reviewed_spelling(name: str) -> str:
    """The PDB's source spelling reduced to the spelling a reviewed row uses."""

    for quoted, plain in GENERATED_MEMBERS.items():
        name = name.replace(quoted, plain)
    return _TEMPLATE_SPACING.sub("", name)


def _boundary_rows(repo: Path) -> list[dict[str, str]]:
    path = repo / "config" / "reccmp" / "wiz8-gameplay-boundaries.csv"
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def _body(procedure: Procedure, types: TypeStream, object_file: str) -> Body:
    return Body(
        name=reviewed_spelling(procedure.name),
        length=procedure.length,
        object_file=object_file,
        signature=types.signature(procedure.type_index),
        frame_variables=tuple(
            (variable.name, variable.frame_offset, types.name(variable.type_index))
            for variable in procedure.frame_variables
        ),
    )


def bodies_from_pdb(pdb: Path) -> list[Body]:
    """The linked executable's view: what the linker actually took."""

    database: ProgramDatabase = load(pdb)
    return [_body(procedure, database.types, procedure.module) for procedure in database.procedures]


def bodies_from_objects(build_dir: Path) -> list[Body]:
    """The compiled view: every `/Z7` object, linked or not.

    This build compiles more than it links - the SGP units are object libraries
    the executable selects from - so the linked PDB alone would report a
    recovered function as absent simply because this executable had no use for
    it. Each object resolves its own type indices.
    """

    bodies: list[Body] = []
    for path in sorted(build_dir.rglob("*.obj")):
        info = load_object(path, name=str(path.relative_to(build_dir)))
        bodies.extend(
            _body(procedure, info.types, info.object_file) for procedure in info.procedures
        )
    return bodies


def index_bodies(bodies: list[Body]) -> tuple[dict[str, Body], dict[str, list[str]]]:
    """Bodies by name, with genuinely conflicting names held back.

    A COMDAT template instantiation appears in every object that used it, and
    those copies are the same body - the linker keeps one. Two *different*
    bodies under one name are a real ambiguity: the reviewed row cannot say
    which it means, so neither can this.
    """

    grouped: dict[str, list[Body]] = {}
    for body in bodies:
        grouped.setdefault(body.name, []).append(body)
    unique: dict[str, Body] = {}
    ambiguous: dict[str, list[str]] = {}
    for name, group in grouped.items():
        if len({body.shape() for body in group}) == 1:
            unique[name] = group[0]
        else:
            ambiguous[name] = sorted(body.object_file for body in group)
    return unique, ambiguous


def build_transfer_plan(
    repo: Path, bodies: list[Body], verified_exact: set[str] | None = None
) -> TransferPlan:
    """Join every reviewed boundary row against the build's debug information."""

    compiled, ambiguous = index_bodies(bodies)
    verified_exact = {address.lower().zfill(8) for address in (verified_exact or set())}
    plan = TransferPlan(verified_exact=verified_exact)
    for row in _boundary_rows(repo):
        symbol = row["symbol"].strip()
        body = compiled.get(symbol)
        if body is None:
            plan.unmatched.append(
                {
                    "address": row["address"],
                    "symbol": symbol,
                    "confidence": row["confidence"],
                    "reason": (
                        "compiled into several objects with different bodies: "
                        + ", ".join(ambiguous[symbol])
                        if symbol in ambiguous
                        else "no body of this name in the build"
                    ),
                }
            )
            continue
        confidence = row["confidence"].strip()
        reviewed_size = int(row["size"])
        blocked = ""
        if body.length < reviewed_size:
            # A COMDAT carries the body's switch tables and alignment padding,
            # so a longer compiled extent is ordinary; a shorter one cannot
            # hold the reviewed body at all and is a different function.
            blocked = (
                f"build body is {body.length} bytes against the reviewed "
                f"{reviewed_size}; too short to be this body"
            )
        elif confidence == "exact" and row["address"].lower().zfill(8) not in verified_exact:
            blocked = "fresh relocation-masked boundary verification required"
        tier = REVIEWED_TIER if confidence == "exact" and not blocked else OVERLAY_TIER
        plan.transfers.append(
            Transfer(
                address=row["address"],
                symbol=symbol,
                confidence=confidence,
                tier=tier,
                object_file=body.object_file,
                signature=body.signature,
                frame_variables=body.frame_variables,
                blocked=blocked,
            )
        )
    return plan


def verified_boundary_addresses(repo: Path, object_root: Path) -> set[str]:
    """Recompute every exact row's recorded masked digest from current objects."""

    from .boundaries import collect_object_candidates, masked_digest, resolve_boundary_function

    rows = _boundary_rows(repo)
    candidates = collect_object_candidates(object_root)
    verified: set[str] = set()
    for row in rows:
        if row["confidence"].strip() != "exact" or not row["relocation_masked_sha256"].strip():
            continue
        size = int(row["size"])
        function = resolve_boundary_function(candidates.get(row["symbol"].strip(), ()), size, None)
        if function is None:
            continue
        if masked_digest(function, size) == row["relocation_masked_sha256"].strip():
            verified.add(row["address"].lower().zfill(8))
    return verified


def incoming_variables(
    frame_variables: tuple[tuple[str, int, str], ...],
) -> list[tuple[str, int, str]]:
    """The frame variables that are parameters, lowest offset first.

    Locals sit below the frame origin and parameters above it, whichever origin
    the body uses, so the sign separates them without knowing which it is.
    """

    return sorted(
        (variable for variable in frame_variables if variable[1] > 0),
        key=lambda item: item[1],
    )


def frame_origin(frame_variables: tuple[tuple[str, int, str], ...]) -> int | None:
    """Where this body's frame offsets are measured from, or None if unknown.

    VC6 records two origins and does not say which it used. A body that keeps
    its frame pointer puts the first parameter at `ebp+8`; `/O2` omits the
    frame pointer and the offsets become return-address relative, so the first
    parameter is at 4 - the same origin Ghidra measures from. The first
    parameter's offset is therefore the origin, and a body with no parameters
    gives no evidence at all, which is reported rather than assumed.
    """

    incoming = incoming_variables(frame_variables)
    return incoming[0][1] if incoming else None


def parameter_names(transfer: Transfer) -> tuple[str, ...]:
    """The source's own parameter names, when the frame proves the pairing.

    The names are only used when the frame holds exactly as many incoming
    variables as the type stream says there are parameters; a different count
    means the frame is not describing this parameter list, and positional
    `argumentN` is the honest fallback.
    """

    parameters = transfer.signature.parameters if transfer.signature else ()
    candidates = [name for name, _offset, _type in incoming_variables(transfer.frame_variables)]
    if len(candidates) == len(parameters):
        return tuple(candidates)
    return tuple(f"argument{index}" for index in range(1, len(parameters) + 1))


def proposed_signature_rows(plan: TransferPlan, program: str = "wiz8") -> list[dict[str, str]]:
    """Reviewed-tier transfers as `signatures.csv` rows, for a human to accept.

    The rows are complete but deliberately not written into the ledger: an
    exact body proves the *bytes* match, and a reviewer still has to agree that
    our parameter names and spellings describe the original's intent rather than
    ours. `previous_auto_signature` stays empty because only Ghidra can fill it.
    """

    rows = []
    for transfer in plan.transfers:
        signature = transfer.signature
        if transfer.tier != REVIEWED_TIER or signature is None or transfer.blocked:
            continue
        parameters = [
            [name, parameter]
            for name, parameter in zip(parameter_names(transfer), signature.parameters, strict=True)
        ]
        rows.append(
            {
                "program": program,
                "address": transfer.address,
                "calling_convention": signature.convention,
                "return_type": signature.return_type,
                "parameters_json": json.dumps(parameters),
                "variadic": "false",
                "this_type": f"{signature.owner} *" if signature.owner else "",
                # Exact bytes prove convention and stack shape, not every
                # semantic spelling reconstructed source chose.
                "confidence": "strong",
                "calling_convention_authority": "exact-body",
                "stack_argument_shape_authority": "exact-body",
                "return_type_authority": "candidate-reconstructed-source",
                "parameter_type_authority": "candidate-reconstructed-source",
                "parameter_name_authority": "reconstructed-source-not-original-name-evidence",
                "evidence_id": f"signatures:{program}:{transfer.address}",
                "previous_auto_signature": "",
                "evidence": (
                    f"The reconstructed build's debug information for {transfer.symbol}, "
                    f"compiled from {transfer.object_file}, whose body is byte-exact "
                    "against this address under relocation masking"
                ),
            }
        )
    return rows


def write_report(plan: TransferPlan, destination: Path, program: str = "wiz8") -> dict[str, Any]:
    """Emit the plan under `build/` as CSV rows plus a summary."""

    destination.mkdir(parents=True, exist_ok=True)
    transfers = destination / "transfers.csv"
    with transfers.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(
            stream,
            fieldnames=[
                "address",
                "symbol",
                "confidence",
                "tier",
                "object_file",
                "calling_convention",
                "return_type",
                "this_type",
                "parameters",
                "frame_variables",
                "blocked",
            ],
        )
        writer.writeheader()
        for transfer in sorted(plan.transfers, key=lambda item: item.address):
            writer.writerow(transfer.row())

    proposals = proposed_signature_rows(plan, program)
    signatures = destination / "proposed-signatures.csv"
    with signatures.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(proposals[0]) if proposals else ["program"])
        writer.writeheader()
        writer.writerows(proposals)

    unmatched = destination / "unmatched.csv"
    with unmatched.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=["address", "symbol", "confidence", "reason"])
        writer.writeheader()
        writer.writerows(plan.unmatched)

    summary = plan.summary()
    summary["proposed_signatures"] = len(proposals)
    (destination / "summary.json").write_text(
        json.dumps(summary, indent=2) + "\n", encoding="utf-8"
    )
    return summary
