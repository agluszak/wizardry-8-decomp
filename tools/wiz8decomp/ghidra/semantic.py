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
the daemon's lifetime. Three styles matter here - `decompile` for typed
analysis, `normalize` for a stable data-flow form without type-recovery noise,
and `paramid` for parameter measurement - and the extraction styles keep the
syntax tree while skipping C generation entirely.

Read-only by construction: nothing here mutates the program, so the module is
replay-inert for the materialization key.
"""

from __future__ import annotations

from typing import Any

_TIMEOUT_SECONDS = 120
# One trace step per varnode-op edge; a function that legitimately exceeds
# this is beyond what one query should return anyway.
_TRACE_LIMIT = 20000
_STYLES = ("decompile", "normalize", "paramid")

# (program unique id, style) -> DecompInterface. The daemon serves one program,
# so this holds at most a handful of interfaces; one-shot paths dispose on exit.
_sessions: dict[tuple[int, str], Any] = {}


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


def _session(program: Any, style: str, *, c_output: bool) -> Any:
    """The persistent decompiler interface for `program` in `style`.

    C generation is part of the key: toggling it resets an open interface, so
    an extraction session (tree only) and a presentation session (tree plus C)
    coexist rather than thrash one interface's configuration.
    """

    from ghidra.app.decompiler import DecompInterface

    if style not in _STYLES:
        raise ValueError(f"unknown decompiler style: {style}")
    key = (int(program.getUniqueProgramID()), f"{style}:{'c' if c_output else 'tree'}")
    interface = _sessions.get(key)
    if interface is not None:
        return interface
    interface = DecompInterface()
    interface.setSimplificationStyle(style)
    if not c_output:
        interface.toggleCCode(False)
        interface.toggleSyntaxTree(True)
    interface.openProgram(program)
    _sessions[key] = interface
    return interface


def dispose_sessions() -> None:
    """Release every cached interface; the daemon calls this on shutdown."""

    import contextlib

    while _sessions:
        _, interface = _sessions.popitem()
        # A dying JVM can make dispose fail; the process is exiting either way.
        with contextlib.suppress(Exception):
            interface.dispose()


def _high_function(program: Any, function: Any, style: str = "decompile") -> Any:
    from ghidra.util.task import TaskMonitor

    interface = _session(program, style, c_output=False)
    result = interface.decompileFunction(function, _TIMEOUT_SECONDS, TaskMonitor.DUMMY)
    high = result.getHighFunction() if result is not None else None
    if high is None:
        error = result.getErrorMessage() if result is not None else "no result"
        raise RuntimeError(
            f"no high function for {function.getEntryPoint()}: {error or 'decompilation failed'}"
        )
    return high


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

    from .query import _function, function_metadata

    function = _function(program, argument)
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
        "function": function_metadata(program, function),
        "return_type": (
            prototype.getReturnType().getDisplayName() if prototype.getReturnType() else None
        ),
        "calling_convention": str(prototype.getModelName()),
        "parameters": parameters,
        "locals": sorted(locals_, key=lambda item: item["name"]),
    }


def pcode(program: Any, argument: str, style: str = "decompile") -> dict[str, Any]:
    """The function's P-code stream in execution order, one entry per op."""

    from .query import _function

    function = _function(program, argument)
    high = _high_function(program, function, style)
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


def trace_accesses(instances: list[Any], root: str) -> list[dict[str, Any]]:
    """Every access reachable from the given root varnodes, with derived offsets.

    The trace follows value-preserving ops (COPY, CAST, MULTIEQUAL, INDIRECT)
    and constant pointer arithmetic (INT_ADD, INT_SUB, PTRADD, PTRSUB), so each
    reached varnode carries a byte offset from the root. Loads spawn further
    levels: the loaded pointer becomes a sub-root whose own accesses describe
    the *pointee* - which is what turns `delete this->member` sequences into
    typed shape constraints instead of prose.

    Everything here is duck-typed against the varnode and P-code op surface, so
    the traversal is unit-testable with fakes; the JPype boundary is exactly
    where a Python identity check silently broke once already.
    """

    accesses: list[dict[str, Any]] = []
    steps = 0

    def same(left: Any, right: Any) -> bool:
        # JPype hands out fresh wrapper objects, so Python identity is useless
        # here; Java equality is the identity that matters.
        return right is not None and bool(left.equals(right))

    def record(kind: str, op: Any, path: str, offset: int, **extra: Any) -> None:
        accesses.append(
            {
                "kind": kind,
                "site": str(op.getSeqnum().getTarget()),
                "path": path,
                "offset": f"0x{offset:x}",
                **extra,
            }
        )

    def trace(node: Any, offset: int, path: str, depth: int, visited: set[Any]) -> None:
        nonlocal steps
        marker = _node_key(node)
        if marker in visited:
            return
        visited.add(marker)
        descendants = node.getDescendants()
        while descendants.hasNext():
            steps += 1
            if steps > _TRACE_LIMIT:
                raise RuntimeError(f"field trace exceeded {_TRACE_LIMIT} steps")
            op = descendants.next()
            mnemonic = op.getMnemonic()
            output = op.getOutput()
            if mnemonic in {"COPY", "CAST", "MULTIEQUAL", "INDIRECT"}:
                if output is not None:
                    trace(output, offset, path, depth, visited)
            elif mnemonic in {"INT_ADD", "INT_SUB", "PTRSUB"}:
                other = op.getInput(1) if same(node, op.getInput(0)) else op.getInput(0)
                if other is not None and other.isConstant() and output is not None:
                    delta = other.getOffset()
                    if mnemonic == "INT_SUB" and same(node, op.getInput(0)):
                        delta = -delta
                    trace(output, offset + delta, path, depth, visited)
            elif mnemonic == "PTRADD":
                index_node, scale = op.getInput(1), op.getInput(2)
                if (
                    same(node, op.getInput(0))
                    and index_node is not None
                    and index_node.isConstant()
                    and scale is not None
                    and scale.isConstant()
                    and output is not None
                ):
                    trace(
                        output,
                        offset + index_node.getOffset() * scale.getOffset(),
                        path,
                        depth,
                        visited,
                    )
            elif mnemonic == "LOAD":
                if same(node, op.getInput(1)) and output is not None:
                    record("load", op, path, offset, width=output.getSize())
                    if depth < 3 and output.getSize() == 4:
                        # The loaded value is a candidate pointer; its own
                        # accesses describe the pointee's shape.
                        trace(output, 0, f"{path}[{offset:#x}]", depth + 1, visited)
            elif mnemonic == "STORE":
                if same(node, op.getInput(1)):
                    value = op.getInput(2)
                    record(
                        "store",
                        op,
                        path,
                        offset,
                        width=value.getSize() if value is not None else None,
                        value=_varnode(value),
                    )
                elif same(node, op.getInput(2)):
                    record("stored-elsewhere", op, path, offset)
            elif mnemonic in {"CALL", "CALLIND"}:
                target = op.getInput(0)
                positions = [
                    index - 1
                    for index in range(1, op.getNumInputs())
                    if same(node, op.getInput(index))
                ]
                if mnemonic == "CALLIND" and same(node, target):
                    record(
                        "indirect-call-target",
                        op,
                        path,
                        offset,
                        arguments=[_varnode(op.getInput(i)) for i in range(1, op.getNumInputs())],
                    )
                for position in positions:
                    record(
                        "call-arg" if mnemonic == "CALL" else "indirect-call-arg",
                        op,
                        path,
                        offset,
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
                    record("null-test", op, path, offset, negated=mnemonic == "INT_NOTEQUAL")
            elif mnemonic == "RETURN":
                record("returned", op, path, offset)

    for instance in instances:
        trace(instance, 0, root, 0, set())
    accesses.sort(key=lambda item: (item["path"], int(item["offset"], 16), item["site"]))
    return accesses


def trace_value_paths(instances: list[Any], root: str) -> dict[tuple[Any, ...], tuple[str, int]]:
    """Map every root-derived varnode to its semantic path and byte offset.

    This is the data-flow witness needed for implicit ``this`` calls.  VC6
    passes a receiver in ECX, but a High CALL with an unresolved prototype has
    no receiver input.  The COPY into ECX immediately before that CALL still
    has a normal SSA definition, so retaining the root path of every derived
    value lets callers prove which member reached the implicit receiver.
    """

    paths: dict[tuple[Any, ...], tuple[str, int]] = {}
    steps = 0

    def same(left: Any, right: Any) -> bool:
        return right is not None and bool(left.equals(right))

    def trace(node: Any, offset: int, path: str, depth: int, visited: set[Any]) -> None:
        nonlocal steps
        marker = _node_key(node)
        if marker is None or marker in visited:
            return
        visited.add(marker)
        paths[marker] = (path, offset)
        descendants = node.getDescendants()
        while descendants.hasNext():
            steps += 1
            if steps > _TRACE_LIMIT:
                raise RuntimeError(f"value-path trace exceeded {_TRACE_LIMIT} steps")
            op = descendants.next()
            mnemonic = op.getMnemonic()
            output = op.getOutput()
            if mnemonic in {"COPY", "CAST", "MULTIEQUAL", "INDIRECT"}:
                if output is not None:
                    trace(output, offset, path, depth, visited)
            elif mnemonic in {"INT_ADD", "INT_SUB", "PTRSUB"}:
                other = op.getInput(1) if same(node, op.getInput(0)) else op.getInput(0)
                if other is not None and other.isConstant() and output is not None:
                    delta = other.getOffset()
                    if mnemonic == "INT_SUB" and same(node, op.getInput(0)):
                        delta = -delta
                    trace(output, offset + delta, path, depth, visited)
            elif mnemonic == "PTRADD":
                index_node, scale = op.getInput(1), op.getInput(2)
                if (
                    same(node, op.getInput(0))
                    and index_node is not None
                    and index_node.isConstant()
                    and scale is not None
                    and scale.isConstant()
                    and output is not None
                ):
                    trace(
                        output,
                        offset + index_node.getOffset() * scale.getOffset(),
                        path,
                        depth,
                        visited,
                    )
            elif (
                mnemonic == "LOAD"
                and same(node, op.getInput(1))
                and output is not None
                and depth < 3
                and output.getSize() == 4
            ):
                trace(output, 0, f"{path}[{offset:#x}]", depth + 1, visited)

    for instance in instances:
        trace(instance, 0, root, 0, set())
    return paths


def field_accesses(program: Any, argument: str, root: str) -> dict[str, Any]:
    """`trace_accesses` for one function parameter, plus the call table.

    The call table lists every CALL and CALLIND in flow order with block
    indexes: a receiver passed through ECX to an unknown-prototype callee never
    appears among that CALL's inputs, so the downstream object rules correlate
    a member's null test with the calls of the guarded successor block instead.
    """

    from .query import _function

    function = _function(program, argument)
    high = _high_function(program, function)
    symbol = _resolve_root(high, root)
    instances = _instances(symbol)
    accesses = trace_accesses(instances, symbol.getName())
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
        "entry": str(function.getEntryPoint()),
        "root": symbol.getName(),
        "accesses": accesses,
        "calls": calls,
    }


def _implicit_receiver_paths(
    program: Any, function: Any, accesses: list[dict[str, Any]]
) -> dict[str, tuple[str, int, str]]:
    """Follow raw register P-code from a semantic load to ECX at a call.

    High P-code intentionally omits an unresolved call's implicit ECX input.
    Raw instruction P-code still records ``MOV ECX, EDI``. A semantic load at
    the instruction that defines EDI seeds the register path, COPY operations
    propagate it, and the ECX value at CALL is therefore a real data-flow
    witness rather than same-block proximity.
    """

    load_paths: dict[str, tuple[str, int, str]] = {}
    for access in accesses:
        if access["kind"] != "load" or "[" in access["path"]:
            continue
        offset = int(access["offset"], 16)
        load_paths[access["site"]] = (
            f"{access['path']}[{offset:#x}]",
            0,
            access["site"],
        )

    registers: dict[tuple[int, int], tuple[str, int, str]] = {}
    receivers: dict[str, tuple[str, int, str]] = {}
    instructions = program.getListing().getInstructions(function.getBody(), True)
    while instructions.hasNext():
        instruction = instructions.next()
        site = str(instruction.getAddress())
        if instruction.getMnemonicString().upper() == "CALL":
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
    return receivers


def condition_accesses(program: Any, argument: str) -> dict[str, Any]:
    """P-code backward slice of the branch controlling an assertion call.

    The result deliberately describes machine facts rather than rendered C:
    every contributing load, its width, extension, absolute address or
    root-relative displacement, and the branch whose successor contains the
    assertion.  Consumers may pair one assertion's member vocabulary with
    these accesses without regexing decompiler presentation.
    """

    from .query import _address, _function

    call_address = _address(program, argument)
    function = _function(program, argument)
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
    candidates = []
    if call_block is not None:
        for index in range(call_block.getInSize()):
            predecessor = call_block.getIn(index)
            candidates.extend(
                op
                for op in operations
                if op.getParent() is not None
                and op.getParent().getIndex() == predecessor.getIndex()
                and op.getMnemonic() == "CBRANCH"
            )
    if not candidates:
        candidates = [
            op
            for op in operations
            if op.getMnemonic() == "CBRANCH"
            and op.getSeqnum().getTarget().compareTo(call_address) <= 0
        ]
    if not candidates:
        return {
            "entry": str(function.getEntryPoint()),
            "callsite": str(call_address),
            "branch": None,
            "accesses": [],
        }
    branch = max(candidates, key=lambda op: (op.getSeqnum().getTarget(), op.getSeqnum().getTime()))
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
            if resolved[0] == "absolute":
                address = (
                    program.getAddressFactory()
                    .getDefaultAddressSpace()
                    .getAddress(f"{resolved[1]:x}")
                )
                symbol = program.getSymbolTable().getPrimarySymbol(address)
                entry.update(
                    {
                        "kind": "absolute",
                        "address": str(address),
                        "storage": symbol.getName() if symbol is not None else f"DAT_{address}",
                    }
                )
            elif resolved[0] == "offset":
                entry.update({"kind": "root-offset", "offset": f"0x{resolved[1]:x}"})
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
        "accesses": sorted(
            unique.values(),
            key=lambda item: (
                item.get("site") or "",
                item.get("address") or item.get("offset") or "",
            ),
        ),
    }


def _address_expression(node: Any) -> tuple[str, int] | tuple[None, None]:
    """Classify a P-code pointer as absolute storage or root-relative offset."""

    if node is None:
        return (None, None)
    space = node.getAddress().getAddressSpace().getName()
    if space == "ram" and node.getDef() is None:
        return ("absolute", int(node.getOffset()))
    definition = node.getDef()
    if definition is None:
        return (None, None)
    mnemonic = definition.getMnemonic()
    if mnemonic in {"COPY", "CAST", "INDIRECT"}:
        return _address_expression(definition.getInput(0))
    if mnemonic in {"INT_ADD", "PTRSUB"} and definition.getNumInputs() >= 2:
        left, right = definition.getInput(0), definition.getInput(1)
        if right is not None and right.isConstant():
            base = _address_expression(left)
            if base[0] == "absolute":
                return ("absolute", int(base[1]) + int(right.getOffset()))
            return ("offset", int(right.getOffset()))
        if left is not None and left.isConstant():
            base = _address_expression(right)
            if base[0] == "absolute":
                return ("absolute", int(base[1]) + int(left.getOffset()))
            return ("offset", int(left.getOffset()))
    if mnemonic == "PTRADD" and definition.getNumInputs() > 2:
        index, scale = definition.getInput(1), definition.getInput(2)
        if index is not None and index.isConstant() and scale is not None and scale.isConstant():
            delta = int(index.getOffset()) * int(scale.getOffset())
            base = _address_expression(definition.getInput(0))
            if base[0] == "absolute":
                return ("absolute", int(base[1]) + delta)
            return ("offset", delta)
    return (None, None)


def callsite(program: Any, argument: str) -> dict[str, Any]:
    """The CALL or CALLIND at one address, with normalized arguments."""

    from .query import _address, _function

    address = _address(program, argument)
    function = _function(program, argument)
    high = _high_function(program, function)
    iterator = high.getPcodeOps(address)
    sites = []
    while iterator.hasNext():
        op = iterator.next()
        if op.getMnemonic() not in {"CALL", "CALLIND"}:
            continue
        target = op.getInput(0)
        sites.append(
            {
                "op": op.getMnemonic(),
                "address": str(op.getSeqnum().getTarget()),
                "target": (
                    str(target.getAddress())
                    if target is not None and target.isAddress()
                    else _varnode(target)
                ),
                "arguments": [_varnode(op.getInput(i)) for i in range(1, op.getNumInputs())],
                "output": _varnode(op.getOutput()),
            }
        )
    if not sites:
        raise ValueError(f"no call operation at {address}")
    return {"entry": str(function.getEntryPoint()), "sites": sites}
