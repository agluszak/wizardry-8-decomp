from __future__ import annotations

import hashlib
import re
from typing import Any


def _address(program: Any, text: str) -> Any:
    value = program.getAddressFactory().getAddress(text)
    if value is None:
        value = (
            program.getAddressFactory().getDefaultAddressSpace().getAddress(text.removeprefix("0x"))
        )
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
        except Exception:  # noqa: BLE001,S110 - unreadable instruction bytes stay absent
            pass
        operand_types = [
            str(instruction.getOperandType(index)) for index in range(instruction.getNumOperands())
        ]
        mnemonic_parts.append(instruction.getMnemonicString() + ":" + ",".join(operand_types))
    callers = sorted(
        {str(ref.getFromAddress()) for ref in references.getReferencesTo(function.getEntryPoint())}
    )
    callees = sorted(
        {
            str(ref.getToAddress())
            for instruction in instructions
            for ref in references.getReferencesFrom(instruction.getAddress())
            if manager.getFunctionAt(ref.getToAddress()) is not None
        }
    )
    data_references = sorted(
        (
            {
                "site": str(reference.getFromAddress()),
                "target": str(reference.getToAddress()),
                "access": str(reference.getReferenceType()),
            }
            for instruction in instructions
            for reference in references.getReferencesFrom(instruction.getAddress())
            if manager.getFunctionAt(reference.getToAddress()) is None
            and program.getMemory().contains(reference.getToAddress())
        ),
        key=lambda item: (item["site"], item["target"], item["access"]),
    )
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
        "thunk_target": str(function.getThunkedFunction(False).getEntryPoint())
        if function.isThunk() and function.getThunkedFunction(False)
        else None,
        "calling_convention": function.getCallingConventionName(),
        "prototype": function.getPrototypeString(False, False),
        "plate_comment": listing.getComment(CodeUnit.PLATE_COMMENT, function.getEntryPoint()),
        "caller_count": len(callers),
        "callee_count": len(callees),
        "callers": callers,
        "callees": callees,
        "data_references": data_references,
        "instruction_addresses": [str(instruction.getAddress()) for instruction in instructions],
        "referenced_strings": sorted(set(strings), key=str.casefold),
        "raw_body_sha256": hashlib.sha256(raw).hexdigest(),
        "instruction_fingerprint_sha256": hashlib.sha256(
            "\n".join(mnemonic_parts).encode()
        ).hexdigest(),
    }


def _listing(program: Any, argument: str) -> dict[str, Any]:
    function = _function(program, argument)
    lines = [
        f"{instruction.getAddress()}  {instruction}"
        for instruction in program.getListing().getInstructions(function.getBody(), True)
    ]
    return {"function": function_metadata(program, function), "listing": "\n".join(lines)}


def _decompile(program: Any, argument: str) -> dict[str, Any]:
    from .semantic import decompile_c

    function = _function(program, argument)
    return {"function": function_metadata(program, function), **decompile_c(program, function)}


def _xrefs(program: Any, argument: str, direction: str) -> dict[str, Any]:
    address = _address(program, argument)
    manager = program.getReferenceManager()
    if direction == "to":
        refs = manager.getReferencesTo(address)
    elif direction == "from":
        refs = manager.getReferencesFrom(address)
    else:
        refs = list(manager.getReferencesTo(address)) + list(manager.getReferencesFrom(address))
    values = sorted(
        {
            (str(ref.getFromAddress()), str(ref.getToAddress()), str(ref.getReferenceType()))
            for ref in refs
        }
    )
    return {
        "address": str(address),
        "references": [{"from": a, "to": b, "type": c} for a, b, c in values],
    }


def _strings(program: Any, pattern: str | None = None) -> dict[str, Any]:
    values = []
    listing = program.getListing()
    iterator = listing.getDefinedData(True)
    regex = re.compile(pattern, re.IGNORECASE) if pattern else None
    while iterator.hasNext():
        data = iterator.next()
        if data.hasStringValue():
            value = str(data.getValue())
            if regex is None or regex.search(value):
                refs = program.getReferenceManager().getReferencesTo(data.getAddress())
                values.append(
                    {
                        "address": str(data.getAddress()),
                        "value": value,
                        "references": sorted(str(ref.getFromAddress()) for ref in refs),
                    }
                )
    return {"strings": values}


def _functions(program: Any, pattern: str | None = None) -> dict[str, Any]:
    regex = re.compile(pattern, re.IGNORECASE) if pattern else None
    values = []
    iterator = program.getFunctionManager().getFunctions(True)
    while iterator.hasNext():
        function = iterator.next()
        if regex is None or regex.search(function.getName()):
            values.append(
                {
                    "entry": str(function.getEntryPoint()),
                    "name": function.getName(),
                    "size": function.getBody().getNumAddresses(),
                }
            )
    return {"functions": values}


def _function_of(program: Any, argument: str) -> dict[str, Any]:
    """Containing-function entry for each comma-separated address; null when none.

    Containment uses the function manager's real (possibly non-contiguous) body,
    which an entry-plus-size reading of the `functions` listing cannot reproduce.
    """

    manager = program.getFunctionManager()
    values: dict[str, str | None] = {}
    for text in argument.split(","):
        text = text.strip()
        if not text:
            continue
        function = manager.getFunctionContaining(_address(program, text))
        values[text] = str(function.getEntryPoint()) if function is not None else None
    return {"functions": values}


def _symbols(program: Any, external: bool) -> dict[str, Any]:
    table = program.getSymbolTable()
    iterator = table.getExternalSymbols() if external else table.getAllSymbols(True)
    values = []
    while iterator.hasNext():
        symbol = iterator.next()
        if external or symbol.isExternalEntryPoint():
            values.append(
                {
                    "name": symbol.getName(True),
                    "address": str(symbol.getAddress()),
                    "type": str(symbol.getSymbolType()),
                }
            )
    return {"imports" if external else "exports": values}


def _sections(program: Any) -> dict[str, Any]:
    return {
        "sections": [
            {
                "name": block.getName(),
                "start": str(block.getStart()),
                "end": str(block.getEnd()),
                "size": block.getSize(),
                "read": block.isRead(),
                "write": block.isWrite(),
                "execute": block.isExecute(),
            }
            for block in program.getMemory().getBlocks()
        ]
    }


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
    return {
        "address": str(address),
        "size": read,
        "hex": bytes(b & 0xFF for b in buffer[:read]).hex(),
    }


def _operator_delete_entries(program: Any) -> set[str]:
    """Entry addresses of every operator-delete function, thunks included.

    The deleters anchor the destruction-shape rules, so they are read from the
    program's own symbols rather than hardcoded: the MSVC decorated name and
    the demangled spelling both count, and a thunk's entry is what call sites
    actually target.
    """

    entries: set[str] = set()
    iterator = program.getFunctionManager().getFunctions(True)
    while iterator.hasNext():
        function = iterator.next()
        name = function.getName()
        if "operator_delete" in name or name.startswith("??3@"):
            entries.add(str(function.getEntryPoint()))
    return entries


def execute_query(program: Any, command: str, arguments: list[str]) -> dict[str, Any]:
    if command == "listing":
        return _listing(program, arguments[0])
    if command == "decompile":
        return _decompile(program, arguments[0])
    if command in {"xrefs", "xrefs-to", "xrefs-from"}:
        return _xrefs(
            program,
            arguments[0],
            {"xrefs": "both", "xrefs-to": "to", "xrefs-from": "from"}[command],
        )
    if command in {"function", "function-slice"}:
        return {"function": function_metadata(program, _function(program, arguments[0]))}
    if command == "function-of":
        return _function_of(program, arguments[0])
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
    if command == "type-variables":
        from .semantic import field_accesses

        traced = field_accesses(program, arguments[0], arguments[1])
        deleters = _operator_delete_entries(program)
        from ..typevars import derive_type_variables

        return {
            "entry": traced["entry"],
            "root": traced["root"],
            "deleters": sorted(deleters),
            "variables": derive_type_variables(
                traced["entry"], traced["root"], traced["accesses"], traced["calls"], deleters
            ),
        }
    if command == "facts-at":
        from .apply_provenance import facts_at

        return facts_at(program, arguments[0])
    if command == "high-function":
        from .semantic import high_function

        return high_function(program, arguments[0])
    if command == "pcode":
        from .semantic import pcode

        return pcode(program, *arguments)
    if command == "field-accesses":
        from .semantic import field_accesses

        return field_accesses(program, arguments[0], arguments[1])
    if command == "callsite":
        from .semantic import callsite

        return callsite(program, arguments[0])
    if command == "condition-accesses":
        from .semantic import condition_accesses

        return condition_accesses(program, arguments[0])
    if command == "observation-audit":
        from .observation_evidence import audit_observation_evidence

        return audit_observation_evidence(program)
    raise ValueError(f"unsupported query command: {command}")


def validate_query_arguments(command: str, arguments: list[str]) -> None:
    arity = {
        "listing": 1,
        "decompile": 1,
        "xrefs": 1,
        "xrefs-to": 1,
        "xrefs-from": 1,
        "function": 1,
        "function-slice": 1,
        "function-of": 1,
        "read-data": 2,
        "strings": 0,
        "string-refs": 1,
        "search": 1,
        "functions": 0,
        "imports": 0,
        "exports": 0,
        "sections": 0,
        "observation-audit": 0,
        "high-function": 1,
        "field-accesses": 2,
        "callsite": 1,
        "facts-at": 1,
        "type-variables": 2,
        "condition-accesses": 1,
    }
    if command == "pcode":
        if len(arguments) not in {1, 2}:
            raise ValueError("pcode expects an address and an optional style")
        return
    if command not in arity:
        raise ValueError("unknown command; expected one of: " + ", ".join(sorted(arity)))
    if len(arguments) != arity[command]:
        raise ValueError(f"{command} expects {arity[command]} argument(s), got {len(arguments)}")
