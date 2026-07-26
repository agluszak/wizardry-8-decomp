from __future__ import annotations

import hashlib
import re
from typing import Any


def _address(program: Any, text: str) -> Any:
    value = program.getAddressFactory().getAddress(text)
    if value is None:
        value = program.getAddressFactory().getDefaultAddressSpace().getAddress(text.removeprefix("0x"))
    if value is None:
        raise ValueError(f"invalid address: {text}")
    return value


def _function(program: Any, text: str) -> Any:
    address = _address(program, text)
    manager = program.getFunctionManager()
    function = manager.getFunctionContaining(address) or manager.getFunctionAt(address)
    if function is None:
        raise ValueError(f"no function contains {address}")
    return function


def function_metadata(program: Any, function: Any) -> dict[str, Any]:
    from ghidra.program.model.listing import CodeUnit

    manager = program.getFunctionManager()
    references = program.getReferenceManager()
    body = function.getBody()
    listing = program.getListing()
    instructions = list(listing.getInstructions(body, True))
    raw = bytearray()
    mnemonic_parts = []
    for instruction in instructions:
        try:
            raw.extend(instruction.getBytes())
        except Exception:
            pass
        operand_types = [str(instruction.getOperandType(index)) for index in range(instruction.getNumOperands())]
        mnemonic_parts.append(instruction.getMnemonicString() + ":" + ",".join(operand_types))
    callers = sorted({str(ref.getFromAddress()) for ref in references.getReferencesTo(function.getEntryPoint())})
    callees = sorted({str(ref.getToAddress()) for instruction in instructions for ref in references.getReferencesFrom(instruction.getAddress()) if manager.getFunctionAt(ref.getToAddress()) is not None})
    strings = []
    for instruction in instructions:
        for reference in references.getReferencesFrom(instruction.getAddress()):
            data = listing.getDataAt(reference.getToAddress())
            if data is not None and data.hasStringValue():
                strings.append(str(data.getValue()))
    return {
        "entry": str(function.getEntryPoint()),
        "size": body.getNumAddresses(),
        "name": function.getName(),
        "namespace": str(function.getParentNamespace()),
        "thunk": bool(function.isThunk()),
        "thunk_target": str(function.getThunkedFunction(False).getEntryPoint()) if function.isThunk() and function.getThunkedFunction(False) else None,
        "calling_convention": function.getCallingConventionName(),
        "prototype": function.getPrototypeString(False, False),
        "plate_comment": listing.getComment(CodeUnit.PLATE_COMMENT, function.getEntryPoint()),
        "caller_count": len(callers),
        "callee_count": len(callees),
        "callers": callers,
        "callees": callees,
        "referenced_strings": sorted(set(strings), key=str.casefold),
        "raw_body_sha256": hashlib.sha256(raw).hexdigest(),
        "instruction_fingerprint_sha256": hashlib.sha256("\n".join(mnemonic_parts).encode()).hexdigest(),
    }


def _listing(program: Any, argument: str) -> dict[str, Any]:
    function = _function(program, argument)
    lines = [f"{instruction.getAddress()}  {instruction}" for instruction in program.getListing().getInstructions(function.getBody(), True)]
    return {"function": function_metadata(program, function), "listing": "\n".join(lines)}


def _decompile(program: Any, argument: str) -> dict[str, Any]:
    from ghidra.app.decompiler import DecompInterface
    from ghidra.util.task import TaskMonitor

    function = _function(program, argument)
    interface = DecompInterface()
    interface.openProgram(program)
    try:
        result = interface.decompileFunction(function, 120, TaskMonitor.DUMMY)
        return {
            "function": function_metadata(program, function),
            "completed": bool(result.decompileCompleted()),
            "error": result.getErrorMessage(),
            "decompiled": result.getDecompiledFunction().getC() if result.decompileCompleted() and result.getDecompiledFunction() else None,
        }
    finally:
        interface.dispose()


def _xrefs(program: Any, argument: str, direction: str) -> dict[str, Any]:
    address = _address(program, argument)
    manager = program.getReferenceManager()
    if direction == "to":
        refs = manager.getReferencesTo(address)
    elif direction == "from":
        refs = manager.getReferencesFrom(address)
    else:
        refs = list(manager.getReferencesTo(address)) + list(manager.getReferencesFrom(address))
    values = sorted({(str(ref.getFromAddress()), str(ref.getToAddress()), str(ref.getReferenceType())) for ref in refs})
    return {"address": str(address), "references": [{"from": a, "to": b, "type": c} for a, b, c in values]}


def _strings(program: Any, pattern: str | None = None) -> dict[str, Any]:
    values = []
    listing = program.getListing()
    iterator = listing.getDefinedData(True)
    regex = re.compile(pattern, re.I) if pattern else None
    while iterator.hasNext():
        data = iterator.next()
        if data.hasStringValue():
            value = str(data.getValue())
            if regex is None or regex.search(value):
                refs = program.getReferenceManager().getReferencesTo(data.getAddress())
                values.append({"address": str(data.getAddress()), "value": value, "references": sorted(str(ref.getFromAddress()) for ref in refs)})
    return {"strings": values}


def _functions(program: Any, pattern: str | None = None) -> dict[str, Any]:
    regex = re.compile(pattern, re.I) if pattern else None
    values = []
    iterator = program.getFunctionManager().getFunctions(True)
    while iterator.hasNext():
        function = iterator.next()
        if regex is None or regex.search(function.getName()):
            values.append({"entry": str(function.getEntryPoint()), "name": function.getName(), "size": function.getBody().getNumAddresses()})
    return {"functions": values}


def _symbols(program: Any, external: bool) -> dict[str, Any]:
    table = program.getSymbolTable()
    iterator = table.getExternalSymbols() if external else table.getAllSymbols(True)
    values = []
    while iterator.hasNext():
        symbol = iterator.next()
        if external or symbol.isExternalEntryPoint():
            values.append({"name": symbol.getName(True), "address": str(symbol.getAddress()), "type": str(symbol.getSymbolType())})
    return {"imports" if external else "exports": values}


def _sections(program: Any) -> dict[str, Any]:
    return {"sections": [{"name": block.getName(), "start": str(block.getStart()), "end": str(block.getEnd()), "size": block.getSize(), "read": block.isRead(), "write": block.isWrite(), "execute": block.isExecute()} for block in program.getMemory().getBlocks()]}


def _read_data(program: Any, address_text: str, size_text: str) -> dict[str, Any]:
    import jpype

    address = _address(program, address_text)
    size = int(size_text, 0)
    if size < 0 or size > 16 * 1024 * 1024:
        raise ValueError("read-data size must be between 0 and 16 MiB")
    # A plain Python bytearray is converted to a *copy* of a Java byte[] at
    # the JPype boundary, so Memory.getBytes's in-place fill is invisible
    # back in Python; a JPype JArray shares storage across the boundary.
    buffer = jpype.JArray(jpype.JByte)(size)
    read = program.getMemory().getBytes(address, buffer)
    return {"address": str(address), "size": read, "hex": bytes(b & 0xFF for b in buffer[:read]).hex()}


def execute_query(program: Any, command: str, arguments: list[str]) -> dict[str, Any]:
    if command == "listing":
        return _listing(program, arguments[0])
    if command == "decompile":
        return _decompile(program, arguments[0])
    if command in {"xrefs", "xrefs-to", "xrefs-from"}:
        return _xrefs(program, arguments[0], {"xrefs": "both", "xrefs-to": "to", "xrefs-from": "from"}[command])
    if command in {"function", "function-slice"}:
        return {"function": function_metadata(program, _function(program, arguments[0]))}
    if command == "read-data":
        return _read_data(program, arguments[0], arguments[1])
    if command == "strings":
        return _strings(program)
    if command == "string-refs":
        return _strings(program, arguments[0])
    if command == "search":
        return {**_strings(program, arguments[0]), **_functions(program, arguments[0])}
    if command == "functions":
        return _functions(program)
    if command == "imports":
        return _symbols(program, True)
    if command == "exports":
        return _symbols(program, False)
    if command == "sections":
        return _sections(program)
    raise ValueError(f"unsupported query command: {command}")


def validate_query_arguments(command: str, arguments: list[str]) -> None:
    arity = {
        "listing": 1, "decompile": 1, "xrefs": 1, "xrefs-to": 1, "xrefs-from": 1,
        "function": 1, "function-slice": 1, "read-data": 2, "strings": 0,
        "string-refs": 1, "search": 1, "functions": 0, "imports": 0, "exports": 0, "sections": 0,
    }
    if command not in arity:
        raise ValueError("unknown command; expected one of: " + ", ".join(sorted(arity)))
    if len(arguments) != arity[command]:
        raise ValueError(f"{command} expects {arity[command]} argument(s), got {len(arguments)}")
