"""Shared source facts and conservative, report-only integer recovery.

Current AST types describe the reconstruction. Only separately reviewed evidence
seeds a recovery property; source arithmetic is a use to account for, not an oracle.
"""

from __future__ import annotations

import difflib
import hashlib
import json
import re
from collections import defaultdict, deque
from dataclasses import dataclass, field, replace
from pathlib import Path


@dataclass(frozen=True)
class DeclarationFact:
    key: str
    file: str
    line: int
    column: int
    kind: str
    name: str
    name_bool: bool
    byte_candidate: bool
    width: int
    signedness: str
    domain: str
    spelling: str


@dataclass(frozen=True, order=True)
class Use:
    key: str
    detail: str
    file: str
    line: int
    column: int


@dataclass(frozen=True, order=True)
class Flow:
    target: str
    source: str
    role: str
    file: str
    line: int
    column: int


@dataclass
class ScalarFacts:
    arrays: set[tuple] = field(default_factory=set)
    array_uses: set[Use] = field(default_factory=set)
    records: set[tuple] = field(default_factory=set)
    record_fields: set[tuple] = field(default_factory=set)
    translation_units: set[str] = field(default_factory=set)
    declarations: dict[str, DeclarationFact] = field(default_factory=dict)
    flows: set[Flow] = field(default_factory=set)
    spans: set[tuple[str, str, int, int, str, str]] = field(default_factory=set)
    types: dict[str, tuple[str, str, str]] = field(default_factory=dict)
    operands: set[tuple[str, str, int, str, int, int]] = field(default_factory=set)
    conversions: set[tuple[str, str, str, str]] = field(default_factory=set)
    callback_slots: dict[str, tuple[int, int, bool]] = field(default_factory=dict)
    callback_bindings: set[tuple[str, str, int, int, bool]] = field(default_factory=set)
    constants: dict[str, set[int]] = field(default_factory=lambda: defaultdict(set))
    operations: set[Use] = field(default_factory=set)
    escapes: set[Use] = field(default_factory=set)
    bodies: set[str] = field(default_factory=set)
    source_domains: dict[str, set[str]] = field(default_factory=lambda: defaultdict(set))
    inconsistent: set[str] = field(default_factory=set)
    locations: list[tuple[str, str, int]] = field(default_factory=list)
    # Boolean-domain annotations are one solver's input, not generic type facts.
    bool_writes: dict[str, list[tuple[str, tuple[str, ...]]]] = field(
        default_factory=lambda: defaultdict(list)
    )
    bool_invalid: set[str] = field(default_factory=set)
    bool_escaped: set[str] = field(default_factory=set)
    bool_supported: set[str] = field(default_factory=set)
    bool_pending: set[tuple[str, str]] = field(default_factory=set)
    bool_bodies: set[str] = field(default_factory=set)


def decode_field(value: str) -> str:
    if "\\" not in value:
        return value
    escapes = {"t": "\t", "n": "\n", "r": "\r", "\\": "\\"}
    result = []
    index = 0
    while index < len(value):
        if value[index] == "\\":
            index += 1
            if index >= len(value) or value[index] not in escapes:
                raise ValueError("invalid fact field escape")
            result.append(escapes[value[index]])
        else:
            result.append(value[index])
        index += 1
    return "".join(result)


def read_scalar_facts(directory: Path) -> ScalarFacts:
    facts = ScalarFacts()
    seen: set[str] = set()
    for path in sorted(directory.glob("facts-*.tsv")):
        with path.open(encoding="utf-8") as stream:
            for number, raw in enumerate(stream, 1):
                raw = raw.removesuffix("\n")
                if raw in seen:
                    continue
                seen.add(raw)
                if not raw:
                    continue
                try:
                    parts = [decode_field(part) for part in raw.split("\t")]
                    tag = parts[0]
                    if tag == "M":
                        if len(parts) != 2:
                            raise ValueError("translation unit requires 2 fields")
                        facts.translation_units.add(parts[1])
                    elif tag == "ARR":
                        if len(parts) != 10:
                            raise ValueError("array observation requires 10 fields")
                        facts.arrays.add(tuple(parts[1:]))
                    elif tag == "AU":
                        if len(parts) != 6:
                            raise ValueError("array use requires 6 fields")
                        facts.array_uses.add(
                            Use(parts[1], parts[2], parts[3], int(parts[4]), int(parts[5]))
                        )
                    elif tag in {"REC", "RF"}:
                        if len(parts) != (8 if tag == "REC" else 6):
                            raise ValueError("invalid record observation")
                        (facts.records if tag == "REC" else facts.record_fields).add(
                            tuple(parts[1:])
                        )
                    elif tag == "D":
                        if len(parts) != 13:
                            raise ValueError(
                                f"declaration requires 13 fields, got {len(parts)}: {raw!r}"
                            )
                        declaration = DeclarationFact(
                            parts[1],
                            parts[2],
                            int(parts[3]),
                            int(parts[4]),
                            parts[5],
                            parts[6],
                            parts[7] == "1",
                            parts[8] == "1",
                            int(parts[9]),
                            parts[10],
                            parts[11],
                            parts[12],
                        )
                        previous = facts.declarations.get(declaration.key)
                        if previous is not None:
                            # Generic collection and the bool client may provide the
                            # same declaration with different presentation flags.
                            if replace(previous, name_bool=False, name="") != replace(
                                declaration, name_bool=False, name=""
                            ):
                                facts.inconsistent.add(declaration.key)
                            declaration = replace(
                                previous, name_bool=previous.name_bool or declaration.name_bool
                            )
                        facts.declarations[declaration.key] = declaration
                        facts.locations.append(
                            (declaration.key, declaration.file, declaration.line)
                        )
                    elif tag == "L":
                        if len(parts) != 7:
                            raise ValueError("source span requires 7 fields")
                        facts.spans.add(
                            (parts[1], parts[2], int(parts[3]), int(parts[4]), parts[5], parts[6])
                        )
                    elif tag == "T":
                        if len(parts) != 5:
                            raise ValueError("type observation requires 5 fields")
                        metadata = tuple(parts[2:])
                        if parts[1] in facts.types and facts.types[parts[1]] != metadata:
                            facts.inconsistent.add(parts[1])
                        facts.types[parts[1]] = metadata
                    elif tag == "O":
                        if len(parts) != 7:
                            raise ValueError("operand observation requires 7 fields")
                        facts.operands.add(
                            (
                                parts[1],
                                parts[2],
                                int(parts[3]),
                                parts[4],
                                int(parts[5]),
                                int(parts[6]),
                            )
                        )
                    elif tag == "V":
                        if len(parts) != 5:
                            raise ValueError("conversion observation requires 5 fields")
                        facts.conversions.add(tuple(parts[1:]))
                    elif tag == "J":
                        if len(parts) != 5:
                            raise ValueError("callback slot requires 5 fields")
                        signature = (int(parts[2]), int(parts[3]), parts[4] == "1")
                        if (
                            parts[1] in facts.callback_slots
                            and facts.callback_slots[parts[1]] != signature
                        ):
                            facts.inconsistent.add(parts[1])
                        facts.callback_slots[parts[1]] = signature
                    elif tag == "C":
                        if len(parts) != 6:
                            raise ValueError("callback binding requires 6 fields")
                        facts.callback_bindings.add(
                            (parts[1], parts[2], int(parts[3]), int(parts[4]), parts[5] == "1")
                        )
                    elif tag == "F":
                        flow = Flow(
                            parts[1], parts[2], parts[3], parts[4], int(parts[5]), int(parts[6])
                        )
                        facts.flows.add(flow)
                    elif tag == "K":
                        facts.constants[parts[1]].add(int(parts[2]))
                    elif tag in {"U", "A", "H", "G"}:
                        use = Use(parts[1], parts[2], parts[3], int(parts[4]), int(parts[5]))
                        if tag == "U":
                            facts.operations.add(use)
                        elif tag == "A":
                            facts.escapes.add(use)
                        elif tag == "H":
                            facts.bodies.add(use.key)
                        else:
                            facts.source_domains[use.key].add(use.detail)
                    elif tag == "P":
                        facts.bool_pending.add((parts[1], parts[5]))
                        facts.locations.append((parts[1], parts[2], int(parts[3])))
                    elif tag == "B":
                        facts.bool_bodies.add(parts[1])
                    elif tag == "W":
                        dependencies = tuple(filter(None, parts[3].split(",")))
                        write = (parts[2], dependencies)
                        if write not in facts.bool_writes[parts[1]]:
                            facts.bool_writes[parts[1]].append(write)
                        facts.locations.append((parts[1], parts[4], int(parts[5])))
                        facts.locations.extend(
                            (key, parts[4], int(parts[5])) for key in dependencies
                        )
                    elif tag in {"X", "E", "S"}:
                        bucket = {
                            "X": facts.bool_invalid,
                            "E": facts.bool_escaped,
                            "S": facts.bool_supported,
                        }[tag]
                        bucket.add(parts[1])
                        facts.locations.append((parts[1], parts[2], int(parts[3])))
                    else:
                        raise ValueError(f"unknown fact tag {tag!r}")
                except (ValueError, IndexError) as error:
                    raise ValueError(f"{path}:{number}: {error}") from error
    facts.bool_escaped.update(
        key for key, callee in facts.bool_pending if callee not in facts.bool_bodies
    )
    return facts


_SIGNED = {"jl", "jle", "jg", "jge", "idiv", "movsx"}
_UNSIGNED = {"jb", "jbe", "ja", "jae", "jc", "jnc", "div", "movzx"}
_BASES = {"retail", "external-api", "decorated-export", "source-oracle"}


def semantic_domain(declaration: DeclarationFact) -> str:
    if declaration.domain == "enum":
        return "enum:" + declaration.spelling.removeprefix("enum ")
    return declaration.domain


def abi_width_inventory(facts: ScalarFacts) -> list[dict]:
    """Classify current declaration spellings, without seeding retail recovery.

    Saved facts retain typedef spelling, so aliases not named here require a
    separate alias review. An enum fact does not say whether its declaration
    has an explicit underlying type. Pointer/member-pointer ABI is target
    dependent even if the pointed-to integer has an invariant width.
    """
    rows = []
    for key, declaration in sorted(facts.declarations.items()):
        spelling = declaration.spelling
        hazards = []
        if re.search(r"\blong\b", spelling) and not re.search(r"\blong\s+long\b", spelling):
            hazards.append("long_ilp32_llp64_32_lp64_64")
        if re.search(r"\b(?:size_t|ptrdiff_t|intptr_t|uintptr_t)\b", spelling):
            hazards.append("pointer_sized_integer")
        if re.search(r"\bwchar_t\b", spelling):
            hazards.append("vc6_wide_code_unit_16")
        if declaration.domain == "enum":
            hazards.append("enum_representation_review")
        if declaration.domain == "pointer":
            hazards.append("pointer_representation")
        if not hazards:
            continue
        rows.append(
            {
                "key": key,
                "file": declaration.file,
                "line": declaration.line,
                "kind": declaration.kind,
                "spelling": spelling,
                "current_ast_width": declaration.width,
                "hazards": hazards,
                "status": "inconsistent" if key in facts.inconsistent else "requires_review",
            }
        )
    return rows


def pointer_integer_transports(facts: ScalarFacts) -> list[dict]:
    """AST-attributed transport edges, including typedef-backed integer slots."""
    rows = []
    for flow in sorted(facts.flows):
        source = facts.declarations.get(flow.source)
        target = facts.declarations.get(flow.target)
        if source is None or target is None:
            continue
        if {source.domain, target.domain} != {"pointer", "integer"}:
            continue
        rows.append(
            {
                **vars(flow),
                "source_type": source.spelling,
                "target_type": target.spelling,
                "source_width": source.width,
                "target_width": target.width,
                "status": "requires_abi_boundary_review",
            }
        )
    return rows


def read_evidence(path: Path | None, facts: ScalarFacts) -> list[dict]:
    if path is None:
        return []
    payload = json.loads(path.read_text(encoding="utf-8"))
    if payload.get("schema") != "wiz8.scalar-evidence-v1":
        raise ValueError("unsupported scalar evidence schema")
    claims = payload["claims"]
    for claim in claims:
        selector = claim.get("selector")
        if selector is not None:
            if "key" in claim or set(selector) != {"file", "kind", "name"}:
                raise ValueError("selector requires exactly file, kind and name, without key")
            matches = [
                d.key
                for d in facts.declarations.values()
                if (
                    d.file,
                    d.kind,
                    d.key.split(":parameter:", 1)[-1] if d.kind == "parameter" else d.name,
                )
                == (selector["file"], selector["kind"], selector["name"])
            ]
            if len(matches) != 1:
                raise ValueError(
                    f"evidence selector requires one declaration, found {len(matches)}"
                )
            claim["key"] = matches[0]
    widths = {(claim["key"], claim["value"]) for claim in claims if claim["property"] == "width"}
    for claim in claims:
        if claim["key"] not in facts.declarations:
            raise ValueError(f"evidence refers to unknown declaration {claim['key']!r}")
        basis = claim["basis"]
        if basis["kind"] not in _BASES or not basis.get("reference") or not basis.get("reason"):
            raise ValueError("evidence requires independent provenance, a reference and a reason")
        property_ = claim["property"]
        value = claim["value"]
        if property_ == "width":
            if value not in {8, 16, 32}:
                raise ValueError("integer width must be 8, 16 or 32")
            if facts.declarations[claim["key"]].kind in {
                "function",
                "callback-return",
            } and not claim.get("complete_return_boundary"):
                raise ValueError(
                    "return width requires both producer and complete caller-consumption evidence"
                )
            if facts.declarations[claim["key"]].kind in {
                "parameter",
                "callback-parameter",
            } and not claim.get("complete_argument_boundary"):
                raise ValueError("parameter width requires complete argument/callee evidence")
            if basis["kind"] == "retail":
                if not basis.get("value_width") == value:
                    raise ValueError(
                        "width evidence must identify the observed storage/value width"
                    )
                if facts.declarations[claim["key"]].kind == "field" and not claim.get(
                    "complete_storage_accesses"
                ):
                    raise ValueError(
                        "field width requires complete receiver-attributed storage accesses"
                    )
        elif property_ == "signedness":
            if value not in {"signed", "unsigned", "irrelevant"}:
                raise ValueError("invalid signedness evidence")
            if basis["kind"] == "retail":
                mnemonic = basis.get("mnemonic", "").lower()
                expected = (
                    "signed"
                    if mnemonic in _SIGNED
                    else "unsigned"
                    if mnemonic in _UNSIGNED
                    else None
                )
                if mnemonic in {"sar", "shr"} and basis.get("high_bits_relevant"):
                    expected = "signed" if mnemonic == "sar" else "unsigned"
                if expected != value:
                    raise ValueError("instruction does not establish this signedness property")
                # A comparison of a promoted byte is not signed-byte evidence.
                if basis.get("value_width") != basis.get("operand_width"):
                    raise ValueError(
                        "promotion/operand-width mismatch requires additional reviewed evidence"
                    )
                width = basis.get("value_width")
                if width not in {8, 16, 32}:
                    raise ValueError("retail signedness requires an observed value width")
                if (
                    width != facts.declarations[claim["key"]].width
                    and (claim["key"], width) not in widths
                ):
                    raise ValueError("storage-width mismatch requires independent width evidence")
                if mnemonic in {"movsx", "movzx"} and (
                    basis.get("operand_role") != "input" or width not in {8, 16}
                ):
                    raise ValueError("extension evidence must identify the narrow input operand")
        elif property_ == "pointee":
            if (
                not isinstance(value, str)
                or value in {"", "void"}
                or value not in {metadata[2] for metadata in facts.types.values() if metadata[2]}
            ):
                raise ValueError("pointee recovery requires an existing concrete pointee identity")
            if basis["kind"] == "retail":
                raise ValueError("same pointer ABI does not establish an authored pointee type")
        elif property_ == "nominal":
            if not isinstance(value, str) or value not in {
                metadata[1] for metadata in facts.types.values() if metadata[1]
            }:
                raise ValueError("nominal recovery requires an existing typedef identity")
            if basis["kind"] == "retail":
                raise ValueError("typedef spelling requires independent owner evidence")
            if claim.get("role") not in {
                "ID",
                "index",
                "count",
                "handle",
                "timer",
                "status",
                "flags",
            }:
                raise ValueError("nominal recovery requires a reviewed semantic role")
        elif property_ == "domain":
            if not isinstance(value, str) or not value.startswith("enum:"):
                raise ValueError("domain recovery requires an existing enum identity")
            if value not in {
                semantic_domain(declaration)
                for declaration in facts.declarations.values()
                if declaration.domain == "enum"
            }:
                raise ValueError("cannot invent an enum from numeric ranges")
            if basis["kind"] == "retail":
                raise ValueError(
                    "enum identity requires independent symbol, header or source evidence"
                )
        else:
            raise ValueError("unsupported scalar property")
    return claims


def flow_components(facts: ScalarFacts) -> list[set[str]]:
    """One copy/argument/return graph shared by all domain clients."""
    neighbors: dict[str, set[str]] = defaultdict(set)
    for flow in facts.flows:
        neighbors[flow.source].add(flow.target)
        neighbors[flow.target].add(flow.source)
    visited: set[str] = set()
    result = []
    for root in sorted(set(facts.declarations) | set(neighbors)):
        if root in visited:
            continue
        members, pending = set(), [root]
        while pending:
            key = pending.pop()
            if key in members:
                continue
            members.add(key)
            pending.extend(neighbors[key] - members)
        visited.update(members)
        result.append(members)
    return result


def sentinel_values(facts: ScalarFacts, key: str) -> list[int]:
    declaration = facts.declarations[key]
    values = set(facts.constants[key])
    values.update(
        value
        for owner, operation, value, *_ in facts.operands
        if owner == key
        and operation.removeprefix("rhs:") in {"==", "!=", "<", "<=", ">", ">=", "case"}
    )
    return sorted(
        value
        for value in values
        if value < 0 or (declaration.width and value == (1 << declaration.width) - 1)
    )


def sentinel_index(facts: ScalarFacts) -> dict[str, list[int]]:
    consumed: dict[str, set[int]] = defaultdict(set)
    for key, operation, value, *_ in facts.operands:
        if operation.removeprefix("rhs:") in {"==", "!=", "<", "<=", ">", ">=", "case"}:
            consumed[key].add(value)
    return {
        key: sorted(
            value
            for value in facts.constants[key] | consumed[key]
            if value < 0 or (declaration.width and value == (1 << declaration.width) - 1)
        )
        for key, declaration in facts.declarations.items()
    }


def domain_inventory(facts: ScalarFacts) -> list[dict]:
    """Classify observed behavior separately from historical source types.

    Finite values flow directionally; an unknown producer, escape or unseeded
    cycle prevents a complete-domain claim. Mask operations never prove enum
    ownership, and numeric ranges never create a named domain.
    """
    sentinels = sentinel_index(facts)
    values = {key: set(facts.constants[key]) for key in facts.declarations}
    for key, produced in values.items():
        if facts.source_domains[key] == {"bool"}:
            produced.update({0, 1})
    incoming: dict[str, set[str]] = defaultdict(set)
    for flow in facts.flows:
        incoming[flow.target].add(flow.source)
    outgoing: dict[str, set[str]] = defaultdict(set)
    for target, sources in incoming.items():
        for source in sources:
            outgoing[source].add(target)
    # Finite union lattice bounded by observed constants; visit affected edges.
    pending = deque(key for key, produced in values.items() if produced)
    while pending:
        source = pending.popleft()
        for target in outgoing[source]:
            if target in values and not values[source] <= values[target]:
                values[target].update(values[source])
                pending.append(target)
    escaped = {use.key for use in facts.escapes}
    escaped.update(use.key for use in facts.operations if use.detail in {"++", "--"})
    unavailable = {
        key
        for key, declaration in facts.declarations.items()
        if declaration.kind in {"function", "parameter", "callback-return", "callback-parameter"}
        and key not in facts.bodies
    }
    complete = (
        {key for key in values if values[key] and not incoming[key]}
        - escaped
        - facts.inconsistent
        - unavailable
    )
    pending = deque(complete)
    while pending:
        source = pending.popleft()
        for target in outgoing[source] - complete - escaped - facts.inconsistent - unavailable:
            if incoming[target] <= complete:
                complete.add(target)
                pending.append(target)
    operand_index: dict[str, list[tuple]] = defaultdict(list)
    use_index: dict[str, list[Use]] = defaultdict(list)
    for item in sorted(facts.operands):
        operand_index[item[0]].append(item)
    for use in sorted(facts.operations):
        use_index[use.key].append(use)
    result = []
    for members in flow_components(facts):
        operands = [item for key in sorted(members) for item in operand_index[key]]
        uses = [use for key in sorted(members) for use in use_index[key]]
        masks = [
            item
            for item in operands
            if item[1].removeprefix("rhs:") in {"&", "|", "^", "&=", "|=", "^="} and item[2] > 0
        ]
        exclusive = [
            item for item in operands if item[1].removeprefix("rhs:") in {"==", "!=", "case"}
        ]
        arithmetic = any(
            use.detail in {"+", "-", "*", "/", "%", "++", "--", "index"} for use in uses
        )
        behavior = (
            "mixed"
            if masks and (exclusive or arithmetic)
            else "flags-like"
            if masks and not arithmetic
            else "exclusive-values"
            if exclusive and not arithmetic
            else "scalar"
        )
        finite = sorted(set().union(*(values.get(key, set()) for key in members)))
        domain = (
            "boolean-values"
            if finite and set(finite) <= {0, 1}
            else "tri-state-values"
            if set(finite) == {-1, 0, 1}
            else "sentinel-bearing"
            if any(sentinels[key] for key in members if key in facts.declarations)
            else "finite-integer-values"
            if finite
            else "unknown"
        )
        result.append(
            {
                "members": sorted(members),
                "behavior": behavior,
                "observed_values": finite,
                "value_domain": domain,
                "complete_value_domain": members <= complete,
                "sentinels": [
                    {"key": key, "values": sentinels[key]}
                    for key in sorted(members)
                    if key in facts.declarations and sentinels[key]
                ],
                "mask_operands": [
                    {
                        "key": key,
                        "operation": operation,
                        "value": value,
                        "file": file,
                        "line": line,
                        "column": column,
                    }
                    for key, operation, value, file, line, column in masks
                ],
                "source_type_recovered": False,
            }
        )
    return result


def nominal_representation(declaration: DeclarationFact, metadata: tuple) -> tuple:
    # Ordinary integer spellings are not independent properties. A proven
    # existing typedef can own unsigned int and unsigned long representations
    # alike on Win32; named enum/pointer identities still need to agree.
    identity = "" if declaration.domain == "integer" else metadata[0]
    return declaration.width, declaration.signedness, declaration.domain, identity


def anchored_report(facts: ScalarFacts, claims: list[dict], property_: str) -> list[dict]:
    """Existing pointer/typedef identities require independent owner evidence."""
    sentinels = sentinel_index(facts)
    uses: dict[str, list[Use]] = defaultdict(list)
    escapes: dict[str, list[Use]] = defaultdict(list)
    conversions: dict[str, list[tuple]] = defaultdict(list)
    owner_types: dict[str, set[tuple]] = defaultdict(set)
    for use in sorted(facts.operations):
        uses[use.key].append(use)
    for use in sorted(facts.escapes):
        escapes[use.key].append(use)
    for conversion in sorted(facts.conversions):
        for key in set(conversion[:2]):
            conversions[key].append(conversion)
    for key, metadata in facts.types.items():
        declaration = facts.declarations.get(key)
        if metadata[1] and declaration is not None:
            owner_types[metadata[1]].add(nominal_representation(declaration, metadata))
    result = []
    for members in flow_components(facts):
        relevant = [
            claim for claim in claims if claim["key"] in members and claim["property"] == property_
        ]
        values = {claim["value"] for claim in relevant}
        if not relevant:
            result.append(
                {
                    "members": sorted(members),
                    "status": "unknown",
                    "value": None,
                    "changes": [],
                    "blockers": [],
                    "evidence": [],
                }
            )
            continue
        value = next(iter(values)) if len(values) == 1 else None
        blockers = []
        for key in sorted(members):
            declaration = facts.declarations.get(key)
            metadata = facts.types.get(key)
            if declaration is None or metadata is None:
                blockers.append({"key": key, "reason": "missing typed declaration"})
                continue
            if key in facts.inconsistent:
                blockers.append({"key": key, "reason": "inconsistent cross-TU declaration"})
            if (
                declaration.kind
                in {"function", "parameter", "callback-return", "callback-parameter"}
                and key not in facts.bodies
                and not any(
                    claim["key"] == key and claim["basis"]["kind"] != "retail" for claim in relevant
                )
            ):
                blockers.append({"key": key, "reason": "unreviewed signature boundary"})
            if property_ == "pointee":
                if declaration.domain != "pointer" or not metadata[2]:
                    blockers.append({"key": key, "reason": "non-object-pointer domain"})
                elif value is not None and metadata[2] not in {"void", value}:
                    blockers.append(
                        {
                            "key": key,
                            "reason": "conflicting pointee or qualifiers",
                            "pointee": metadata[2],
                        }
                    )
                if any(constant != 0 for constant in facts.constants[key]):
                    blockers.append({"key": key, "reason": "non-null numeric pointer producer"})
            elif value is not None:
                if nominal_representation(declaration, metadata) not in owner_types[value] or (
                    metadata[1] and metadata[1] != value
                ):
                    blockers.append(
                        {"key": key, "reason": "different representation or existing typedef owner"}
                    )
                if sentinels[key]:
                    blockers.append(
                        {"key": key, "reason": "sentinel requires nominal-domain review"}
                    )
            for use in escapes[key]:
                # A by-value T* argument cannot change its pointer storage.
                # Aggregate fields, address-taking, references and T** remain barriers.
                if (
                    property_ == "pointee"
                    and use.detail == "indirect storage argument"
                    and metadata[2]
                    and "*" not in metadata[2]
                ):
                    continue
                if property_ == "pointee" and use.detail == "explicit conversion":
                    local_conversions = conversions[key]
                    if (
                        value
                        and local_conversions
                        and all(
                            {source, target} <= {"void *", value + " *"}
                            for _, _, source, target in local_conversions
                        )
                    ):
                        continue
                blockers.append(
                    {"key": key, "reason": use.detail, "file": use.file, "line": use.line}
                )
            covered = {
                (claim["key"], use["file"], use["line"], use["operation"])
                for claim in relevant
                for use in claim.get("covered_uses", [])
            }
            for use in uses[key]:
                if (key, use.file, use.line, use.detail) not in covered:
                    blockers.append(
                        {
                            "key": key,
                            "reason": "unreviewed operation: " + use.detail,
                            "file": use.file,
                            "line": use.line,
                        }
                    )
        status = (
            "unknown"
            if not values
            else "conflict"
            if len(values) > 1
            else "blocked"
            if blockers
            else "candidate"
        )
        index = 2 if property_ == "pointee" else 1
        result.append(
            {
                "members": sorted(members),
                "status": status,
                "value": value,
                "changes": [key for key in sorted(members) if facts.types[key][index] != value]
                if status == "candidate"
                else [],
                "blockers": blockers,
                "evidence": relevant,
            }
        )
    return result


def callback_report(facts: ScalarFacts) -> list[dict]:
    result = []
    for slot, signature in sorted(facts.callback_slots.items()):
        bindings = [binding for binding in sorted(facts.callback_bindings) if binding[0] == slot]
        blockers = []
        if not bindings:
            blockers.append("no resolved implementation")
        if signature[2] or any(binding[4] for binding in bindings):
            blockers.append("variadic signature")
        if any(binding[2:] != signature for binding in bindings):
            blockers.append("calling convention or parameter count mismatch")
        if any(
            slot + "::callback-arg#" + str(index) not in facts.declarations
            for index in range(signature[0])
        ):
            blockers.append("unmodeled callback parameter")
        if slot in facts.inconsistent:
            blockers.append("inconsistent callback slot")
        if any(use.key == slot for use in facts.escapes):
            blockers.append("callback slot escapes or has an unmodeled producer")
        result.append(
            {
                "slot": slot,
                "arity": signature[0],
                "calling_convention": signature[1],
                "implementations": [binding[1] for binding in bindings],
                "nodes": [
                    key
                    for key in sorted(facts.declarations)
                    if key.startswith(slot + "::callback-")
                ],
                "status": "blocked" if blockers else "modeled",
                "blockers": blockers,
                "requires": "independent signature evidence and complete implementation/table/invocation coverage before edits",
            }
        )
    return result


def structural_report(facts: ScalarFacts) -> dict:
    """Source layout/use inventory; these observations never seed retail recovery."""
    uses = defaultdict(set)
    for use in facts.array_uses:
        uses[use.key].add(use.detail)
    arrays = []
    for key, file, line, column, name, spelling, extent, bits, element in sorted(facts.arrays):
        roles = sorted(uses[key])
        arrays.append(
            {
                "key": key,
                "file": file,
                "line": int(line),
                "name": name,
                "element_type": spelling,
                "extent": int(extent),
                "element_width": int(bits),
                "element_node": element,
                "uses": roles,
                "text_initializer": "string-initializer" in roles,
                "requires": "complete storage/access and independent extent evidence",
            }
        )
    layouts = defaultdict(list)
    fields = defaultdict(list)
    for key, name, offset, size, type_ in facts.record_fields:
        fields[key].append((int(offset), int(size), type_))
    records = []
    for key, file, line, name, size, align, natural in sorted(facts.records):
        shape = tuple(sorted(fields[key]))
        layouts[(int(size), shape)].append(key)
        records.append(
            {
                "key": key,
                "file": file,
                "line": int(line),
                "name": name,
                "size_bits": int(size),
                "alignment_bits": int(align),
                "natural_size_bits": None if natural == "unknown" else int(natural),
                "packing_changes_size": None if natural == "unknown" else int(natural) != int(size),
                "requires": "retail offsets/stride before changing packing or merging records",
            }
        )
    return {
        "arrays": arrays,
        "records": records,
        "duplicate_layouts": [
            keys for (size, shape), keys in sorted(layouts.items()) if shape and len(keys) > 1
        ],
    }


def integer_report(facts: ScalarFacts, claims: list[dict]) -> dict:
    """Report connected copy chains; never emit edits or infer int/long spelling."""
    sentinels_by_key = sentinel_index(facts)
    evidence: dict[str, list[dict]] = defaultdict(list)
    for claim in claims:
        evidence[claim["key"]].append(claim)
    operations: dict[str, list[Use]] = defaultdict(list)
    escapes: dict[str, list[Use]] = defaultdict(list)
    for use in facts.operations:
        operations[use.key].append(use)
    for use in facts.escapes:
        escapes[use.key].append(use)
    components = []
    for members in flow_components(facts):
        attached = [claim for key in sorted(members) for claim in evidence[key]]
        properties = {}
        for property_ in ("width", "signedness", "domain"):
            relevant = [claim for claim in attached if claim["property"] == property_]
            values = {claim["value"] for claim in relevant}
            if not relevant:
                properties[property_] = {
                    "status": "unknown",
                    "value": None,
                    "changes": [],
                    "evidence": [],
                    "blockers": [],
                }
                continue
            blockers = []
            for key in sorted(members):
                declaration = facts.declarations.get(key)
                if declaration is None:
                    blockers.append({"key": key, "reason": "missing declaration"})
                    continue
                if key in facts.inconsistent:
                    blockers.append({"key": key, "reason": "inconsistent cross-TU declaration"})
                allowed_domains = {"integer", "enum"} if property_ == "domain" else {"integer"}
                if declaration.domain not in allowed_domains:
                    blockers.append(
                        {"key": key, "reason": "preserve semantic domain: " + declaration.domain}
                    )
                blockers.extend(
                    {"key": key, "reason": use.detail, "file": use.file, "line": use.line}
                    for use in sorted(escapes[key])
                )
                local_evidence = evidence[key]
                if (
                    declaration.kind
                    in {"function", "parameter", "callback-return", "callback-parameter"}
                    and key not in facts.bodies
                    and not any(c["basis"]["kind"] != "retail" for c in local_evidence)
                ):
                    blockers.append(
                        {"key": key, "reason": "missing body or unmodeled ABI boundary"}
                    )
                covered = {
                    (u["file"], u["line"], u["operation"])
                    for claim in relevant
                    if claim["key"] == key
                    for u in claim.get("covered_uses", [])
                }
                for use in sorted(operations[key]):
                    if (use.file, use.line, use.detail) not in covered:
                        blockers.append(
                            {
                                "key": key,
                                "reason": "unreviewed operation: " + use.detail,
                                "file": use.file,
                                "line": use.line,
                            }
                        )
            status = (
                "unknown"
                if not values
                else "conflict"
                if len(values) > 1
                else "blocked"
                if blockers
                else "candidate"
            )
            value = next(iter(values)) if len(values) == 1 else None
            if property_ == "signedness" and value == "unsigned":
                for key in sorted(members):
                    declaration = facts.declarations.get(key)
                    if declaration is None:
                        continue
                    sentinels = sentinels_by_key[key]
                    if sentinels:
                        blockers.append(
                            {
                                "key": key,
                                "reason": "sentinel requires signedness/domain review",
                                "sentinels": sentinels,
                            }
                        )
            if property_ == "width" and value is not None:
                for key in sorted(members):
                    for constant in sorted(facts.constants[key]):
                        # Negative/all-ones sentinels require a reviewed domain;
                        # fitting positive values alone never seed a narrow type.
                        if constant < 0 or constant >= 1 << value:
                            blockers.append(
                                {
                                    "key": key,
                                    "reason": "constant requires domain review",
                                    "constant": constant,
                                }
                            )
                if blockers and status == "candidate":
                    status = "blocked"
            if property_ == "width" and value is not None:
                # Propagation does not prove ABI/storage completeness at other
                # nodes. Every changed boundary needs its own reviewed claim.
                for key in sorted(members):
                    declaration = facts.declarations.get(key)
                    if (
                        declaration is not None
                        and declaration.width != value
                        and declaration.kind
                        in {
                            "function",
                            "parameter",
                            "field",
                            "callback-return",
                            "callback-parameter",
                        }
                        and not any(c["property"] == "width" for c in evidence[key])
                    ):
                        blockers.append({"key": key, "reason": "unreviewed width boundary"})
            if property_ == "domain" and value is not None:
                for key in sorted(members):
                    declaration = facts.declarations.get(key)
                    if (
                        declaration is not None
                        and declaration.domain == "enum"
                        and semantic_domain(declaration) != value
                    ):
                        blockers.append({"key": key, "reason": "different existing enum"})
                    if facts.constants[key] or facts.source_domains[key]:
                        blockers.append(
                            {"key": key, "reason": "producer outside named enum domain"}
                        )
            if blockers and status == "candidate":
                status = "blocked"
            changes = (
                [
                    key
                    for key in sorted(members)
                    if key in facts.declarations
                    and (
                        semantic_domain(facts.declarations[key])
                        if property_ == "domain"
                        else getattr(facts.declarations[key], property_)
                    )
                    != value
                ]
                if status == "candidate"
                else []
            )
            properties[property_] = {
                "status": status,
                "value": value,
                "changes": changes,
                "evidence": relevant,
                "blockers": blockers,
            }
        components.append({"members": sorted(members), "properties": properties})
    # Value-domain observations are inventory only. Neither 0/1 constants nor
    # predicate naming establish the historical 32-bit return declaration.
    incoming = {flow.target for flow in facts.flows}
    predicates = [
        key
        for key, declaration in sorted(facts.declarations.items())
        if declaration.kind == "function"
        and declaration.width == 32
        and declaration.domain == "integer"
        and key in facts.bodies
        and key not in incoming
        and not escapes[key]
        and (facts.constants[key] or facts.source_domains[key])
        and facts.constants[key] <= {0, 1}
        and facts.source_domains[key] <= {"bool"}
    ]
    return {
        "schema": "wiz8.scalar-report-v1",
        "report_only": True,
        "translation_units": sorted(facts.translation_units),
        "coverage": "supplied translation units and reviewed evidence; no whole-binary or source-spelling proof",
        "declarations": [vars(facts.declarations[key]) for key in sorted(facts.declarations)],
        "flows": [vars(flow) for flow in sorted(facts.flows)],
        "components": components,
        "domain_inventory": domain_inventory(facts),
        "pointer_components": anchored_report(facts, claims, "pointee"),
        "nominal_components": anchored_report(facts, claims, "nominal"),
        "callbacks": callback_report(facts),
        "structural_inventory": structural_report(facts),
        "predicate32_inventory": [
            {
                "key": key,
                "requires": "independent return ABI and symbol/signature evidence; no bool conversion",
            }
            for key in predicates
        ],
        "abi_width_inventory": abi_width_inventory(facts),
        "pointer_integer_transports": pointer_integer_transports(facts),
    }


def write_integer_report(directory: Path, evidence_path: Path | None, destination: Path) -> dict:
    facts = read_scalar_facts(directory)
    report = integer_report(facts, read_evidence(evidence_path, facts))
    # A saved AST snapshot can outlive the source that produced it. Hash its
    # inputs for attribution without calling it current or retail evidence.
    report["input_snapshot"] = {
        "current_source_status": "not_verified",
        "files": [
            {"path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
            for path in sorted(directory.glob("facts-*.tsv"))
        ],
    }
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def write_recovery_patch(
    facts: ScalarFacts, claims: list[dict], repository: Path, destination: Path
) -> dict:
    """Emit reviewable whole-component patches, never mutate source files.

    Only signedness at an unchanged width, independently named typedefs and
    void object pointees have an unambiguous type-atom edit. Width changes and
    complex signatures require further ABI/declarator recovery. All observed
    redeclarations and shared declaration atoms must agree, and every source
    snapshot is validated before writing any artifact.
    """
    repository = repository.resolve()
    report = integer_report(facts, claims)
    groups = []
    for component in report["components"]:
        proposal = component["properties"]["signedness"]
        if (
            proposal["status"] == "candidate"
            and proposal["changes"]
            and proposal["value"] in {"signed", "unsigned"}
            and not any(
                claim["property"] == "width"
                and claim["key"] in component["members"]
                and claim["value"] != facts.declarations[claim["key"]].width
                for claim in claims
            )
        ):
            groups.append(
                (component["members"], proposal["changes"], "signedness", proposal["value"])
            )
    for property_, rows in (
        ("nominal", report["nominal_components"]),
        ("pointee", report["pointer_components"]),
    ):
        for proposal in rows:
            if proposal["status"] == "candidate" and proposal["changes"]:
                groups.append(
                    (proposal["members"], proposal["changes"], property_, proposal["value"])
                )
    spans: dict[str, list[tuple]] = defaultdict(list)
    owners: dict[tuple[str, int, int], set[str]] = defaultdict(set)
    for key, file, offset, length, text, digest in sorted(facts.spans):
        spans[key].append((file, offset, length, text, digest))
        owners[file, offset, length].add(key)
    proposed: dict[str, str] = {}
    rejected = []
    for members, changes, property_, value in groups:
        edits = {}
        reason = None
        for key in changes:
            declaration = facts.declarations[key]
            if not spans[key]:
                reason = "no editable type atom for every changed declaration"
                break
            replacements = set()
            for _, _, _, text, _ in spans[key]:
                if property_ == "signedness":
                    # Preserve int/long spelling; don't strip existing typedefs.
                    unsigned = text.startswith("unsigned ")
                    base = text.removeprefix("unsigned ").removeprefix("signed ")
                    if base not in {"int", "long", "long int", "short", "short int", "char"}:
                        reason = "non-builtin spelling requires independent nominal recovery"
                        break
                    replacement = (
                        "unsigned " + base
                        if value == "unsigned"
                        else "signed char"
                        if base == "char"
                        else base
                    )
                    if unsigned == (value == "unsigned") and declaration.signedness != value:
                        reason = "source spelling disagrees with declaration observation"
                        break
                elif property_ == "pointee":
                    if text != "void":
                        reason = "only a void pointee atom is automatically editable"
                        break
                    replacement = value
                else:
                    if declaration.domain not in {"integer", "enum"}:
                        reason = "nominal edit requires an integer-domain atom"
                        break
                    replacement = value
                replacements.add(replacement)
            if reason or len(replacements) != 1:
                reason = reason or "redeclarations require different edits"
                break
            edits[key] = replacements.pop()
        if not reason:
            for key in changes:
                for file, offset, length, _, _ in spans[key]:
                    if not owners[file, offset, length] <= set(changes):
                        reason = "type atom shared with an unchanged declaration"
                        break
        if reason:
            rejected.append({"members": members, "property": property_, "reason": reason})
            continue
        for key, replacement in edits.items():
            if key in proposed and proposed[key] != replacement:
                raise ValueError(
                    "conflicting accepted recovery properties; split/review the evidence"
                )
            proposed[key] = replacement
    files: dict[str, bytes] = {}
    file_edits: dict[str, dict[tuple[int, int], tuple[bytes, bytes]]] = defaultdict(dict)
    for key, replacement in proposed.items():
        for file, offset, length, text, digest in spans[key]:
            path = (repository / file).resolve()
            if not path.is_relative_to(repository):
                raise ValueError("source span is outside the repository")
            if file not in files:
                files[file] = path.read_bytes()
            original = files[file]
            if (
                hashlib.sha256(original).hexdigest() != digest
                or original[offset : offset + length] != text.encode()
            ):
                raise ValueError(f"stale source facts for {file}; recollect before editing")
            atom = (text.encode(), replacement.encode())
            previous = file_edits[file].get((offset, length))
            if previous is not None and previous != atom:
                raise ValueError("conflicting shared type atom")
            file_edits[file][offset, length] = atom
    patch = []
    for file, edits in sorted(file_edits.items()):
        original = files[file]
        updated = original
        previous_start = len(original)
        for (offset, length), (_, replacement) in sorted(edits.items(), reverse=True):
            if offset + length > previous_start:
                raise ValueError("overlapping source edits")
            updated = updated[:offset] + replacement + updated[offset + length :]
            previous_start = offset
        label = str(Path(file).relative_to(repository)) if Path(file).is_absolute() else file
        patch.extend(
            difflib.unified_diff(
                original.decode().splitlines(keepends=True),
                updated.decode().splitlines(keepends=True),
                fromfile="a/" + label,
                tofile="b/" + label,
            )
        )
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text("".join(patch), encoding="utf-8")
    return {
        "patch": str(destination),
        "changed_declarations": sorted(proposed),
        "rejected": rejected,
        "coverage": "supplied translation units; review full project/ABI coverage before applying",
    }
