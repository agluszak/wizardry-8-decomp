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
    field_references: set[Use] = field(default_factory=set)
    character_uses: set[tuple[str, str, str, str, int, int]] = field(default_factory=set)
    linkage: dict[str, tuple[str, str]] = field(default_factory=dict)
    translation_units: set[str] = field(default_factory=set)
    # Written by the campaign before collection; None means coverage is unverified.
    expected_units: set[str] | None = None
    declarations: dict[str, DeclarationFact] = field(default_factory=dict)
    flows: set[Flow] = field(default_factory=set)
    spans: set[tuple[str, str, str, int, int, str, str]] = field(default_factory=set)
    types: dict[str, tuple[str, str, str]] = field(default_factory=dict)
    operands: set[tuple[str, str, int, str, int, int]] = field(default_factory=set)
    conversions: set[tuple[str, str, str, str]] = field(default_factory=set)
    signatures: dict[str, tuple[str, int, int, bool, str]] = field(default_factory=dict)
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
    expected = directory / "expected-units.txt"
    if expected.exists():
        facts.expected_units = set(filter(None, expected.read_text(encoding="utf-8").splitlines()))
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
                    elif tag == "FN":
                        if len(parts) != 7:
                            raise ValueError("function signature requires 7 fields")
                        signature = (
                            parts[2],
                            int(parts[3]),
                            int(parts[4]),
                            parts[5] == "1",
                            parts[6],
                        )
                        if parts[1] in facts.signatures and facts.signatures[parts[1]] != signature:
                            facts.inconsistent.add(parts[1])
                        facts.signatures[parts[1]] = signature
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
                        if len(parts) != (9 if tag == "REC" else 8):
                            raise ValueError("invalid record observation")
                        (facts.records if tag == "REC" else facts.record_fields).add(
                            tuple(parts[1:])
                        )
                    elif tag == "FR":
                        if len(parts) != 5:
                            raise ValueError("field reference requires 5 fields")
                        facts.field_references.add(
                            Use(parts[1], "reference", parts[2], int(parts[3]), int(parts[4]))
                        )
                    elif tag == "CH":
                        if len(parts) != 7:
                            raise ValueError("character observation requires 7 fields")
                        facts.character_uses.add(
                            (parts[1], parts[2], parts[3], parts[4], int(parts[5]), int(parts[6]))
                        )
                    elif tag == "LK":
                        if len(parts) != 4:
                            raise ValueError("linkage observation requires 4 fields")
                        # Redeclarations visible in different TUs may spell
                        # different DLL attributes; any visible one is a boundary.
                        linkage, dll = facts.linkage.get(parts[1], ("internal", "none"))
                        facts.linkage[parts[1]] = (
                            "external" if "external" in {linkage, parts[2]} else parts[2],
                            dll if parts[3] == "none" else parts[3],
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
                        if len(parts) != 8:
                            raise ValueError("declarator component span requires 8 fields")
                        facts.spans.add(
                            (
                                parts[1],
                                parts[2],
                                parts[3],
                                int(parts[4]),
                                int(parts[5]),
                                parts[6],
                                parts[7],
                            )
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
# Independent sources of an array extent. Retail kinds additionally require a
# reviewed complete storage-access census; contracts come from declarations.
_RETAIL_EXTENTS = {"indexing-range", "serialized-extent", "allocation-stride", "field-boundary"}
_EXTENT_KINDS = _RETAIL_EXTENTS | {"declaration-contract"}


def semantic_domain(declaration: DeclarationFact, facts: ScalarFacts) -> str:
    if declaration.domain == "enum":
        # In-class spelling may be unqualified or a typedef. Canonical TypeLoc
        # metadata identifies the same existing enum across every use context.
        identity = facts.types.get(declaration.key, (declaration.spelling, "", ""))[0]
        while identity.startswith(("const ", "volatile ")):
            identity = identity.split(" ", 1)[1]
        return "enum:" + identity.removeprefix("enum ")
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


def concrete_pointee(identity: str) -> bool:
    # cv qualification does not make erased void storage a concrete object owner.
    return bool(identity) and identity.replace("const ", "").replace("volatile ", "") != "void"


def selector_keys(facts: ScalarFacts, selector: dict) -> list[str]:
    if set(selector) != {"file", "kind", "name"}:
        raise ValueError("selector requires exactly file, kind and name")
    if selector["kind"] == "array-element":
        return [
            row[8]
            for row in facts.arrays
            if (row[1], row[4]) == (selector["file"], selector["name"])
        ]
    if selector["kind"] == "function":
        # FN includes void/record returns, which are not scalar D nodes.
        return [
            key
            for key, signature in facts.signatures.items()
            if key.split(":function:", 1)[0].rsplit(":", 2)[0] == selector["file"]
            and signature[0] == selector["name"]
        ] or [
            d.key
            for d in facts.declarations.values()
            if (d.file, d.kind, d.name) == (selector["file"], "function", selector["name"])
        ]
    return [
        d.key
        for d in facts.declarations.values()
        if (
            d.file,
            d.kind,
            d.key.split(":parameter:", 1)[-1].split("::specialization=", 1)[0]
            if d.kind == "parameter"
            else d.name,
        )
        == (selector["file"], selector["kind"], selector["name"])
    ]


def harvest_declaration_evidence(
    current: ScalarFacts, original: ScalarFacts, mappings: list[dict]
) -> dict:
    """Harvest properties from independently collected, explicitly paired ASTs.

    The caller verifies immutable source bytes or retail exports and supplies
    established correspondences. No name/range/type similarity creates a pair.
    A typedef's semantic role still requires review; mangling never seeds aliases.
    """
    claims, skipped = [], []
    aliases = {metadata[1] for metadata in current.types.values() if metadata[1]}
    pointees = {metadata[2] for metadata in current.types.values() if concrete_pointee(metadata[2])}
    enums = {
        semantic_domain(d, current) for d in current.declarations.values() if d.domain == "enum"
    }

    def exact(facts, specification):
        if "key" in specification:
            key = specification["key"]
            if key not in facts.declarations and key not in facts.signatures:
                raise ValueError("unknown paired declaration: " + key)
            return key
        if "semantic_id" in specification:
            keys = [
                key
                for key, signature in facts.signatures.items()
                if signature[4] == specification["semantic_id"]
            ]
        else:
            keys = selector_keys(facts, specification["selector"])
        if len(keys) != 1:
            raise ValueError(f"correspondence requires one declaration, found {len(keys)}")
        return keys[0]

    def parameter(facts, function, index):
        signature = facts.signatures[function]
        file = function.split(":function:", 1)[0].rsplit(":", 2)[0]
        keys = selector_keys(
            facts, {"file": file, "kind": "parameter", "name": signature[0] + "::#" + str(index)}
        )
        return keys[0] if len(keys) == 1 else None

    for mapping in mappings:
        basis = mapping["basis"]
        if basis["kind"] not in {"source-oracle", "decorated-export"}:
            raise ValueError("harvesting requires immutable source or decorated-export provenance")
        try:
            source, target = (
                exact(original, mapping["original"]),
                exact(current, mapping["current"]),
            )
        except ValueError as error:
            if basis["kind"] != "decorated-export":
                raise
            skipped.append({"mapping": mapping, "reason": str(error)})
            continue
        pairs = [(source, target, "return" if source in original.signatures else "declaration")]
        if source in original.signatures:
            a, b = original.signatures[source], current.signatures.get(target)
            if b is None or a[1:4] != b[1:4] or a[3]:
                skipped.append(
                    {"key": target, "reason": "signature arity/convention/variadic mismatch"}
                )
                continue
            for index in range(a[1]):
                pairs.append(
                    (
                        parameter(original, source, index),
                        parameter(current, target, index),
                        "#" + str(index),
                    )
                )
        pending = list(pairs)
        while pending:
            old, new, role = pending.pop(0)
            if old is None or new is None:
                skipped.append({"key": target, "reason": "unsupported paired parameter"})
                continue
            if old in original.callback_slots:
                if original.callback_slots[old] != current.callback_slots.get(new):
                    skipped.append({"key": new, "reason": "callback signature mismatch"})
                    continue
                pending.append(
                    (
                        old + "::callback-return",
                        new + "::callback-return",
                        role + ":callback-return",
                    )
                )
                pending.extend(
                    (
                        old + "::callback-arg#" + str(i),
                        new + "::callback-arg#" + str(i),
                        role + ":callback-arg#" + str(i),
                    )
                    for i in range(original.callback_slots[old][0])
                )
            declaration = original.declarations.get(old)
            if declaration is None:
                # void/record returns are outside the scalar clients.
                continue
            if (
                old in original.inconsistent
                or new in current.inconsistent
                or new not in current.declarations
            ):
                skipped.append({"key": new, "reason": "missing/inconsistent typed counterpart"})
                continue
            observed = original.types.get(old, ("", "", ""))
            properties = []
            array = next((row for row in original.arrays if row[8] == old), None)
            if array is not None:
                # A retained declaration contract states the element type and extent.
                properties.append(("extent", int(array[6])))
                if character_kind(declaration) in {"char", "wchar_t"}:
                    properties.append(("character", character_kind(declaration)))
            if declaration.domain == "integer":
                if declaration.width in {8, 16, 32}:
                    properties.append(("width", declaration.width))
                # MSVC's /J changes plain char signedness without changing
                # its decorated type. Neither a char token nor mangling proves
                # that compiler option; character semantics are a separate client.
                if observed[0] not in {
                    "char",
                    "const char",
                    "volatile char",
                    "const volatile char",
                }:
                    properties.append(("signedness", declaration.signedness))
            elif declaration.domain == "enum" and semantic_domain(declaration, original) in enums:
                properties.append(("domain", semantic_domain(declaration, original)))
            elif (
                declaration.domain == "pointer"
                and concrete_pointee(observed[2])
                and observed[2] in pointees
            ):
                properties.append(("pointee", observed[2]))
            nominal_role = mapping.get("nominal_roles", {}).get(role)
            if basis["kind"] == "source-oracle" and nominal_role and observed[1] in aliases:
                properties.append(("nominal", observed[1]))
            for property_, value in properties:
                claim = {
                    "key": new,
                    "property": property_,
                    "value": value,
                    "basis": {
                        **basis,
                        "declaration": {
                            "file": declaration.file,
                            "line": declaration.line,
                            "column": declaration.column,
                        },
                    },
                }
                if property_ == "nominal":
                    claim["role"] = nominal_role
                if property_ == "extent":
                    claim["basis"]["extent_kind"] = "declaration-contract"
                if property_ == "width":
                    # The explicitly retained declaration contract establishes
                    # this ABI boundary. Retail-only width evidence retains its
                    # separate producer/caller/storage completeness requirements.
                    if declaration.kind in {"function", "callback-return"}:
                        claim["complete_return_boundary"] = True
                    if declaration.kind in {"parameter", "callback-parameter"}:
                        claim["complete_argument_boundary"] = True
                claims.append(claim)
    unique = {json.dumps(claim, sort_keys=True): claim for claim in claims}
    return {
        "schema": "wiz8.scalar-evidence-v1",
        "claims": [unique[key] for key in sorted(unique)],
        "skipped": skipped,
    }


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
            matches = selector_keys(facts, selector)
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
                or not concrete_pointee(value)
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
                semantic_domain(declaration, facts)
                for declaration in facts.declarations.values()
                if declaration.domain == "enum"
            }:
                raise ValueError("cannot invent an enum from numeric ranges")
            if basis["kind"] == "retail":
                raise ValueError(
                    "enum identity requires independent symbol, header or source evidence"
                )
        elif property_ in {"character", "extent"}:
            if facts.declarations[claim["key"]].kind != "array-element":
                raise ValueError(f"{property_} recovery applies to fixed array element storage")
            if property_ == "character":
                if value not in {"char", "wchar_t", "raw-byte"}:
                    raise ValueError("character recovery requires char, wchar_t or raw-byte")
                if basis["kind"] == "retail" and basis.get("value_width") != (
                    16 if value == "wchar_t" else 8
                ):
                    raise ValueError("retail character evidence must identify the code-unit width")
            else:
                if not isinstance(value, int) or isinstance(value, bool) or value <= 0:
                    raise ValueError("array extent must be a positive integer")
                extent_kind = basis.get("extent_kind")
                if extent_kind not in _EXTENT_KINDS:
                    raise ValueError("extent evidence requires a reviewed extent_kind")
                if basis["kind"] == "retail" and (
                    extent_kind not in _RETAIL_EXTENTS or not claim.get("complete_storage_accesses")
                ):
                    raise ValueError(
                        "retail extent requires a complete receiver-attributed storage census"
                    )
        else:
            raise ValueError("unsupported scalar property")
    return claims


_BINDING_ROLES = {"callback-return", "callback-parameter", "callback-argument"}

# What one directed transfer means for each recovered property. `equal` joins
# a component; `barrier` is independent storage; `review` blocks a change at
# either endpoint and `review-source` a change of the producer whose
# representation the conversion consumes; `producer` lets an erased consumer
# change only when its producer already has the recovered value. Unlisted
# classes are barriers.
_EDGE_POLICY: dict[str, dict[str, str]] = {
    "signedness": {
        "copy": "equal",
        "alias-change": "equal",
        "binding": "equal",
        "widening": "review-source",
        "domain-change": "review-source",
        "conversion": "conversion",
    },
    "width": {
        "copy": "equal",
        "alias-change": "equal",
        "binding": "equal",
        "widening": "review",
        "narrowing": "review",
        "domain-change": "review",
    },
    "domain": {
        "copy": "equal",
        "alias-change": "equal",
        "binding": "equal",
        "domain-change": "enum-integer",
        "widening": "review",
        "narrowing": "review",
    },
    "pointee": {
        "copy": "equal",
        "alias-change": "equal",
        "binding": "equal",
        "erasure": "producer",
    },
    "nominal": {"copy": "equal", "binding": "equal", "alias-change": "producer"},
    "character": {
        "copy": "equal",
        "alias-change": "equal",
        "binding": "equal",
        "widening": "review",
        "narrowing": "review",
        "domain-change": "review",
    },
}
_SIGNATURE_KINDS = {"function", "parameter", "callback-return", "callback-parameter"}


def edge_class(facts: ScalarFacts, flow: Flow) -> str:
    """Semantic class of one directed transfer, from both endpoint declarations."""
    if flow.role in _BINDING_ROLES:
        return "binding"
    if flow.role == "explicit-conversion":
        return "conversion"
    source = facts.declarations.get(flow.source)
    target = facts.declarations.get(flow.target)
    if source is None or target is None:
        return "copy"
    if source.domain != target.domain:
        return "domain-change"
    source_type = facts.types.get(flow.source, ("", "", ""))
    target_type = facts.types.get(flow.target, ("", "", ""))
    if source.domain == "pointer" and source_type[2] != target_type[2]:
        if concrete_pointee(source_type[2]) and not concrete_pointee(target_type[2]):
            return "erasure"
        return "pointer-conversion"
    if source.width != target.width:
        return "widening" if source.width < target.width else "narrowing"
    if source_type[1] != target_type[1]:
        return "alias-change"
    return "copy"


def edge_mode(facts: ScalarFacts, flow: Flow, property_: str) -> str:
    mode = _EDGE_POLICY[property_].get(edge_class(facts, flow), "barrier")
    source = facts.declarations.get(flow.source)
    target = facts.declarations.get(flow.target)
    if mode == "conversion":
        # A cast to a same-or-narrower integer is independent of the
        # producer's signedness; widening or a domain change consumes it.
        narrowing = (
            source is not None
            and target is not None
            and target.domain == "integer"
            and target.width <= source.width
        )
        return "barrier" if narrowing else "review-source"
    if mode == "enum-integer":
        domains = {source.domain, target.domain} if source and target else set()
        return "equal" if domains == {"enum", "integer"} else "producer"
    return mode


def property_constraints(
    facts: ScalarFacts, property_: str
) -> tuple[list[set[str]], dict[str, list[tuple[Flow, str]]]]:
    """Components joined by this property's equality edges, plus edge constraints."""
    parent: dict[str, str] = {}

    def find(key: str) -> str:
        parent.setdefault(key, key)
        while parent[key] != key:
            parent[key] = parent[parent[key]]
            key = parent[key]
        return key

    constraints: dict[str, list[tuple[Flow, str]]] = defaultdict(list)
    for key in facts.declarations:
        find(key)
    for flow in sorted(facts.flows):
        mode = edge_mode(facts, flow, property_)
        if mode == "equal":
            parent[find(flow.source)] = find(flow.target)
        else:
            find(flow.source)
            find(flow.target)
            if mode != "barrier":
                constraints[flow.source].append((flow, mode))
                constraints[flow.target].append((flow, mode))
    groups: dict[str, set[str]] = defaultdict(set)
    for key in list(parent):
        groups[find(key)].add(key)
    return sorted(groups.values(), key=min), constraints


def flow_components(facts: ScalarFacts, property_: str | None = None) -> list[set[str]]:
    """Connected source transfers; a property selects its own equality edges.

    Without a property every transfer connects its endpoints: the finite
    value-domain inventory reports observed behavior, not a recovered type.
    """
    if property_ is not None:
        return property_constraints(facts, property_)[0]
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
    # Truncation can change any observed value.
    escaped.update(flow.target for flow in facts.flows if edge_class(facts, flow) == "narrowing")
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


def callback_boundaries(facts: ScalarFacts) -> tuple[set[str], dict[str, list[str]]]:
    """A modeled slot is complete only when every bound implementation has a body.

    Synthetic signature nodes have no body of their own. Their implementation
    edges establish source-side coverage, while escapes/ABI mismatches remain
    blockers even when independent evidence seeds the slot directly.
    """
    complete = set()
    blocked = {}
    implementations = defaultdict(set)
    for flow in facts.flows:
        if flow.role == "callback-return":
            implementations[flow.target].add(flow.source)
        elif flow.role == "callback-parameter":
            implementations[flow.source].add(flow.target)
    for slot in callback_report(facts):
        for key in slot["nodes"]:
            reasons = list(slot["blockers"])
            if not implementations[key] or not implementations[key] <= facts.bodies:
                reasons.append("unavailable callback implementation boundary")
            if reasons:
                blocked[key] = reasons
            else:
                complete.add(key)
    return complete, blocked


def source_boundaries(facts: ScalarFacts) -> dict[str, list[str]]:
    """Source-side reasons an entity's boundary is not completely collected.

    This is the machine-checkable half of a boundary-completeness claim: the
    configured corpus was collected, bodies exist, and nothing outside it can
    name or reach the entity through DLL linkage, address-taking or aggregate
    storage. Retail producer/consumer review remains the evidence's assertion.
    """
    coverage = []
    if facts.expected_units is None:
        coverage.append("translation-unit coverage unverified")
    elif facts.expected_units - facts.translation_units:
        coverage.append("configured translation units were not collected")
    escaped: dict[str, set[str]] = defaultdict(set)
    for use in facts.escapes:
        escaped[use.key].add(use.detail)
    result = {}
    for key, declaration in facts.declarations.items():
        reasons = list(coverage)
        if declaration.kind in {"function", "parameter"} and key not in facts.bodies:
            reasons.append("no collected body")
        dll = facts.linkage.get(key, ("", "none"))[1]
        if dll != "none":
            reasons.append("dll " + dll + " boundary")
        reasons.extend(
            sorted(
                escaped[key]
                & {
                    "function pointer",
                    "virtual slot",
                    "address taken",
                    "reference binding",
                    "aggregate storage argument",
                    "indirect storage argument",
                    "array storage argument",
                }
            )
        )
        result[key] = reasons
    return result


_CHARACTER_BITS = {"char": 8, "wchar_t": 16, "raw-byte": 8}


def character_kind(declaration: DeclarationFact) -> str:
    """Current character representation of a byte/code-unit declaration."""
    spelling = re.sub(r"\b(?:const|volatile)\b", "", declaration.spelling).strip()
    if spelling == "char":
        return "char"
    if spelling in {"wchar_t", "WCHAR"}:
        return "wchar_t"
    if declaration.width == 8 and declaration.domain in {"integer", "character"}:
        return "raw-byte"
    return "integer" + str(declaration.width)


def property_value(facts: ScalarFacts, key: str, property_: str):
    if property_ in {"pointee", "nominal"}:
        metadata = facts.types.get(key)
        return None if metadata is None else metadata[2 if property_ == "pointee" else 1]
    declaration = facts.declarations.get(key)
    if declaration is None:
        return None
    if property_ == "domain":
        return semantic_domain(declaration, facts)
    if property_ == "character":
        return character_kind(declaration)
    return getattr(declaration, property_)


def _indexes(facts: ScalarFacts) -> dict:
    """Lookups shared by every property evaluated over one fact snapshot."""
    callback_complete, callback_blocked = callback_boundaries(facts)
    index = {
        "callback_complete": callback_complete,
        "callback_blocked": callback_blocked,
        "sentinels": sentinel_index(facts),
        "operations": defaultdict(list),
        "escapes": defaultdict(list),
        "conversions": defaultdict(list),
        "conversion_sites": {
            (key, flow.file, flow.line)
            for flow in facts.flows
            if flow.role == "explicit-conversion"
            for key in (flow.source, flow.target)
        },
        "owner_types": defaultdict(set),
        "arrays": {row[8]: row for row in facts.arrays if row[8]},
        "array_uses": defaultdict(list),
        "characters": defaultdict(list),
        "boundaries": source_boundaries(facts),
    }
    for use in sorted(facts.operations):
        index["operations"][use.key].append(use)
    for use in sorted(facts.escapes):
        index["escapes"][use.key].append(use)
    for conversion in sorted(facts.conversions):
        for key in set(conversion[:2]):
            index["conversions"][key].append(conversion)
    for key, metadata in facts.types.items():
        declaration = facts.declarations.get(key)
        if metadata[1] and declaration is not None:
            index["owner_types"][metadata[1]].add(nominal_representation(declaration, metadata))
    for use in sorted(facts.array_uses):
        index["array_uses"][use.key].append(use)
    for row in sorted(facts.character_uses):
        index["characters"][row[0]].append(row)
    return index


def _escape_modeled(facts: ScalarFacts, index: dict, key: str, use: Use, property_: str, value):
    """True when an escape is represented by this property's edge or consumer model."""
    if (
        use.detail == "explicit conversion"
        and property_ != "pointee"
        and (key, use.file, use.line) in index["conversion_sites"]
    ):
        return True
    if property_ == "pointee":
        metadata = facts.types.get(key, ("", "", ""))
        # A by-value T* argument cannot change its pointer storage.
        # Aggregate fields, address-taking, references and T** remain barriers.
        if use.detail == "indirect storage argument" and metadata[2] and "*" not in metadata[2]:
            return True
        if use.detail == "explicit conversion":
            local = index["conversions"][key]
            return bool(
                value
                and local
                and all(
                    {source, target} <= {"void *", value + " *"} for *_, source, target in local
                )
            )
    # Storage passed to a modeled consumer, including the cast that recovery
    # makes redundant, is represented by that consumer's character contract.
    if property_ == "character" and use.detail in {
        "array storage argument",
        "indirect storage argument",
        "explicit conversion",
    }:
        owner = index["arrays"].get(key, ("",))[0]
        return any(
            (row[3], row[4]) == (use.file, use.line) for row in index["characters"].get(owner, [])
        )
    return False


def _member_blockers(
    facts: ScalarFacts,
    index: dict,
    key: str,
    property_: str,
    value,
    relevant: list[dict],
    evidence: dict[str, list[dict]],
) -> list[dict]:
    blockers = [{"key": key, "reason": reason} for reason in index["callback_blocked"].get(key, [])]
    declaration = facts.declarations.get(key)
    metadata = facts.types.get(key)
    if declaration is None or (property_ in {"pointee", "nominal"} and metadata is None):
        return [*blockers, {"key": key, "reason": "missing typed declaration"}]
    if key in facts.inconsistent:
        blockers.append({"key": key, "reason": "inconsistent cross-TU declaration"})
    if (
        declaration.kind in _SIGNATURE_KINDS
        and key not in facts.bodies | index["callback_complete"]
        and not any(claim["basis"]["kind"] != "retail" for claim in evidence[key])
    ):
        blockers.append({"key": key, "reason": "missing body or unmodeled ABI boundary"})
    allowed = {
        "width": {"integer"},
        "signedness": {"integer"},
        "domain": {"integer", "enum"},
        "character": {"integer", "character"},
    }.get(property_)
    if allowed is not None and declaration.domain not in allowed:
        blockers.append({"key": key, "reason": "preserve semantic domain: " + declaration.domain})
    for use in index["escapes"][key]:
        if not _escape_modeled(facts, index, key, use, property_, value):
            blockers.append({"key": key, "reason": use.detail, "file": use.file, "line": use.line})
    covered = {
        (use["file"], use["line"], use["operation"])
        for claim in relevant
        if claim["key"] == key
        for use in claim.get("covered_uses", [])
    }
    for use in index["operations"][key]:
        if (use.file, use.line, use.detail) not in covered:
            blockers.append(
                {
                    "key": key,
                    "reason": "unreviewed operation: " + use.detail,
                    "file": use.file,
                    "line": use.line,
                }
            )
    if value is None:
        return blockers
    current = property_value(facts, key, property_)
    if property_ == "pointee":
        if declaration.domain != "pointer" or not metadata[2]:
            blockers.append({"key": key, "reason": "non-object-pointer domain"})
        elif metadata[2] not in {"void", value}:
            blockers.append(
                {"key": key, "reason": "conflicting pointee or qualifiers", "pointee": metadata[2]}
            )
        if any(constant != 0 for constant in facts.constants[key]):
            blockers.append({"key": key, "reason": "non-null numeric pointer producer"})
    elif property_ == "nominal":
        if nominal_representation(declaration, metadata) not in index["owner_types"][value] or (
            metadata[1] and metadata[1] != value
        ):
            blockers.append(
                {"key": key, "reason": "different representation or existing typedef owner"}
            )
        if index["sentinels"][key]:
            blockers.append({"key": key, "reason": "sentinel requires nominal-domain review"})
    elif property_ == "signedness" and value == "unsigned" and index["sentinels"][key]:
        blockers.append(
            {
                "key": key,
                "reason": "sentinel requires signedness/domain review",
                "sentinels": index["sentinels"][key],
            }
        )
    elif property_ == "width":
        for constant in sorted(facts.constants[key]):
            # Negative/all-ones sentinels require a reviewed domain;
            # fitting positive values alone never seed a narrow type.
            if constant < 0 or constant >= 1 << value:
                blockers.append(
                    {"key": key, "reason": "constant requires domain review", "constant": constant}
                )
        if current != value and declaration.kind in _SIGNATURE_KINDS | {"field"}:
            # Propagation does not prove ABI/storage completeness at other
            # nodes. Every changed boundary needs its own reviewed claim, and
            # the source side of that boundary must be completely collected.
            if not any(claim["property"] == "width" for claim in evidence[key]):
                blockers.append({"key": key, "reason": "unreviewed width boundary"})
            blockers.extend(
                {"key": key, "reason": "source boundary incomplete: " + reason}
                for reason in index["boundaries"].get(key, [])
            )
    elif property_ == "domain":
        if declaration.domain == "enum" and current != value:
            blockers.append({"key": key, "reason": "different existing enum"})
        if facts.constants[key] or facts.source_domains[key]:
            blockers.append({"key": key, "reason": "producer outside named enum domain"})
    elif property_ == "character":
        blockers.extend(_character_blockers(facts, index, key, value, current, evidence))
    if current != value and declaration.kind in _SIGNATURE_KINDS | {"variable"}:
        # A changed declaration of a DLL-visible entity changes its decorated ABI.
        dll = facts.linkage.get(key, ("", "none"))[1]
        if dll != "none":
            blockers.append({"key": key, "reason": "dll " + dll + " signature boundary"})
    return blockers


def _character_blockers(facts, index, key, value, current, evidence) -> list[dict]:
    row = index["arrays"].get(key)
    if row is None:
        if current != value:
            return [{"key": key, "reason": "scalar character recovery requires review"}]
        return []
    blockers = []
    for _, kind, role, file, line, _ in index["characters"].get(row[0], []):
        if kind.startswith(("bytes:", "units:")) or kind in {value, "raw-byte"}:
            continue
        consumer = "project consumer" if role.startswith("call:") else "consumer"
        blockers.append(
            {
                "key": key,
                "reason": f"{consumer} {role} uses {kind} storage",
                "file": file,
                "line": line,
            }
        )
    if _CHARACTER_BITS[value] != int(row[7]) and not any(
        claim["property"] == "extent" for claim in evidence[key]
    ):
        blockers.append({"key": key, "reason": "element width change requires extent evidence"})
    return blockers


def _edge_blockers(facts, members, changing, constraints, property_, value) -> list[dict]:
    blockers, seen = [], set()
    for key in sorted(members):
        for flow, mode in constraints.get(key, []):
            if (flow, mode) in seen:
                continue
            seen.add((flow, mode))
            source_changes, target_changes = flow.source in changing, flow.target in changing
            reason = None
            if (mode == "review" and (source_changes or target_changes)) or (
                mode == "review-source" and source_changes
            ):
                reason = edge_class(facts, flow) + " transfer requires review"
            elif (
                mode == "producer"
                and target_changes
                and property_value(facts, flow.source, property_) != value
            ):
                reason = "producer does not have the recovered value"
            if reason:
                blockers.append(
                    {
                        "key": flow.target if mode == "producer" else key,
                        "reason": reason,
                        "source": flow.source,
                        "target": flow.target,
                        "file": flow.file,
                        "line": flow.line,
                    }
                )
    return blockers


def component_report(
    facts: ScalarFacts, claims: list[dict], property_: str, index: dict | None = None
) -> list[dict]:
    """Evaluate one recovered property over its own propagation components."""
    index = index or _indexes(facts)
    components, constraints = property_constraints(facts, property_)
    evidence: dict[str, list[dict]] = defaultdict(list)
    for claim in claims:
        evidence[claim["key"]].append(claim)
    result = []
    for members in components:
        relevant = [
            claim
            for key in sorted(members)
            for claim in evidence[key]
            if claim["property"] == property_
        ]
        values = {json.dumps(claim["value"]) for claim in relevant}
        if not relevant:
            result.append(
                {
                    "members": sorted(members),
                    "status": "unknown",
                    "value": None,
                    "changes": [],
                    "evidence": [],
                    "blockers": [],
                }
            )
            continue
        value = relevant[0]["value"] if len(values) == 1 else None
        changing = (
            {
                key
                for key in members
                if key in facts.declarations and property_value(facts, key, property_) != value
            }
            if value is not None
            else set()
        )
        blockers = [
            blocker
            for key in sorted(members)
            for blocker in _member_blockers(facts, index, key, property_, value, relevant, evidence)
        ]
        blockers.extend(_edge_blockers(facts, members, changing, constraints, property_, value))
        status = "conflict" if value is None else "blocked" if blockers else "candidate"
        result.append(
            {
                "members": sorted(members),
                "status": status,
                "value": value,
                "changes": sorted(changing) if status == "candidate" else [],
                "evidence": relevant,
                "blockers": blockers,
            }
        )
    return result


def anchored_report(facts: ScalarFacts, claims: list[dict], property_: str) -> list[dict]:
    """Existing pointer/typedef identities require independent owner evidence."""
    return component_report(facts, claims, property_)


def callback_report(facts: ScalarFacts) -> list[dict]:
    # Index once: scanning/sorting the entire declaration graph per callback
    # turns a whole-program campaign into a quadratic inventory operation.
    slot_nodes = defaultdict(list)
    for key in sorted(facts.declarations):
        if "::callback-" in key:
            slot_nodes[key.rsplit("::callback-", 1)[0]].append(key)
    slot_bindings = defaultdict(list)
    for binding in sorted(facts.callback_bindings):
        slot_bindings[binding[0]].append(binding)
    escaped = {use.key for use in facts.escapes}
    result = []
    for slot, signature in sorted(facts.callback_slots.items()):
        bindings = slot_bindings[slot]
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
        if slot in escaped:
            blockers.append("callback slot escapes or has an unmodeled producer")
        result.append(
            {
                "slot": slot,
                "arity": signature[0],
                "calling_convention": signature[1],
                "implementations": [binding[1] for binding in bindings],
                "nodes": slot_nodes[slot],
                "status": "blocked" if blockers else "modeled",
                "blockers": blockers,
                "requires": "independent signature evidence and complete implementation/table/invocation coverage before edits",
            }
        )
    return result


def _record_index(
    facts: ScalarFacts,
) -> tuple[dict[str, tuple], dict[str, list[tuple]], set[str]]:
    """Records, their fields, and records compiled differently by different TUs.

    A member type defined under a different active `#pragma pack` in another
    translation unit (e.g. Win32 structs first seen inside MSS.H's pack(1))
    gives one source record several compiled layouts. No single replay can
    represent it, so it is reported rather than resolved.
    """
    records: dict[str, tuple] = {}
    divergent: set[str] = set()
    for row in sorted(facts.records):
        if row[0] in records:
            divergent.add(row[0])
        records[row[0]] = row
    fields: dict[str, list[tuple]] = defaultdict(list)
    seen: dict[str, tuple] = {}
    for record, key, name, offset, size, align, type_ in sorted(facts.record_fields):
        row = (int(offset), key, name, int(size), int(align), type_)
        if key in seen:
            divergent.add(record)
            continue
        seen[key] = row
        fields[record].append(row)
    for rows in fields.values():
        rows.sort()
    return records, fields, divergent


def relayout(
    record: tuple,
    fields: list[tuple],
    overrides: dict[str, tuple[int, int]] | None = None,
    removed: set[str] | frozenset[str] = frozenset(),
    packing: int | None = None,
) -> dict | None:
    """Replay natural MSVC placement for a simple record (bits), or None.

    `#pragma pack(n)` caps every member alignment. Records with bases,
    polymorphism, unions, bitfields or attributed members have no replay.
    """
    if record[6] == "unknown":
        return None
    pack = int(record[7]) if packing is None else packing
    offset, alignment, offsets = 0, 8, {}
    for _, key, _, size, align, _ in fields:
        if key in removed:
            continue
        size, align = (overrides or {}).get(key, (size, align))
        if pack:
            align = min(align, pack)
        offset = -(-offset // align) * align
        offsets[key] = offset
        offset += size
        alignment = max(alignment, align)
    return {"offsets": offsets, "size": -(-offset // alignment) * alignment, "alignment": alignment}


def _layout_of(record: tuple, fields: list[tuple]) -> dict:
    return {
        "offsets": {key: offset for offset, key, *_ in fields},
        "size": int(record[4]),
        "alignment": int(record[5]),
    }


def _layout_delta(before: dict, after: dict, ignored: set[str]) -> list[str]:
    delta = [
        f"{key} moves {offset // 8} -> {after['offsets'].get(key, 0) // 8}"
        for key, offset in sorted(before["offsets"].items())
        if key not in ignored and after["offsets"].get(key) != offset
    ]
    for property_ in ("size", "alignment"):
        if before[property_] != after[property_]:
            delta.append(f"record {property_} {before[property_] // 8} -> {after[property_] // 8}")
    return delta


_PADDING_NAME = re.compile(r"_?(?:pad|padding|align|alignment)(?:_?[0-9A-Za-z]+)*", re.IGNORECASE)
_PADDING_TYPE = re.compile(r"(?:(?:un)?signed )?(?:char|short)(?: ?\[\d+\])?")


def padding_report(facts: ScalarFacts) -> list[dict]:
    """Explicit padding members that natural placement already reproduces.

    A member qualifies only when it is padding-shaped, never referenced or
    initialized, owns a sole declaration span, and removing it leaves every
    other member offset, the record size and the alignment unchanged.
    """
    records, fields, divergent = _record_index(facts)
    referenced = {use.key for use in facts.field_references} | {use.key for use in facts.array_uses}
    for flow in facts.flows:
        referenced.update((flow.source, flow.target))
    referenced.update(use.key for use in facts.operations | facts.escapes)
    referenced.update(key for key, values in facts.constants.items() if values)
    referenced.update(row[0] for row in facts.operands)
    referenced.update(row[0] for row in facts.character_uses)
    referenced = {key.removesuffix("::element") for key in referenced}
    aggregate = {use.key for use in facts.array_uses if use.detail == "aggregate-initializer"}
    spans = defaultdict(list)
    for key, component, *_ in facts.spans:
        if component == "field-declaration":
            spans[key].append(component)
    rows = []
    for record_key, record in sorted(records.items()):
        members = fields[record_key]
        shaped = [
            row
            for row in members
            if _PADDING_NAME.fullmatch(row[2]) and _PADDING_TYPE.fullmatch(row[5])
        ]
        if not shaped:
            continue
        actual = _layout_of(record, members)
        replay = relayout(record, members)
        accepted: set[str] = set()
        for offset, key, name, *_ in shaped:
            reason = None
            if record_key in divergent:
                reason = "record layout differs across translation units"
            elif replay is None:
                reason = "record has no natural layout replay"
            elif _layout_delta(actual, replay, set()):
                reason = "layout replay disagrees with the compiled record"
            elif record_key in aggregate:
                reason = "record has positional aggregate initializers"
            elif key in referenced:
                reason = "member is referenced"
            elif len(spans[key]) != 1:
                reason = "no sole removable declaration"
            else:
                trial = relayout(record, members, removed=accepted | {key})
                assert trial is not None
                delta = _layout_delta(actual, trial, accepted | {key})
                if delta:
                    reason = "removal changes layout: " + "; ".join(delta)
            if reason is None:
                accepted.add(key)
            rows.append(
                {
                    "record": record_key,
                    "record_name": record[3],
                    "key": key,
                    "name": name,
                    "offset": offset // 8,
                    "status": "blocked" if reason else "candidate",
                    "reason": reason,
                }
            )
    return rows


def extent_report(facts: ScalarFacts, claims: list[dict], index: dict | None = None) -> list[dict]:
    """Array extents need independent extent evidence and a complete source census."""
    index = index or _indexes(facts)
    spans = defaultdict(list)
    for key, component, file, offset, length, text, digest in facts.spans:
        if component == "array-extent":
            spans[key].append(text)
    claimed: dict[str, list[dict]] = defaultdict(list)
    for claim in claims:
        if claim["property"] == "extent":
            claimed[claim["key"]].append(claim)
    character = {
        claim["key"]: claim["value"] for claim in claims if claim["property"] == "character"
    }
    result = []
    for key, relevant in sorted(claimed.items()):
        row = index["arrays"][key]
        values = {claim["value"] for claim in relevant}
        value = next(iter(values)) if len(values) == 1 else None
        current = int(row[6])
        element_bits = _CHARACTER_BITS.get(character.get(key, ""), int(row[7]))
        covered = {
            (use["file"], use["line"], use["operation"])
            for claim in relevant
            for use in claim.get("covered_uses", [])
        }
        blockers = []
        if key in facts.inconsistent:
            blockers.append({"key": key, "reason": "inconsistent cross-TU declaration"})
        if not spans[key]:
            blockers.append({"key": key, "reason": "no editable extent declaration"})
        for text in spans[key]:
            if not re.fullmatch(r"(?:0[xX][0-9A-Fa-f]+|[1-9][0-9]*)[uUlL]*", text):
                blockers.append(
                    {"key": key, "reason": "symbolic extent requires its constant owner: " + text}
                )
        for use in index["array_uses"][row[0]] if value is not None else []:
            site = (use.file, use.line)
            if use.detail == "sizeof" and (*site, "sizeof") not in covered:
                blockers.append(
                    {"key": key, "reason": "sizeof depends on the extent", **_site(use)}
                )
            elif use.detail.startswith("index:") and int(use.detail[6:]) >= value:
                blockers.append(
                    {
                        "key": key,
                        "reason": "constant index outside extent: " + use.detail[6:],
                        **_site(use),
                    }
                )
            elif use.detail == "indexed" and value < current and (*site, "indexed") not in covered:
                blockers.append(
                    {"key": key, "reason": "shrinking requires reviewed index bounds", **_site(use)}
                )
        for _, kind, role, file, line, _ in index["characters"].get(row[0], []) if value else []:
            if kind.startswith("units:") and int(kind[6:]) > value:
                blockers.append({"key": key, "reason": f"{role} needs {kind[6:]} elements"})
            if kind.startswith("bytes:") and int(kind[6:]) * 8 > value * element_bits:
                blockers.append({"key": key, "reason": f"{role} accesses {kind[6:]} bytes"})
        status = "conflict" if value is None else "blocked" if blockers else "candidate"
        result.append(
            {
                "key": key,
                "array": row[0],
                "name": row[4],
                "current": current,
                "value": value,
                "status": status,
                "changes": [key] if status == "candidate" and value != current else [],
                "blockers": blockers,
                "evidence": relevant,
            }
        )
    return result


def _site(use: Use) -> dict:
    return {"file": use.file, "line": use.line}


def array_report(facts: ScalarFacts, claims: list[dict], index: dict | None = None) -> dict:
    """Character representation and extent recovery of fixed arrays.

    Each property is evaluated independently, then every record containing a
    changed member is replayed: surviving offsets, size and alignment must not
    move, otherwise every proposal touching that record is blocked.
    """
    index = index or _indexes(facts)
    characters = component_report(facts, claims, "character", index)
    extents = extent_report(facts, claims, index)
    records, fields, divergent = _record_index(facts)
    owner_record = {row[1]: record for record, rows in fields.items() for row in rows}
    shape: dict[str, list] = {}
    for component in characters:
        for key in component["changes"]:
            if key in index["arrays"]:
                row = index["arrays"][key]
                shape.setdefault(key, [int(row[7]), int(row[6])])[0] = _CHARACTER_BITS[
                    component["value"]
                ]
    for row in extents:
        if row["changes"]:
            array = index["arrays"][row["key"]]
            shape.setdefault(row["key"], [int(array[7]), int(array[6])])[1] = row["value"]
    affected: dict[str, dict[str, tuple[int, int]]] = defaultdict(dict)
    for key, (bits, extent) in shape.items():
        record = owner_record.get(index["arrays"][key][0])
        if record is not None:
            affected[record][index["arrays"][key][0]] = (bits * extent, bits)
    failures: dict[str, str] = {}
    for record_key, overrides in affected.items():
        record, members = records[record_key], fields[record_key]
        before, after = _layout_of(record, members), relayout(record, members, overrides)
        if record_key in divergent:
            failures[record_key] = "record layout differs across translation units"
        elif after is None:
            failures[record_key] = "record has no natural layout replay"
        elif _layout_delta(before, relayout(record, members) or before, set()):
            failures[record_key] = "layout replay disagrees with the compiled record"
        elif delta := _layout_delta(before, after, set(overrides)):
            failures[record_key] = "record layout changes: " + "; ".join(delta)
    for item in [*characters, *extents]:
        keys = item["changes"]
        reasons = {
            failures[owner_record[index["arrays"][key][0]]]
            for key in keys
            if key in index["arrays"] and owner_record.get(index["arrays"][key][0]) in failures
        }
        if reasons:
            item["blockers"].extend({"reason": reason} for reason in sorted(reasons))
            item["status"], item["changes"] = "blocked", []
    return {"character_components": characters, "extents": extents}


def structural_report(facts: ScalarFacts) -> dict:
    """Source layout/use inventory plus layout-preserving padding candidates."""
    uses = defaultdict(set)
    for use in facts.array_uses:
        uses[use.key].add(use.detail)
    characters = defaultdict(set)
    for owner, kind, role, *_ in facts.character_uses:
        characters[owner].add(role + "=" + kind)
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
                "character_uses": sorted(characters[key]),
                "text_initializer": "string-initializer" in roles,
                "requires": "complete storage/access and independent extent evidence",
            }
        )
    records, fields, divergent = _record_index(facts)
    layouts = defaultdict(list)
    rows = []
    for key, record in sorted(records.items()):
        _, file, line, name, size, align, natural, pack = record
        members = fields[key]
        layouts[(int(size), tuple((o, s, t) for o, _, _, s, _, t in members))].append(key)
        unpacked = relayout(record, members, packing=0) if int(pack) else None
        rows.append(
            {
                "key": key,
                "file": file,
                "line": int(line),
                "name": name,
                "size_bits": int(size),
                "alignment_bits": int(align),
                "natural_size_bits": None if natural == "unknown" else int(natural),
                "packing_changes_size": None if natural == "unknown" else int(natural) != int(size),
                "packing_bits": int(pack),
                "layout_differs_across_units": key in divergent,
                "redundant_packing": None
                if unpacked is None
                else not _layout_delta(_layout_of(record, members), unpacked, set()),
                "requires": "retail offsets/stride before changing packing or merging records",
            }
        )
    return {
        "arrays": arrays,
        "records": rows,
        "duplicate_layouts": [
            keys for (size, shape), keys in sorted(layouts.items()) if shape and len(keys) > 1
        ],
        "padding": padding_report(facts),
        "divergent_layouts": sorted(divergent),
    }


def integer_report(facts: ScalarFacts, claims: list[dict]) -> dict:
    """Report per-property components; never infer int/long spelling."""
    escaped = {use.key for use in facts.escapes}
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
        and key not in escaped
        and (facts.constants[key] or facts.source_domains[key])
        and facts.constants[key] <= {0, 1}
        and facts.source_domains[key] <= {"bool"}
    ]
    index = _indexes(facts)
    boundaries = index["boundaries"]
    return {
        "schema": "wiz8.scalar-report-v2",
        "report_only": True,
        "translation_units": sorted(facts.translation_units),
        "coverage": {
            "scope": "supplied translation units and reviewed evidence; no whole-binary or source-spelling proof",
            "expected_translation_units": None
            if facts.expected_units is None
            else sorted(facts.expected_units),
            "complete": facts.expected_units is not None
            and facts.expected_units <= facts.translation_units,
            "source_complete_boundaries": sum(not reasons for reasons in boundaries.values()),
        },
        "declarations": [vars(facts.declarations[key]) for key in sorted(facts.declarations)],
        "flows": [{**vars(flow), "class": edge_class(facts, flow)} for flow in sorted(facts.flows)],
        "integer_components": {
            property_: component_report(facts, claims, property_, index)
            for property_ in ("width", "signedness", "domain")
        },
        "domain_inventory": domain_inventory(facts),
        "pointer_components": component_report(facts, claims, "pointee", index),
        "nominal_components": component_report(facts, claims, "nominal", index),
        "array_components": array_report(facts, claims, index),
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


_BUILTIN_STORAGE = re.compile(
    r"(?:(?:un)?signed\s+)?(?:char|short(?:\s+int)?|int|long(?:\s+int)?)|unsigned|signed|wchar_t"
)


def _type_replacement(property_: str, value, text: str, declaration: DeclarationFact):
    """Replacement for one protected type atom, or (None, reason)."""
    if property_ == "signedness":
        # Preserve int/long spelling; don't strip existing typedefs.
        unsigned = text.startswith("unsigned ")
        base = text.removeprefix("unsigned ").removeprefix("signed ")
        if base not in {"int", "long", "long int", "short", "short int", "char"}:
            return None, "non-builtin spelling requires independent nominal recovery"
        if unsigned == (value == "unsigned") and declaration.signedness != value:
            return None, "source spelling disagrees with declaration observation"
        return (
            "unsigned " + base if value == "unsigned" else "signed char" if base == "char" else base
        ), None
    if property_ == "pointee":
        if text != "void":
            return None, "only a void pointee atom is automatically editable"
        return value, None
    if property_ == "character":
        if value == "raw-byte":
            return None, "raw-byte storage spelling requires nominal recovery"
        if not _BUILTIN_STORAGE.fullmatch(text):
            return None, "non-builtin element spelling requires nominal recovery"
        return value, None
    if declaration.domain not in {"integer", "enum"}:
        return None, "enum/nominal edit requires an integer-domain atom"
    return value, None


def _line_bounds(original: bytes, offset: int, length: int) -> tuple[int, int] | None:
    """Whole-line extent of a declaration that owns its line (plus a trailing comment)."""
    start = original.rfind(b"\n", 0, offset) + 1
    end = original.find(b"\n", offset + length)
    end = len(original) if end < 0 else end + 1
    before = original[start:offset]
    after = original[offset + length : end].strip()
    if before.strip() or (after and not after.startswith(b"//")):
        return None
    return start, end - start


def write_recovery_patch(
    facts: ScalarFacts,
    claims: list[dict],
    repository: Path,
    destination: Path,
    *,
    padding: bool = False,
) -> dict:
    """Emit reviewable whole-component patches, never mutate source files.

    Signedness at an unchanged width, existing enums/typedefs, void object
    pointees and character element types edit protected type components,
    including callback signatures. Array extents edit their literal extent and
    layout-preserving padding members are deleted. Width changes require
    further ABI recovery. All observed redeclarations and shared declaration
    atoms must agree, and every source snapshot is validated before writing.
    """
    repository = repository.resolve()
    report = integer_report(facts, claims)
    groups = []
    for component in report["integer_components"]["signedness"]:
        if (
            component["status"] == "candidate"
            and component["changes"]
            and component["value"] in {"signed", "unsigned"}
            and not any(
                claim["property"] == "width"
                and claim["key"] in component["members"]
                and claim["value"] != facts.declarations[claim["key"]].width
                for claim in claims
            )
        ):
            groups.append(
                (component["members"], component["changes"], "signedness", component["value"])
            )
    for property_, rows in (
        ("domain", report["integer_components"]["domain"]),
        ("nominal", report["nominal_components"]),
        ("pointee", report["pointer_components"]),
        ("character", report["array_components"]["character_components"]),
        ("extent", report["array_components"]["extents"]),
    ):
        for proposal in rows:
            if proposal["status"] == "candidate" and proposal["changes"]:
                value = proposal["value"]
                if property_ == "domain":
                    value = value.removeprefix("enum:")
                groups.append(
                    (
                        proposal.get("members", proposal["changes"]),
                        proposal["changes"],
                        property_,
                        value,
                    )
                )
    # Evidence-free structural cleanup is requested separately from evidence proposals.
    for row in report["structural_inventory"]["padding"] if padding else []:
        if row["status"] == "candidate":
            groups.append(([row["key"]], [row["key"]], "padding", None))
    components = {"extent": "array-extent", "padding": "field-declaration"}
    spans: dict[tuple[str, str], list[tuple]] = defaultdict(list)
    owners: dict[tuple[str, int, int], set[str]] = defaultdict(set)
    for key, component, file, offset, length, text, digest in sorted(facts.spans):
        kind = next((k for k, c in components.items() if c == component), "type")
        spans[key, kind].append((file, offset, length, text, digest))
        owners[file, offset, length].add(key)
    files: dict[str, bytes] = {}

    def source(file: str, digest: str) -> bytes:
        if file not in files:
            path = (repository / file).resolve()
            if not path.is_relative_to(repository):
                raise ValueError("source span is outside the repository")
            files[file] = path.read_bytes()
        if hashlib.sha256(files[file]).hexdigest() != digest:
            raise ValueError(f"stale source facts for {file}; recollect before editing")
        return files[file]

    proposed: dict[tuple[str, str], str] = {}
    rejected = []
    for members, changes, property_, value in groups:
        kind = property_ if property_ in components else "type"
        edits = {}
        reason = None
        for key in changes:
            declaration = facts.declarations.get(key)
            if not spans[key, kind]:
                reason = "no editable source component for every changed declaration"
                break
            replacements = set()
            for file, offset, length, text, digest in spans[key, kind]:
                if kind == "extent":
                    replacement = str(value)
                elif kind == "padding":
                    if _line_bounds(source(file, digest), offset, length) is None:
                        reason = "padding declaration shares its source line"
                        break
                    replacement = ""
                else:
                    assert declaration is not None
                    replacement, reason = _type_replacement(property_, value, text, declaration)
                    if reason:
                        break
                replacements.add(replacement)
            if reason or len(replacements) != 1:
                reason = reason or "redeclarations require different edits"
                break
            edits[key] = replacements.pop()
        if not reason:
            for key in changes:
                for file, offset, length, _, _ in spans[key, kind]:
                    if not owners[file, offset, length] <= set(changes):
                        reason = "type atom shared with an unchanged declaration"
                        break
        if reason:
            rejected.append({"members": members, "property": property_, "reason": reason})
            continue
        for key, replacement in edits.items():
            if proposed.get((key, kind), replacement) != replacement:
                raise ValueError(
                    "conflicting accepted recovery properties; split/review the evidence"
                )
            proposed[key, kind] = replacement
    file_edits: dict[str, dict[tuple[int, int], tuple[bytes, bytes]]] = defaultdict(dict)
    for (key, kind), replacement in proposed.items():
        for file, offset, length, text, digest in spans[key, kind]:
            original = source(file, digest)
            if original[offset : offset + length] != text.encode():
                raise ValueError(f"stale source facts for {file}; recollect before editing")
            if kind == "padding":
                bounds = _line_bounds(original, offset, length)
                assert bounds is not None
                offset, length = bounds
                text = original[offset : offset + length].decode()
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
        "changed_declarations": sorted({key for key, _ in proposed}),
        "edits": [{"key": key, "component": kind} for key, kind in sorted(proposed)],
        "rejected": rejected,
        "coverage": "supplied translation units; review full project/ABI coverage before applying",
    }
