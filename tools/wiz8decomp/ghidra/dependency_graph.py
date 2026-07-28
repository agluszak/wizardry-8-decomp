"""One ProgramDB-native dependency graph for candidate inference.

The graph is intentionally built from the materialized program, not the
function/vptr snapshots.  Function containment, references (including
overlay-computed calls), P-code data accesses and vptr stores, signature
callers, and atomic candidate-fact dependencies all become edges in the
same relation.  Reports may render this graph, but none owns a second copy.
"""

from __future__ import annotations

import re
from collections import defaultdict, deque
from collections.abc import Iterable
from dataclasses import dataclass, field
from typing import Any

_ADDRESS = re.compile(r"(?<![0-9a-fA-F])(?:0x)?([0-9a-fA-F]{8})(?![0-9a-fA-F])")
_UNINFORMATIVE_TYPES = {
    "bool",
    "byte",
    "char",
    "double",
    "float",
    "int",
    "long",
    "short",
    "uchar",
    "uint",
    "ulong",
    "undefined",
    "undefined1",
    "undefined2",
    "undefined4",
    "undefined8",
    "ushort",
    "void",
}


def canonical_address(value: Any) -> str:
    text = str(value).lower().removeprefix("0x")
    if ":" in text or any(character not in "0123456789abcdef" for character in text):
        return text
    return text.zfill(8)


def _type_node(data_type: Any) -> str | None:
    """A stable graph node for a non-primitive ProgramDB type."""

    if data_type is None:
        return None
    name = str(data_type.getDisplayName()).strip()
    base = name.rstrip(" *").lower()
    if not name or base in _UNINFORMATIVE_TYPES or base.startswith("undefined"):
        return None
    return f"type:{name}"


@dataclass
class DependencyGraph:
    """Reasoned dependency-to-dependent edges."""

    edges: dict[str, dict[str, set[str]]] = field(
        default_factory=lambda: defaultdict(lambda: defaultdict(set))
    )
    reverse: dict[str, dict[str, set[str]]] = field(
        default_factory=lambda: defaultdict(lambda: defaultdict(set))
    )

    def add(self, source: Any, target: Any, reason: str, site: Any | None = None) -> bool:
        source_key, target_key = canonical_address(source), canonical_address(target)
        label = reason if site is None else f"{reason}@{canonical_address(site)}"
        before = len(self.edges[source_key][target_key])
        self.edges[source_key][target_key].add(label)
        self.reverse[target_key][source_key].add(label)
        return len(self.edges[source_key][target_key]) != before

    def closure(self, seeds: Iterable[Any], limit: int = 4096) -> dict[str, Any]:
        """Directed propagation closure with explicit bounded-scope accounting."""

        queue = deque(canonical_address(seed) for seed in seeds)
        seen = set(queue)
        reasons: dict[str, set[str]] = defaultdict(set)
        truncated: set[str] = set()
        while queue:
            node = queue.popleft()
            for neighbour, labels in self.edges.get(node, {}).items():
                reasons[neighbour].update(label.split("@", 1)[0] for label in labels)
                if neighbour not in seen:
                    if len(seen) >= limit:
                        truncated.add(neighbour)
                        continue
                    seen.add(neighbour)
                    queue.append(neighbour)
        grouped: dict[str, list[str]] = defaultdict(list)
        seed_keys = {canonical_address(seed) for seed in seeds}
        for node in sorted(seen - seed_keys):
            labels = reasons.get(node) or {"dependency"}
            for label in sorted(labels):
                grouped[label].append(node)
        return {
            "groups": dict(grouped),
            "nodes": sorted(seen),
            "scope_complete": not truncated,
            "truncated_frontier": sorted(truncated),
            "limit": limit,
        }

    def cone(self, seeds: Iterable[Any], limit: int = 4096) -> dict[str, list[str]]:
        """Compatibility review view of the directed closure."""

        return self.closure(seeds, limit)["groups"]

    def serialise(self) -> dict[str, Any]:
        rows = []
        for source, targets in sorted(self.edges.items()):
            for target, reasons in sorted(targets.items()):
                rows.append({"source": source, "target": target, "reasons": sorted(reasons)})
        return {
            "nodes": len({row["source"] for row in rows} | {row["target"] for row in rows}),
            "edges": rows,
        }


def build_program_graph(program: Any, vtables: set[int] | None = None) -> DependencyGraph:
    """Read authoritative containment and references from one ProgramDB."""

    graph = DependencyGraph()
    functions = program.getFunctionManager()
    references = program.getReferenceManager()
    listing = program.getListing()
    iterator = functions.getFunctions(True)
    while iterator.hasNext():
        function = iterator.next()
        source = function.getEntryPoint()
        instructions = listing.getInstructions(function.getBody(), True)
        while instructions.hasNext():
            instruction = instructions.next()
            refs = references.getReferencesFrom(instruction.getAddress())
            for reference in refs:
                target_function = functions.getFunctionAt(reference.getToAddress())
                reference_type = reference.getReferenceType()
                if target_function is not None:
                    reason = (
                        "computed-call"
                        if reference_type.isComputed()
                        else ("direct-call" if reference_type.isCall() else "function-reference")
                    )
                    graph.add(
                        target_function.getEntryPoint(), source, reason, instruction.getAddress()
                    )
                elif program.getMemory().contains(reference.getToAddress()):
                    graph.add(
                        reference.getToAddress(), source, "data-reference", instruction.getAddress()
                    )
        # Stack objects with named types are signature/EH dependencies even
        # when the cleanup call itself is hidden behind compiler metadata.
        for variable in function.getStackFrame().getStackVariables():
            node = _type_node(variable.getDataType())
            if node is not None:
                graph.add(node, source, "stack-or-eh-object-type")
        return_node = _type_node(function.getReturnType())
        if return_node is not None:
            graph.add(return_node, source, "function-signature-type")
        for parameter in function.getParameters():
            node = _type_node(parameter.getDataType())
            if node is not None:
                graph.add(node, source, "function-signature-type")

    structures = program.getDataTypeManager().getAllStructures()
    while structures.hasNext():
        structure = structures.next()
        owner = f"type:{structure.getDisplayName()}"
        for component in structure.getDefinedComponents():
            node = _type_node(component.getDataType())
            if node is not None and node != owner:
                graph.add(node, owner, "field-or-subobject-type", component.getOffset())

    _add_candidate_dependencies(program, graph)
    if vtables:
        for table in vtables:
            address = (
                program.getAddressFactory().getDefaultAddressSpace().getAddress(f"{table:08x}")
            )
            for reference in references.getReferencesTo(address):
                owner = functions.getFunctionContaining(reference.getFromAddress())
                if owner is not None:
                    instruction = listing.getInstructionAt(reference.getFromAddress())
                    reason = (
                        "vptr-writer"
                        if _instruction_stores_value(instruction, table)
                        else "vtable-reference"
                    )
                    graph.add(
                        address,
                        owner.getEntryPoint(),
                        reason,
                        reference.getFromAddress(),
                    )
    return graph


def _instruction_stores_value(instruction: Any | None, value: int) -> bool:
    """Whether raw instruction P-code stores the specified address value."""

    if instruction is None:
        return False
    has_store = False
    for op in instruction.getPcode():
        if op.getMnemonic() != "STORE" or op.getNumInputs() < 3:
            continue
        has_store = True
        stored = op.getInput(2)
        if stored is None:
            continue
        if stored.isConstant() and int(stored.getOffset()) == value:
            return True
        if stored.isAddress() and int(stored.getAddress().getOffset()) == value:
            return True
    # The reference manager has already proved that this instruction refers
    # to the vtable address.  A raw STORE distinguishes an address installed
    # into object storage from a mere address load even when Sleigh routes the
    # immediate through a unique varnode before the store.
    return has_store


def add_pcode_dependencies(
    program: Any,
    graph: DependencyGraph,
    function: Any,
    vtables: set[int] | None = None,
) -> int:
    """Enrich one function's node from High P-code facts."""

    from .semantic import _address_expression, _high_function

    source = function.getEntryPoint()
    high = _high_function(program, function, "normalize")
    added = 0
    iterator = high.getPcodeOps()
    while iterator.hasNext():
        op = iterator.next()
        mnemonic = op.getMnemonic()
        site = op.getSeqnum().getTarget()
        if mnemonic == "CALL" and op.getNumInputs():
            target = op.getInput(0)
            if target is not None and target.isAddress():
                target_function = program.getFunctionManager().getFunctionAt(target.getAddress())
                if target_function is not None:
                    added += graph.add(target_function.getEntryPoint(), source, "pcode-call", site)
        elif mnemonic in {"LOAD", "STORE"}:
            pointer_index = 1
            pointer = op.getInput(pointer_index) if op.getNumInputs() > pointer_index else None
            resolved = _address_expression(pointer)
            if resolved["kind"] == "absolute":
                added += graph.add(
                    f"{int(resolved['address']):08x}", source, "pcode-data-access", site
                )
            if mnemonic == "STORE" and vtables and op.getNumInputs() > 2:
                value = op.getInput(2)
                if value is not None and value.isConstant() and int(value.getOffset()) in vtables:
                    added += graph.add(f"{int(value.getOffset()):08x}", source, "vptr-write", site)
    return added


def _add_candidate_dependencies(program: Any, graph: DependencyGraph) -> None:
    from .candidate_facts import iter_facts

    functions = program.getFunctionManager()
    for address, _fact_id, record in iter_facts(program):
        owner = functions.getFunctionContaining(address)
        source = owner.getEntryPoint() if owner is not None else address
        for dependency in record.get("depends_on", []):
            for match in _ADDRESS.finditer(str(dependency)):
                graph.add(match.group(1), source, "candidate-dependency", address)
