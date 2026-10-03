"""Shared source facts and conservative, report-only integer recovery.

Current AST types describe the reconstruction. Only separately reviewed evidence
seeds a recovery property; source arithmetic is a use to account for, not an oracle.
"""

from __future__ import annotations

import json
from collections import defaultdict
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
    declarations: dict[str, DeclarationFact] = field(default_factory=dict)
    flows: set[Flow] = field(default_factory=set)
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
    for path in sorted(directory.glob("facts-*.tsv")):
        for number, raw in enumerate(path.read_text(encoding="utf-8").split("\n"), 1):
            if not raw:
                continue
            try:
                parts = [decode_field(part) for part in raw.split("\t")]
                tag = parts[0]
                if tag == "D":
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
                    facts.locations.append((declaration.key, declaration.file, declaration.line))
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
                    facts.locations.extend((key, parts[4], int(parts[5])) for key in dependencies)
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


def read_evidence(path: Path | None, facts: ScalarFacts) -> list[dict]:
    if path is None:
        return []
    payload = json.loads(path.read_text(encoding="utf-8"))
    if payload.get("schema") != "wiz8.scalar-evidence-v1":
        raise ValueError("unsupported scalar evidence schema")
    claims = payload["claims"]
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
            if facts.declarations[claim["key"]].kind == "function" and not claim.get(
                "complete_return_boundary"
            ):
                raise ValueError(
                    "return width requires both producer and complete caller-consumption evidence"
                )
            if facts.declarations[claim["key"]].kind == "parameter" and not claim.get(
                "complete_argument_boundary"
            ):
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


def integer_report(facts: ScalarFacts, claims: list[dict]) -> dict:
    """Report connected copy chains; never emit edits or infer int/long spelling."""
    neighbors: dict[str, set[str]] = defaultdict(set)
    for flow in facts.flows:
        neighbors[flow.source].add(flow.target)
        neighbors[flow.target].add(flow.source)
    evidence: dict[str, list[dict]] = defaultdict(list)
    for claim in claims:
        evidence[claim["key"]].append(claim)
    operations: dict[str, list[Use]] = defaultdict(list)
    escapes: dict[str, list[Use]] = defaultdict(list)
    for use in facts.operations:
        operations[use.key].append(use)
    for use in facts.escapes:
        escapes[use.key].append(use)
    visited: set[str] = set()
    components = []
    for root in sorted(facts.declarations):
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
        attached = [claim for key in sorted(members) for claim in evidence[key]]
        properties = {}
        for property_ in ("width", "signedness", "domain"):
            relevant = [claim for claim in attached if claim["property"] == property_]
            values = {claim["value"] for claim in relevant}
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
                    declaration.kind in {"function", "parameter"}
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
                        and declaration.kind in {"function", "parameter", "field"}
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
        and declaration.name_bool
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
        "coverage": "supplied translation units and reviewed evidence; no whole-binary or source-spelling proof",
        "declarations": [vars(facts.declarations[key]) for key in sorted(facts.declarations)],
        "flows": [vars(flow) for flow in sorted(facts.flows)],
        "components": components,
        "predicate32_inventory": [
            {
                "key": key,
                "requires": "independent return ABI and symbol/signature evidence; no bool conversion",
            }
            for key in predicates
        ],
    }


def write_integer_report(directory: Path, evidence_path: Path | None, destination: Path) -> dict:
    facts = read_scalar_facts(directory)
    report = integer_report(facts, read_evidence(evidence_path, facts))
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report
