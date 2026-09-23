"""Structured HighFunction and P-code queries over a persistent decompiler.

The plain `decompile` query answers "what does this function look like as C".
Recovery needs a different question - "what does this function *do* to the
objects it touches" - and answering it from C text means re-parsing prose the
decompiler already had in structured form. These queries expose that form:
the prototype and storage of every high variable, the P-code stream itself,
the accesses reachable from a chosen parameter root, and the shape of one
call site.

The decompiler is a service, not a subprocess-per-question: sessions are
DecompInterface instances configured once per (program, style) and reused for
one batch session. Three styles matter here - `decompile` for typed
analysis, `normalize` for a stable data-flow form without type-recovery noise,
and `paramid` for parameter measurement - and the extraction styles keep the
syntax tree while skipping C generation entirely.

Read-only by construction: nothing here mutates the program, so the module is
read-only with respect to the reviewed project.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

_TIMEOUT_SECONDS = 120
# One trace step per varnode-op edge; a function that legitimately exceeds
# this is beyond what one query should return anyway.
_TRACE_LIMIT = 20000
_AFFINE_TERM_LIMIT = 8
_RANGE_GUARD_LIMIT = 16
_INCOMPLETENESS_LIMIT = 256
_PHI_INPUT_LIMIT = 8
_STYLES = ("decompile", "normalize", "paramid")
# Named option profiles for enrichment vs recovery export. ``program`` keeps
# the program's saved decompiler options (historical default).
_PROFILES = ("program", "analysis", "recovery")


@dataclass
class _AffineAddress:
    """A root-relative address with SSA-identified affine index terms."""

    constant: int = 0
    terms: dict[tuple[Any, ...], tuple[int, Any]] = field(default_factory=dict)

    def copy(self) -> _AffineAddress:
        return _AffineAddress(self.constant, dict(self.terms))


def _session(
    program: Any,
    style: str,
    *,
    c_output: bool,
    profile: str = "analysis",
    session: Any | None = None,
) -> Any:
    """Decompiler interface for this command-local session."""

    from .inspect import DecompileSession

    if style not in _STYLES:
        raise ValueError(f"unknown decompiler style: {style}")
    if profile not in _PROFILES:
        raise ValueError(f"unknown decompiler profile: {profile}")
    active = session or DecompileSession(program, profile=profile)
    return active.interface(style, c_output=c_output)


def _decompile_result(
    program: Any,
    function: Any,
    style: str,
    *,
    c_output: bool,
    profile: str = "analysis",
    session: Any | None = None,
) -> Any:
    from .inspect import DecompileSession

    active = session or DecompileSession(program, profile=profile)
    close = session is None
    try:
        return active.decompile(function, style, c_output=c_output)
    finally:
        if close:
            active.close()


def _node_key(node: Any) -> tuple[Any, ...] | None:
    """Stable identity for a varnode across JPype wrapper instances."""

    if node is None:
        return None
    definition = node.getDef()
    return (
        node.getAddress().getAddressSpace().getName(),
        node.getOffset(),
        node.getSize(),
        str(definition.getSeqnum()) if definition is not None else None,
    )


def _high_function(
    program: Any,
    function: Any,
    style: str = "decompile",
    *,
    profile: str = "analysis",
    session: Any | None = None,
) -> Any:
    result = _decompile_result(
        program, function, style, c_output=False, profile=profile, session=session
    )
    high = result.getHighFunction() if result is not None else None
    if high is None:
        error = result.getErrorMessage() if result is not None else "no result"
        raise RuntimeError(
            f"no high function for {function.getEntryPoint()}: {error or 'decompilation failed'}"
        )
    return high


def decompile_c(
    program: Any, function: Any, *, profile: str = "analysis", session: Any | None = None
) -> dict[str, Any]:
    """Render C through a command-local decompiler session."""

    result = _decompile_result(
        program, function, "decompile", c_output=True, profile=profile, session=session
    )
    completed = bool(result is not None and result.decompileCompleted())
    rendered = result.getDecompiledFunction() if completed else None
    return {
        "completed": completed,
        "error": result.getErrorMessage() if result is not None else "no result",
        "decompiled": rendered.getC() if rendered is not None else None,
        "profile": profile,
    }


def _varnode(node: Any) -> dict[str, Any] | None:
    """One varnode as a stable, self-describing value.

    A unique (temporary) varnode's offset is only meaningful together with the
    defining operation, so its identity includes the definition site; Ghidra's
    own dynamic hash exists for the same reason.
    """

    if node is None:
        return None
    space = node.getAddress().getAddressSpace().getName()
    value: dict[str, Any] = {
        "space": space,
        "offset": f"0x{node.getOffset():x}",
        "size": node.getSize(),
    }
    if node.isConstant():
        value["constant"] = node.getOffset()
    definition = node.getDef()
    if definition is not None and space == "unique":
        value["defined_at"] = str(definition.getSeqnum().getTarget())
        value["defined_order"] = definition.getSeqnum().getTime()
    high = node.getHigh()
    if high is not None:
        symbol = high.getSymbol()
        if symbol is not None:
            value["high"] = symbol.getName()
        data_type = high.getDataType()
        if data_type is not None:
            value["type"] = data_type.getDisplayName()
            value["type_origin"] = "model-derived HighFunction type"
    return value


def _symbol_entry(symbol: Any) -> dict[str, Any]:
    storage = symbol.getStorage()
    return {
        "name": symbol.getName(),
        "type": symbol.getDataType().getDisplayName() if symbol.getDataType() else None,
        "storage": str(storage) if storage is not None else None,
        "parameter": bool(symbol.isParameter()),
        "category": symbol.getCategoryIndex(),
        "pc_address": str(symbol.getPCAddress()) if symbol.getPCAddress() else None,
    }


def high_function(program: Any, argument: str) -> dict[str, Any]:
    """Prototype, parameters, locals and high variables of one function."""

    from .inspect import function_facts
    from .resolve import resolve_function

    function = resolve_function(program, argument)
    high = _high_function(program, function)
    prototype = high.getFunctionPrototype()
    parameters = [
        _symbol_entry(prototype.getParam(index)) for index in range(prototype.getNumParams())
    ]
    locals_ = []
    symbols = high.getLocalSymbolMap().getSymbols()
    while symbols.hasNext():
        symbol = symbols.next()
        if not symbol.isParameter():
            locals_.append(_symbol_entry(symbol))
    return {
        "function": function_facts(program, function),
        "return_type": (
            prototype.getReturnType().getDisplayName() if prototype.getReturnType() else None
        ),
        "calling_convention": str(prototype.getModelName()),
        "parameters": parameters,
        "locals": sorted(locals_, key=lambda item: item["name"]),
    }


def pcode(program: Any, argument: str, style: str = "decompile") -> dict[str, Any]:
    """The function's P-code stream in execution order, one entry per op."""

    from .resolve import resolve_function

    function = resolve_function(program, argument)
    high = _high_function(program, function, style)
    return _pcode_document(function, high, style)


def _pcode_document(function: Any, high: Any, style: str) -> dict[str, Any]:
    """Serialize one already-live HighFunction for the public query boundary."""

    operations = []
    iterator = high.getPcodeOps()
    while iterator.hasNext():
        op = iterator.next()
        operations.append(
            {
                "address": str(op.getSeqnum().getTarget()),
                "order": op.getSeqnum().getTime(),
                "op": op.getMnemonic(),
                "output": _varnode(op.getOutput()),
                "inputs": [_varnode(op.getInput(index)) for index in range(op.getNumInputs())],
            }
        )
    blocks = [
        {
            "index": block.getIndex(),
            "start": str(block.getStart()),
            "stop": str(block.getStop()),
            "out": [block.getOut(index).getIndex() for index in range(block.getOutSize())],
        }
        for block in high.getBasicBlocks()
    ]
    return {
        "entry": str(function.getEntryPoint()),
        "style": style,
        "blocks": blocks,
        "operations": operations,
    }


def _resolve_root(high: Any, root: str) -> Any:
    """The HighSymbol a root name selects: `this`, a name, or an index."""

    prototype = high.getFunctionPrototype()
    count = prototype.getNumParams()
    if not count:
        raise ValueError("function has no parameters to root at")
    if root in {"this", "0"} or root.isdigit():
        index = 0 if root == "this" else int(root)
        if index >= count:
            raise ValueError(f"parameter index {index} out of range ({count} parameters)")
        return prototype.getParam(index)
    for index in range(count):
        symbol = prototype.getParam(index)
        if symbol.getName() == root:
            return symbol
    raise ValueError(f"no parameter named {root}")


def _instances(symbol: Any) -> list[Any]:
    high = symbol.getHighVariable()
    if high is None:
        return []
    return list(high.getInstances())


def _signed_integer(node: Any, pointer_bits: int) -> int:
    """Interpret a P-code integer constant at its own width, then pointer width."""

    width = max(1, int(node.getSize()) * 8)
    raw = int(node.getOffset()) & ((1 << width) - 1)
    if raw & (1 << (width - 1)):
        raw -= 1 << width
    return _normalize_integer(raw, pointer_bits)


def _normalize_integer(value: int, pointer_bits: int) -> int:
    mask = (1 << pointer_bits) - 1
    normalized = int(value) & mask
    if normalized & (1 << (pointer_bits - 1)):
        normalized -= 1 << pointer_bits
    return normalized


def _merge_affine(left: _AffineAddress, right: _AffineAddress, *, sign: int, bits: int) -> None:
    left.constant = _normalize_integer(left.constant + sign * right.constant, bits)
    for identity, (coefficient, node) in right.terms.items():
        previous = left.terms.get(identity)
        updated = _normalize_integer(
            (previous[0] if previous is not None else 0) + sign * coefficient,
            bits,
        )
        if updated:
            left.terms[identity] = (updated, node)
        else:
            left.terms.pop(identity, None)


def _scale_affine(expression: _AffineAddress, scale: int, bits: int) -> _AffineAddress:
    result = _AffineAddress(_normalize_integer(expression.constant * scale, bits))
    for identity, (coefficient, node) in expression.terms.items():
        updated = _normalize_integer(coefficient * scale, bits)
        if updated:
            result.terms[identity] = (updated, node)
    return result


def _integer_expression(
    node: Any, pointer_bits: int, active: set[tuple[Any, ...]] | None = None
) -> _AffineAddress | None:
    """Describe linear integer P-code as a constant plus SSA-identity terms."""

    if node is None:
        return None
    if node.isConstant():
        return _AffineAddress(_signed_integer(node, pointer_bits))
    marker = _node_key(node)
    if marker is None:
        return None
    active = set() if active is None else active
    if marker in active or len(active) >= 64:
        return None
    active.add(marker)
    try:
        definition = node.getDef()
        if definition is None:
            return _AffineAddress(terms={marker: (1, node)})
        mnemonic = definition.getMnemonic()
        count = definition.getNumInputs()
        if mnemonic in {"COPY", "CAST", "INT_ZEXT", "INT_SEXT"} and count:
            return _integer_expression(definition.getInput(0), pointer_bits, active)
        if mnemonic in {"INT_ADD", "INT_SUB"} and count >= 2:
            left = _integer_expression(definition.getInput(0), pointer_bits, active)
            right = _integer_expression(definition.getInput(1), pointer_bits, active)
            if left is None or right is None:
                return None
            _merge_affine(
                left,
                right,
                sign=-1 if mnemonic == "INT_SUB" else 1,
                bits=pointer_bits,
            )
            return left if len(left.terms) <= _AFFINE_TERM_LIMIT else None
        if mnemonic == "INT_MULT" and count >= 2:
            left_node, right_node = definition.getInput(0), definition.getInput(1)
            if left_node.isConstant():
                expression, scale_node = (
                    _integer_expression(right_node, pointer_bits, active),
                    left_node,
                )
            elif right_node.isConstant():
                expression, scale_node = (
                    _integer_expression(left_node, pointer_bits, active),
                    right_node,
                )
            else:
                return None
            if expression is None:
                return None
            scaled = _scale_affine(
                expression, _signed_integer(scale_node, pointer_bits), pointer_bits
            )
            return scaled if len(scaled.terms) <= _AFFINE_TERM_LIMIT else None
        if mnemonic == "INT_LEFT" and count >= 2:
            shift = definition.getInput(1)
            if not shift.isConstant():
                return None
            count_bits = _signed_integer(shift, pointer_bits)
            if count_bits < 0 or count_bits >= pointer_bits:
                return None
            expression = _integer_expression(definition.getInput(0), pointer_bits, active)
            if expression is None:
                return None
            scaled = _scale_affine(expression, 1 << count_bits, pointer_bits)
            return scaled if len(scaled.terms) <= _AFFINE_TERM_LIMIT else None
        if mnemonic == "INT_2COMP" and count:
            expression = _integer_expression(definition.getInput(0), pointer_bits, active)
            if expression is None:
                return None
            scaled = _scale_affine(expression, -1, pointer_bits)
            return scaled if len(scaled.terms) <= _AFFINE_TERM_LIMIT else None
        # Any other scalar value is still a valid opaque SSA index. Its
        # identity is kept instead of inventing a value or a source variable.
        return _AffineAddress(terms={marker: (1, node)})
    finally:
        active.discard(marker)


def _observed_range_guards(node: Any) -> tuple[list[dict[str, Any]], bool]:
    """List direct constant comparisons of an index; do not infer CFG dominance."""

    guards = []
    comparisons = {
        "INT_EQUAL",
        "INT_NOTEQUAL",
        "INT_LESS",
        "INT_LESSEQUAL",
        "INT_SLESS",
        "INT_SLESSEQUAL",
    }
    descendants = node.getDescendants()
    while descendants.hasNext():
        op = descendants.next()
        mnemonic = op.getMnemonic()
        if mnemonic not in comparisons or op.getNumInputs() < 2:
            continue
        left, right = op.getInput(0), op.getInput(1)
        if _same_varnode(node, left) and right is not None and right.isConstant():
            side, bound = "left", right
        elif _same_varnode(node, right) and left is not None and left.isConstant():
            side, bound = "right", left
        else:
            continue
        guards.append(
            {
                "site": str(op.getSeqnum().getTarget()),
                "predicate": mnemonic,
                "index_side": side,
                "constant": _signed_integer(bound, max(8, int(node.getSize()) * 8)),
                "control_relation": "comparison observed; branch dominance not established",
            }
        )
        if len(guards) > _RANGE_GUARD_LIMIT:
            guards.pop()
            return guards, True
    guards.sort(key=lambda item: (item["site"], item["predicate"]))
    return guards, False


def _same_varnode(left: Any, right: Any) -> bool:
    return left is not None and right is not None and bool(left.equals(right))


def _model_marks_pointer(node: Any) -> bool:
    high = node.getHigh()
    if high is None:
        return False
    data_type = high.getDataType()
    if data_type is None:
        return False
    is_pointer = getattr(data_type, "isPointer", None)
    if is_pointer is not None and bool(is_pointer()):
        return True
    display_name = data_type.getDisplayName()
    return bool(display_name and display_name.rstrip().endswith("*"))


def _used_as_address(node: Any) -> bool:
    """Follow only address-producing copies/arithmetic to a real memory access."""

    pending = [node]
    visited: set[tuple[Any, ...]] = set()
    while pending:
        current = pending.pop()
        marker = _node_key(current)
        if marker is None or marker in visited:
            continue
        visited.add(marker)
        descendants = current.getDescendants()
        while descendants.hasNext():
            op = descendants.next()
            mnemonic = op.getMnemonic()
            output = op.getOutput()
            if mnemonic == "LOAD" and _same_varnode(current, op.getInput(1)):
                return True
            if mnemonic == "STORE" and _same_varnode(current, op.getInput(1)):
                return True
            if mnemonic in {"CALL", "CALLIND"} and any(
                _same_varnode(current, op.getInput(index)) for index in range(1, op.getNumInputs())
            ):
                return True
            if mnemonic == "CALLIND" and _same_varnode(current, op.getInput(0)):
                return True
            if output is None:
                continue
            if mnemonic in {"COPY", "CAST", "MULTIEQUAL"}:
                if any(
                    _same_varnode(current, op.getInput(index)) for index in range(op.getNumInputs())
                ):
                    pending.append(output)
            elif (
                mnemonic == "INT_ADD"
                and any(
                    _same_varnode(current, op.getInput(index)) for index in range(op.getNumInputs())
                )
            ) or (
                mnemonic in {"INT_SUB", "PTRSUB", "PTRADD"}
                and _same_varnode(current, op.getInput(0))
            ):
                pending.append(output)
    return False


def _affine_document(
    expression: _AffineAddress, *, root_identity: str, pointer_bits: int
) -> dict[str, Any]:
    terms = []
    for identity, (stride, node) in sorted(
        expression.terms.items(), key=lambda item: repr(item[0])
    ):
        index_varnode = _varnode(node)
        if index_varnode is not None and "type" in index_varnode:
            index_varnode["type_origin"] = "model-derived HighFunction type"
        guards, guards_truncated = _observed_range_guards(node)
        terms.append(
            {
                "index": {
                    "identity": [str(value) for value in identity],
                    "varnode": index_varnode,
                },
                "stride": stride,
                "range_guards": guards,
                "range_guards_truncated": guards_truncated,
                "range_guard_limit": _RANGE_GUARD_LIMIT,
            }
        )
    constant = _normalize_integer(expression.constant, pointer_bits)
    pieces = [root_identity]
    if constant:
        pieces.append(f"{constant:+#x}")
    for term in terms:
        stride = term["stride"]
        sign = "+" if stride >= 0 else "-"
        pieces.append(f"{sign}{abs(stride)}*ssa:{term['index']['identity']}")
    return {
        "root": root_identity,
        "constant": constant,
        "terms": terms,
        "term_limit": _AFFINE_TERM_LIMIT,
        "text": " ".join(pieces),
        "width_bits": pointer_bits,
        "interpretation": "effective-address constraint; not evidence of an authored array base",
        "type_origin": "P-code-derived; index identities are SSA values",
    }


def _walk_value_flow(
    instances: list[Any],
    root: str,
    *,
    on_node: Any,
    on_operation: Any,
    on_incomplete: Any | None = None,
    follow_loads: bool,
    root_identity: str | None = None,
    pointer_size: int | None = None,
) -> None:
    """One rooted SSA walker shared by fields, receivers and value paths."""

    steps = 0
    limited = False
    root_identity = root_identity or root
    if pointer_size is None:
        pointer_size = int(instances[0].getSize()) if instances else 4
    pointer_bits = max(8, pointer_size * 8)

    def same(left: Any, right: Any) -> bool:
        return _same_varnode(left, right)

    def stop(
        kind: str,
        op: Any | None,
        node: Any | None,
        path: str,
        expression: _AffineAddress,
        depth: int,
        reason: str,
        **details: Any,
    ) -> None:
        if on_incomplete is None:
            return
        on_incomplete(
            {
                "kind": kind,
                "site": str(op.getSeqnum().getTarget()) if op is not None else None,
                "path": path,
                "depth": depth,
                "reason": reason,
                "expression": _affine_document(
                    expression, root_identity=root_identity, pointer_bits=pointer_bits
                ),
                "node": _varnode(node),
                **details,
            }
        )

    def trace(
        node: Any,
        expression: _AffineAddress,
        path: str,
        depth: int,
        provenance: tuple[str, ...],
        visited: set[Any],
    ) -> None:
        nonlocal limited
        if limited:
            return
        nonlocal steps
        marker = _node_key(node)
        if marker is None or marker in visited:
            return
        visited.add(marker)
        on_node(node, marker, path, expression, depth, provenance)
        descendants = node.getDescendants()
        while descendants.hasNext():
            steps += 1
            if steps > _TRACE_LIMIT:
                limited = True
                stop(
                    "step_limit",
                    descendants.next(),
                    node,
                    path,
                    expression,
                    depth,
                    f"value-flow trace exceeded {_TRACE_LIMIT} steps",
                    limit=_TRACE_LIMIT,
                )
                return
            op = descendants.next()
            mnemonic = op.getMnemonic()
            output = op.getOutput()
            site = str(op.getSeqnum().getTarget())
            next_provenance = (*provenance, f"{mnemonic}@{site}#{op.getSeqnum().getTime()}")
            on_operation(node, op, path, expression, depth, provenance, same)
            if mnemonic in {"COPY", "CAST"}:
                if output is not None:
                    trace(output, expression.copy(), path, depth, next_provenance, visited)
            elif mnemonic == "MULTIEQUAL":
                # A phi is value-preserving only when every incoming value is
                # the same SSA value. Distinct inputs require a join-aware
                # proof; choosing whichever root path reaches the phi first
                # turns control-flow ambiguity into a false field offset.
                inputs = [op.getInput(index) for index in range(op.getNumInputs())]
                if output is not None and all(same(node, item) for item in inputs):
                    trace(output, expression.copy(), path, depth, next_provenance, visited)
                elif output is not None:
                    stop(
                        "ambiguous_join",
                        op,
                        node,
                        path,
                        expression,
                        depth,
                        "MULTIEQUAL inputs are not all the same SSA value; no incoming address was selected",
                        inputs=[_varnode(item) for item in inputs[:_PHI_INPUT_LIMIT]],
                        input_count=len(inputs),
                        inputs_truncated=len(inputs) > _PHI_INPUT_LIMIT,
                    )
            elif mnemonic in {"INT_ADD", "INT_SUB", "PTRSUB", "PTRADD"}:
                derived = _derive_pointer_expression(node, op, expression, pointer_bits, same)
                if derived is not None and output is not None:
                    trace(output, derived, path, depth, next_provenance, visited)
                elif output is not None:
                    if mnemonic == "PTRADD" and same(node, op.getInput(0)):
                        index = op.getInput(1)
                        scale = op.getInput(2)
                        stop(
                            "symbolic_index",
                            op,
                            node,
                            path,
                            expression,
                            depth,
                            "PTRADD index or stride is not expressible as a bounded constant-stride SSA affine term",
                            index=_varnode(index),
                            stride=_varnode(scale),
                        )
                    stop(
                        "unsupported_arithmetic",
                        op,
                        node,
                        path,
                        expression,
                        depth,
                        f"{mnemonic} operands do not establish a supported root-relative affine address",
                        operation=mnemonic,
                        inputs=[_varnode(op.getInput(index)) for index in range(op.getNumInputs())],
                    )
            elif mnemonic in {"INT_MULT", "INT_LEFT", "INT_2COMP", "INT_AND", "INT_OR", "INT_XOR"}:
                stop(
                    "unsupported_arithmetic",
                    op,
                    node,
                    path,
                    expression,
                    depth,
                    f"{mnemonic} consumes a root-derived pointer value outside supported address forms",
                    operation=mnemonic,
                    inputs=[_varnode(op.getInput(index)) for index in range(op.getNumInputs())],
                )
            elif (
                follow_loads
                and mnemonic == "LOAD"
                and same(node, op.getInput(1))
                and output is not None
            ):
                pointer_typed = _model_marks_pointer(output)
                address_used = _used_as_address(output)
                if not pointer_typed and not address_used:
                    continue
                if output.getSize() != pointer_size:
                    stop(
                        "unsupported_loaded_pointer",
                        op,
                        node,
                        path,
                        expression,
                        depth,
                        "loaded value is used as an address but its width does not match the target pointer width",
                        load_width=int(output.getSize()),
                        pointer_width=pointer_size,
                        type_origin=(
                            "model-derived pointer type"
                            if pointer_typed
                            else "P-code address-use evidence"
                        ),
                    )
                elif depth >= 3:
                    stop(
                        "depth_limit",
                        op,
                        node,
                        path,
                        expression,
                        depth,
                        "loaded-pointer traversal reached the configured depth limit",
                        limit=3,
                    )
                else:
                    address = _affine_document(
                        expression, root_identity=root_identity, pointer_bits=pointer_bits
                    )
                    if expression.terms:
                        child_path = f"{path}[{address['text']}]"
                    else:
                        child_path = f"{path}[{expression.constant:#x}]"
                    trace(
                        output,
                        _AffineAddress(),
                        child_path,
                        depth + 1,
                        next_provenance,
                        visited,
                    )

    for instance in instances:
        trace(instance, _AffineAddress(), root, 0, (), set())


def _derive_pointer_expression(
    node: Any,
    op: Any,
    expression: _AffineAddress,
    pointer_bits: int,
    same: Any,
) -> _AffineAddress | None:
    mnemonic = op.getMnemonic()
    count = op.getNumInputs()
    if mnemonic == "PTRADD":
        if count < 3 or not same(node, op.getInput(0)):
            return None
        index = _integer_expression(op.getInput(1), pointer_bits)
        scale_node = op.getInput(2)
        if index is None or not scale_node.isConstant():
            return None
        scale = _signed_integer(scale_node, pointer_bits)
        result = expression.copy()
        _merge_affine(result, _scale_affine(index, scale, pointer_bits), sign=1, bits=pointer_bits)
        return result if len(result.terms) <= _AFFINE_TERM_LIMIT else None

    if count < 2:
        return None
    left, right = op.getInput(0), op.getInput(1)
    if same(node, left):
        other = _integer_expression(right, pointer_bits)
        sign = -1 if mnemonic in {"INT_SUB", "PTRSUB"} else 1
    elif mnemonic == "INT_ADD" and same(node, right):
        other = _integer_expression(left, pointer_bits)
        sign = 1
    else:
        return None
    if other is None:
        return None
    result = expression.copy()
    _merge_affine(result, other, sign=sign, bits=pointer_bits)
    return result if len(result.terms) <= _AFFINE_TERM_LIMIT else None


def trace_accesses(
    instances: list[Any],
    root: str,
    *,
    root_identity: str | None = None,
    pointer_size: int | None = None,
    incompleteness: list[dict[str, Any]] | None = None,
) -> list[dict[str, Any]]:
    """Every access reachable from the given root varnodes, with derived offsets.

    The trace follows copies and bounded affine pointer arithmetic. Each access
    retains a normalized root-relative expression, width, and instruction site;
    unresolved joins and unsupported operations are returned through
    ``incompleteness``. Loads spawn further levels only when their result is
    model-marked as a pointer or is structurally used as a memory address.

    Everything here is duck-typed against the varnode and P-code op surface, so
    the traversal is unit-testable with fakes; the JPype boundary is exactly
    where a Python identity check silently broke once already.
    """

    accesses: list[dict[str, Any]] = []
    stops = incompleteness if incompleteness is not None else []
    if not instances:
        stops.append(
            {
                "kind": "missing_root",
                "site": None,
                "reason": "selected parameter has no HighVariable SSA instances",
            }
        )
        return accesses
    active_root_identity = root_identity or root
    active_pointer_size = pointer_size or int(instances[0].getSize())
    pointer_bits = max(8, active_pointer_size * 8)
    stop_index = {(stop.get("kind"), stop.get("site"), stop.get("path")): stop for stop in stops}

    def record_stop(stop: dict[str, Any]) -> None:
        key = (stop.get("kind"), stop.get("site"), stop.get("path"))
        previous = stop_index.get(key)
        if previous is None:
            if len(stops) >= _INCOMPLETENESS_LIMIT - 1:
                limit_record = next(
                    (item for item in stops if item.get("kind") == "stop_record_limit"), None
                )
                if limit_record is None:
                    stops.append(
                        {
                            "kind": "stop_record_limit",
                            "site": None,
                            "reason": "additional incompleteness records were omitted",
                            "limit": _INCOMPLETENESS_LIMIT,
                            "omitted": 1,
                        }
                    )
                else:
                    limit_record["omitted"] += 1
                return
            if stop.get("kind") == "ambiguous_join":
                stop["expressions"] = [stop["expression"]]
                stop["input_sets"] = [stop.get("inputs", [])]
                stop["alternatives_truncated"] = False
            stops.append(stop)
            stop_index[key] = stop
            return
        if (
            stop.get("kind") == "ambiguous_join"
            and stop["expression"] not in previous["expressions"]
        ):
            if len(previous["expressions"]) < _PHI_INPUT_LIMIT:
                previous["expressions"].append(stop["expression"])
                previous["input_sets"].append(stop.get("inputs", []))
            else:
                previous["alternatives_truncated"] = True

    def record(
        kind: str,
        op: Any,
        path: str,
        expression: _AffineAddress,
        provenance: tuple[str, ...],
        **extra: Any,
    ) -> None:
        effective_address = _affine_document(
            expression,
            root_identity=active_root_identity,
            pointer_bits=pointer_bits,
        )
        value = extra.get("value")
        if value is not None and "type" in value:
            value["type_origin"] = "model-derived HighFunction type"
        accesses.append(
            {
                "kind": kind,
                "site": str(op.getSeqnum().getTarget()),
                "path": path,
                "offset": (
                    (
                        f"-0x{-expression.constant:x}"
                        if expression.constant < 0
                        else f"0x{expression.constant:x}"
                    )
                    if not expression.terms
                    else None
                ),
                "effective_address": effective_address,
                "width": extra.pop("width", None),
                "provenance": list(provenance),
                **extra,
            }
        )

    def consume(
        node: Any,
        op: Any,
        path: str,
        expression: _AffineAddress,
        _depth: int,
        provenance: tuple[str, ...],
        same: Any,
    ) -> None:
        mnemonic = op.getMnemonic()
        output = op.getOutput()
        if mnemonic == "LOAD" and same(node, op.getInput(1)) and output is not None:
            record("load", op, path, expression, provenance, width=output.getSize())
        elif mnemonic == "STORE":
            if same(node, op.getInput(1)):
                value = op.getInput(2)
                record(
                    "store",
                    op,
                    path,
                    expression,
                    provenance,
                    width=value.getSize() if value is not None else None,
                    value=_varnode(value),
                )
            elif same(node, op.getInput(2)):
                record("stored-elsewhere", op, path, expression, provenance)
        elif mnemonic in {"CALL", "CALLIND"}:
            target = op.getInput(0)
            positions = [
                index - 1 for index in range(1, op.getNumInputs()) if same(node, op.getInput(index))
            ]
            if mnemonic == "CALLIND" and same(node, target):
                record(
                    "indirect-call-target",
                    op,
                    path,
                    expression,
                    provenance,
                    arguments=[_varnode(op.getInput(i)) for i in range(1, op.getNumInputs())],
                )
            for position in positions:
                record(
                    "call-arg" if mnemonic == "CALL" else "indirect-call-arg",
                    op,
                    path,
                    expression,
                    provenance,
                    argument=position,
                    target=(
                        str(target.getAddress())
                        if mnemonic == "CALL" and target is not None and target.isAddress()
                        else _varnode(target)
                    ),
                    arguments=[_varnode(op.getInput(i)) for i in range(1, op.getNumInputs())],
                )
        elif mnemonic in {"INT_EQUAL", "INT_NOTEQUAL"}:
            other = op.getInput(1) if same(node, op.getInput(0)) else op.getInput(0)
            if other is not None and other.isConstant() and other.getOffset() == 0:
                record(
                    "null-test",
                    op,
                    path,
                    expression,
                    provenance,
                    negated=mnemonic == "INT_NOTEQUAL",
                )
        elif mnemonic == "RETURN":
            record("returned", op, path, expression, provenance)

    _walk_value_flow(
        instances,
        root,
        on_node=lambda *_args: None,
        on_operation=consume,
        on_incomplete=record_stop,
        follow_loads=True,
        root_identity=active_root_identity,
        pointer_size=active_pointer_size,
    )
    accesses.sort(
        key=lambda item: (
            item["path"],
            item["effective_address"]["constant"],
            repr(item["effective_address"]["terms"]),
            item["site"],
        )
    )
    return accesses


def trace_value_paths(
    instances: list[Any], root: str
) -> dict[tuple[Any, ...], tuple[str, int | None]]:
    """Map every root-derived varnode to its semantic path and constant offset.

    This is the data-flow witness needed for implicit ``this`` calls.  VC6
    passes a receiver in ECX, but a High CALL with an unresolved prototype has
    no receiver input.  The COPY into ECX immediately before that CALL still
    has a normal SSA definition, so retaining the root path of every derived
    value lets callers prove which member reached the implicit receiver.
    """

    paths: dict[tuple[Any, ...], tuple[str, int]] = {}

    def remember(
        _node: Any,
        marker: tuple[Any, ...],
        path: str,
        expression: _AffineAddress,
        _depth: int,
        _provenance: tuple[str, ...],
    ) -> None:
        paths[marker] = (path, expression.constant if not expression.terms else None)

    _walk_value_flow(
        instances,
        root,
        on_node=remember,
        on_operation=lambda *_args: None,
        follow_loads=True,
    )
    return paths


def field_accesses(
    program: Any,
    argument: str,
    root: str,
    *,
    profile: str = "analysis",
) -> dict[str, Any]:
    """`trace_accesses` for one function parameter, plus the call table.

    The call table lists every CALL and CALLIND in flow order with block
    indexes: a receiver passed through ECX to an unknown-prototype callee never
    appears among that CALL's inputs, so the downstream object rules correlate
    a member's null test with the calls of the guarded successor block instead.
    """

    from .resolve import resolve_function

    function = resolve_function(program, argument)
    high = _high_function(program, function, profile=profile)
    entry = str(function.getEntryPoint())
    stops: list[dict[str, Any]] = []
    try:
        symbol = _resolve_root(high, root)
    except ValueError as error:
        if root.startswith("global:"):
            root_kind = "global"
            reason = "global roots are not implemented by this parameter-rooted flow query"
            stops.append(
                {
                    "kind": "unsupported_root_kind",
                    "site": None,
                    "reason": reason,
                    "requested": root,
                }
            )
        elif root.startswith("adjusted-receiver:"):
            root_kind = "adjusted_receiver"
            reason = (
                "adjusted-receiver roots are not implemented by this parameter-rooted flow query"
            )
            stops.append(
                {
                    "kind": "unsupported_root_kind",
                    "site": None,
                    "reason": reason,
                    "requested": root,
                }
            )
        else:
            root_kind = "parameter"
            reason = str(error)
        stops.append(
            {
                "kind": "missing_root",
                "site": None,
                "reason": reason,
                "requested": root,
            }
        )
        if root_kind == "parameter":
            stops.append(
                {
                    "kind": "prototype_dependent_input_omission",
                    "site": None,
                    "reason": (
                        "the selected HighFunction prototype does not expose this requested root; "
                        "a machine input omitted from or mis-modeled by that prototype cannot be traced"
                    ),
                    "requested": root,
                }
            )
        return {
            "program": _flow_program_identity(program),
            "entry": entry,
            "function": {"entry": entry, "name": str(function.getName())},
            "profile": _flow_profile_identity(profile),
            "root": {
                "requested": root,
                "kind": root_kind,
                "identity": f"{entry}:{root}",
                "storage": None,
                "type_origin": "model-derived HighFunction prototype",
            },
            "accesses": [],
            "calls": [],
            "completeness": _flow_completeness(stops, []),
        }
    instances = _instances(symbol)
    symbol_record = _symbol_entry(symbol)
    root_name = symbol.getName()
    root_kind = "parameter"
    root_role = "receiver" if root == "this" or root_name == "this" else "argument"
    root_identity = f"{entry}:{root_kind}:{root_name}:{symbol_record['storage']}"
    accesses = trace_accesses(
        instances,
        root_name,
        root_identity=root_identity,
        pointer_size=int(program.getDefaultPointerSize()),
        incompleteness=stops,
    )
    calls = []
    receiver_by_site = _implicit_receiver_paths(program, function, accesses)

    iterator = high.getPcodeOps()
    while iterator.hasNext():
        op = iterator.next()
        block = op.getParent().getIndex() if op.getParent() is not None else None
        if op.getMnemonic() not in {"CALL", "CALLIND"}:
            continue
        target = op.getInput(0)
        call = {
            "op": op.getMnemonic(),
            "site": str(op.getSeqnum().getTarget()),
            "order": op.getSeqnum().getTime(),
            "block": block,
            "target": (
                str(target.getAddress())
                if target is not None and target.isAddress()
                else _varnode(target)
            ),
        }
        receiver = receiver_by_site.get(str(op.getSeqnum().getTarget()))
        if receiver is not None:
            call["receiver_path"] = receiver[0]
            call["receiver_offset"] = f"0x{receiver[1]:x}"
            call["receiver_source"] = receiver[2]
        calls.append(call)
    return {
        "program": _flow_program_identity(program),
        "entry": entry,
        "function": {"entry": entry, "name": str(function.getName())},
        "profile": _flow_profile_identity(profile),
        "root": {
            "requested": root,
            "name": root_name,
            "kind": root_kind,
            "role": root_role,
            "identity": root_identity,
            "storage": symbol_record["storage"],
            "parameter": bool(symbol_record["parameter"]),
            "type": symbol_record["type"],
            "type_origin": "model-derived HighFunction prototype",
        },
        "accesses": accesses,
        "calls": calls,
        "completeness": _flow_completeness(stops, accesses),
    }


def _flow_program_identity(program: Any) -> dict[str, Any]:
    from .import_programs import HASH_OPTION

    return {
        "name": str(program.getName()),
        "binary_sha256": program.getOptions("Program Information").getString(HASH_OPTION, None),
        "language": str(program.getLanguageID()),
        "compiler_spec": str(program.getCompilerSpec().getCompilerSpecID()),
    }


def _flow_profile_identity(profile: str) -> dict[str, Any]:
    settings = {
        "program": "saved ProgramDB DecompileOptions",
        "analysis": {
            "infer_constant_pointers": True,
            "respect_read_only": True,
            "analyze_for_loops": True,
            "split_structures": True,
            "split_arrays": True,
            "split_pointers": True,
            "eliminate_unreachable": True,
        },
        "recovery": {
            "infer_constant_pointers": False,
            "respect_read_only": False,
            "analyze_for_loops": False,
            "split_structures": False,
            "split_arrays": False,
            "split_pointers": False,
            "eliminate_unreachable": False,
        },
    }
    if profile not in settings:
        raise ValueError(f"unknown decompiler profile: {profile}")
    return {"name": profile, "style": "decompile", "settings": settings[profile]}


def _flow_completeness(
    stops: list[dict[str, Any]], accesses: list[dict[str, Any]]
) -> dict[str, Any]:
    return {
        "status": "incomplete" if stops else "complete",
        "observation": (
            "accesses observed within the selected rooted function/profile"
            if accesses
            else "no accesses observed within the selected rooted function/profile"
        ),
        "scope": "one function, selected HighFunction root, and selected decompiler profile",
        "whole_program_absence_claim": False,
        "stops": stops,
    }


def _implicit_receiver_paths(
    program: Any, function: Any, accesses: list[dict[str, Any]]
) -> dict[str, tuple[str, int, str]]:
    """Follow raw register P-code from a semantic load to ECX at a call.

    High P-code intentionally omits an unresolved call's implicit ECX input.
    Raw instruction P-code still records ``MOV ECX, EDI``. A semantic load at
    the instruction that defines EDI seeds a local register path and COPY
    operations propagate it only along a contiguous fallthrough sequence.
    Calls and control-flow boundaries end the evidence.
    """

    load_paths: dict[str, tuple[str, int, str]] = {}
    for access in accesses:
        if access["kind"] != "load" or "[" in access["path"] or access["offset"] is None:
            continue
        offset = int(access["offset"], 16)
        load_paths[access["site"]] = (
            f"{access['path']}[{offset:#x}]",
            0,
            access["site"],
        )

    registers: dict[tuple[int, int], tuple[str, int, str]] = {}
    receivers: dict[str, tuple[str, int, str]] = {}
    expected_fallthrough: Any | None = None
    instructions = program.getListing().getInstructions(function.getBody(), True)
    while instructions.hasNext():
        instruction = instructions.next()
        address = instruction.getAddress()
        if expected_fallthrough is None or not bool(address.equals(expected_fallthrough)):
            registers.clear()
        site = str(instruction.getAddress())
        is_call = instruction.getMnemonicString().upper() == "CALL"
        if is_call:
            receiver = registers.get((4, 4))
            if receiver is not None:
                receivers[site] = receiver
        outputs = []
        for op in instruction.getPcode():
            output = op.getOutput()
            if output is None or output.getAddress().getAddressSpace().getName() != "register":
                continue
            key = (int(output.getOffset()), int(output.getSize()))
            outputs.append(key)
            propagated = None
            if op.getMnemonic() in {"COPY", "CAST", "INT_ZEXT", "INT_SEXT"} and op.getNumInputs():
                source = op.getInput(0)
                if source.getAddress().getAddressSpace().getName() == "register":
                    propagated = registers.get((int(source.getOffset()), int(source.getSize())))
            if propagated is not None:
                registers[key] = propagated
            else:
                registers.pop(key, None)
        seeded = load_paths.get(site)
        if seeded is not None:
            for key in outputs:
                if key[1] == 4:
                    registers[key] = seeded
        flow = instruction.getFlowType()
        if is_call:
            # VC6 calls may clobber every register this fallback tracks. The
            # receiver at this call is evidence; its register state is not
            # evidence for a later call.
            registers.clear()
        expected_fallthrough = (
            None
            if flow is None or flow.isJump() or flow.isTerminal()
            else instruction.getFallThrough()
        )
    return receivers


def condition_accesses(program: Any, argument: str) -> dict[str, Any]:
    """P-code backward slice of the branch controlling an assertion call.

    The result deliberately describes machine facts rather than rendered C:
    every contributing load, its width, extension, absolute address or
    root-relative displacement, and the branch whose successor contains the
    assertion.  Consumers may pair one assertion's member vocabulary with
    these accesses without regexing decompiler presentation.
    """

    from .resolve import program_address, resolve_function

    call_address = program_address(program, argument)
    function = resolve_function(program, argument)
    high = _high_function(program, function, "normalize")
    operations = list(high.getPcodeOps())
    calls = [
        op
        for op in operations
        if op.getMnemonic() in {"CALL", "CALLIND"}
        and op.getSeqnum().getTarget().equals(call_address)
    ]
    if not calls:
        raise ValueError(f"no call operation at {call_address}")
    call = calls[0]
    call_block = call.getParent()
    branches = [op for op in operations if op.getMnemonic() == "CBRANCH"]
    if call_block is None or not branches:
        return {
            "entry": str(function.getEntryPoint()),
            "callsite": str(call_address),
            "branch": None,
            "confidence": "unresolved",
            "accesses": [],
        }
    blocks = {block.getIndex(): block for block in high.getBasicBlocks()}
    successors = {
        index: [block.getOut(position).getIndex() for position in range(block.getOutSize())]
        for index, block in blocks.items()
    }
    call_index = call_block.getIndex()

    def distance(start: int, target: int) -> int | None:
        frontier = [(start, 0)]
        seen: set[int] = set()
        while frontier:
            current, count = frontier.pop(0)
            if current == target:
                return count
            if current in seen:
                continue
            seen.add(current)
            frontier.extend((child, count + 1) for child in successors.get(current, []))
        return None

    controllers: list[tuple[int, Any]] = []
    for branch_op in branches:
        parent = branch_op.getParent()
        outgoing = successors.get(parent.getIndex(), []) if parent is not None else []
        if len(outgoing) != 2:
            continue
        distances = [distance(child, call_index) for child in outgoing]
        reaching = [value for value in distances if value is not None]
        if len(reaching) == 1:
            controllers.append((reaching[0], branch_op))
    confidence = "exact-control-slice"
    if controllers:
        nearest = min(distance_value for distance_value, _branch in controllers)
        selected = [
            branch_op for distance_value, branch_op in controllers if distance_value == nearest
        ]
        if len(selected) != 1:
            return {
                "entry": str(function.getEntryPoint()),
                "callsite": str(call_address),
                "branch": None,
                "confidence": "ambiguous",
                "controllers": [str(item.getSeqnum().getTarget()) for item in selected],
                "accesses": [],
            }
        branch = selected[0]
    else:
        candidates = [
            op for op in branches if op.getSeqnum().getTarget().compareTo(call_address) <= 0
        ]
        if not candidates:
            return {
                "entry": str(function.getEntryPoint()),
                "callsite": str(call_address),
                "branch": None,
                "confidence": "unresolved",
                "accesses": [],
            }
        branch = max(
            candidates, key=lambda op: (op.getSeqnum().getTarget(), op.getSeqnum().getTime())
        )
        confidence = "candidate-control-slice"
    condition = branch.getInput(branch.getNumInputs() - 1)
    accesses: list[dict[str, Any]] = []
    visited: set[tuple[Any, ...]] = set()

    def walk(node: Any, extension: str | None = None, stride: int | None = None) -> None:
        marker = _node_key(node)
        if marker is None or marker in visited:
            return
        visited.add(marker)
        space = node.getAddress().getAddressSpace().getName()
        if space == "ram" and node.getDef() is None:
            address = node.getAddress()
            if program.getMemory().contains(address):
                symbol = program.getSymbolTable().getPrimarySymbol(address)
                accesses.append(
                    {
                        "site": str(branch.getSeqnum().getTarget()),
                        "kind": "absolute",
                        "address": str(address),
                        "storage": symbol.getName() if symbol is not None else f"DAT_{address}",
                        "width": node.getSize(),
                        "extension": extension,
                        "stride": stride,
                    }
                )
            return
        definition = node.getDef()
        if definition is None:
            return
        mnemonic = definition.getMnemonic()
        if mnemonic in {"INT_SEXT", "INT_ZEXT"}:
            walk(definition.getInput(0), "signed" if mnemonic == "INT_SEXT" else "unsigned", stride)
            return
        if mnemonic == "PTRADD" and definition.getNumInputs() > 2:
            scale = definition.getInput(2)
            walk(
                definition.getInput(0),
                extension,
                scale.getOffset() if scale is not None and scale.isConstant() else stride,
            )
            walk(definition.getInput(1), extension, stride)
            return
        if mnemonic == "LOAD":
            pointer = definition.getInput(1)
            resolved = _address_expression(pointer)
            entry: dict[str, Any] = {
                "site": str(definition.getSeqnum().getTarget()),
                "kind": "load",
                "width": node.getSize(),
                "extension": extension,
                "stride": stride,
            }
            if resolved["kind"] == "absolute":
                address = (
                    program.getAddressFactory()
                    .getDefaultAddressSpace()
                    .getAddress(f"{resolved['address']:x}")
                )
                symbol = program.getSymbolTable().getPrimarySymbol(address)
                entry.update(
                    {
                        "kind": "absolute",
                        "address": str(address),
                        "storage": symbol.getName() if symbol is not None else f"DAT_{address}",
                    }
                )
            elif resolved["kind"] == "root-relative":
                entry.update(
                    {
                        "kind": "root-relative",
                        "root": resolved["root"],
                        "offset": f"0x{resolved['offset']:x}",
                    }
                )
            accesses.append(entry)
            walk(pointer, extension, stride)
            return
        for index in range(definition.getNumInputs()):
            walk(definition.getInput(index), extension, stride)

    walk(condition)
    unique = {
        (
            item.get("kind"),
            item.get("address"),
            item.get("offset"),
            item.get("width"),
            item.get("extension"),
            item.get("stride"),
            item.get("site"),
        ): item
        for item in accesses
    }
    return {
        "entry": str(function.getEntryPoint()),
        "callsite": str(call_address),
        "branch": str(branch.getSeqnum().getTarget()),
        "confidence": confidence,
        "accesses": sorted(
            unique.values(),
            key=lambda item: (
                item.get("site") or "",
                item.get("address") or item.get("offset") or "",
            ),
        ),
    }


def _address_expression(node: Any) -> dict[str, Any]:
    """Resolve a P-code pointer without losing the base of a relative offset."""

    if node is None:
        return {"kind": "unresolved"}
    space = node.getAddress().getAddressSpace().getName()
    if space == "ram" and node.getDef() is None:
        return {"kind": "absolute", "address": int(node.getOffset())}
    definition = node.getDef()
    if definition is None:
        high = node.getHigh()
        symbol = high.getSymbol() if high is not None else None
        root = symbol.getName() if symbol is not None else str(node.getAddress())
        return {"kind": "root-relative", "root": root, "offset": 0}
    mnemonic = definition.getMnemonic()
    if mnemonic in {"COPY", "CAST"}:
        return _address_expression(definition.getInput(0))
    if mnemonic in {"INT_ADD", "INT_SUB", "PTRSUB"} and definition.getNumInputs() >= 2:
        left, right = definition.getInput(0), definition.getInput(1)
        if right is not None and right.isConstant():
            base = _address_expression(left)
            delta = int(right.getOffset())
            if mnemonic == "INT_SUB":
                delta = -delta
            if base["kind"] == "absolute":
                return {"kind": "absolute", "address": int(base["address"]) + delta}
            if base["kind"] == "root-relative":
                return {**base, "offset": int(base["offset"]) + delta}
        if mnemonic == "INT_ADD" and left is not None and left.isConstant():
            base = _address_expression(right)
            if base["kind"] == "absolute":
                return {
                    "kind": "absolute",
                    "address": int(base["address"]) + int(left.getOffset()),
                }
            if base["kind"] == "root-relative":
                return {**base, "offset": int(base["offset"]) + int(left.getOffset())}
    if mnemonic == "PTRADD" and definition.getNumInputs() > 2:
        index, scale = definition.getInput(1), definition.getInput(2)
        if index is not None and index.isConstant() and scale is not None and scale.isConstant():
            delta = int(index.getOffset()) * int(scale.getOffset())
            base = _address_expression(definition.getInput(0))
            if base["kind"] == "absolute":
                return {"kind": "absolute", "address": int(base["address"]) + delta}
            if base["kind"] == "root-relative":
                return {**base, "offset": int(base["offset"]) + delta}
    return {"kind": "unresolved"}


def _callsite_facts(function: Any, high: Any, addresses: set[str] | None = None) -> dict[str, Any]:
    """Project call facts from one live HighFunction without redecompiling."""

    sites = []
    iterator = high.getPcodeOps()
    while iterator.hasNext():
        op = iterator.next()
        address = str(op.getSeqnum().getTarget())
        if op.getMnemonic() not in {"CALL", "CALLIND"} or (
            addresses is not None and address not in addresses
        ):
            continue
        target = op.getInput(0)
        sites.append(
            {
                "op": op.getMnemonic(),
                "address": address,
                "target": (
                    str(target.getAddress())
                    if target is not None and target.isAddress()
                    else _varnode(target)
                ),
                "arguments": [_varnode(op.getInput(i)) for i in range(1, op.getNumInputs())],
                "output": _varnode(op.getOutput()),
            }
        )
    return {"entry": str(function.getEntryPoint()), "sites": sites}


def callsite(program: Any, argument: str) -> dict[str, Any]:
    """The CALL or CALLIND at one address, with normalized arguments."""

    from .resolve import program_address, resolve_function

    address = program_address(program, argument)
    function = resolve_function(program, argument)
    high = _high_function(program, function)
    facts = _callsite_facts(function, high, {str(address)})
    sites = facts["sites"]
    if not sites:
        raise ValueError(f"no call operation at {address}")
    return facts
