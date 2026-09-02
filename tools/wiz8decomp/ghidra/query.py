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


def _hex_address(address: Any) -> str:
    return f"0x{address.getOffset():08x}"


def _function(program: Any, text: str) -> Any:
    manager = program.getFunctionManager()
    try:
        address = _address(program, text)
    except ValueError:
        matches = []
        iterator = manager.getFunctions(True)
        while iterator.hasNext():
            candidate = iterator.next()
            if text in {candidate.getName(), candidate.getName(True)}:
                matches.append(candidate)
        if not matches:
            raise ValueError(f"unknown function selector: {text}") from None
        if len(matches) > 1:
            candidates = ", ".join(
                f"{item.getName(True)} at {item.getEntryPoint()}" for item in matches[:8]
            )
            raise ValueError(f"ambiguous function selector {text!r}; candidates: {candidates}")
        return matches[0]
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
    return {"listing": "\n".join(lines)}


def _decompile(program: Any, argument: str) -> dict[str, Any]:
    from .semantic import decompile_c

    function = _function(program, argument)
    return decompile_c(program, function)


def _function_facts(program: Any, argument: str) -> dict[str, Any]:
    function = _function(program, argument)
    symbols = program.getSymbolTable()
    calls: list[dict[str, Any]] = []
    data: list[dict[str, Any]] = []
    vptrs: list[dict[str, Any]] = []
    exception_metadata: list[dict[str, Any]] = []
    listing = program.getListing()
    instructions = program.getListing().getInstructions(function.getBody(), True)
    while instructions.hasNext():
        instruction = instructions.next()
        for reference in instruction.getReferencesFrom():
            symbol = symbols.getPrimarySymbol(reference.getToAddress())
            name = symbol.getName(True) if symbol is not None else ""
            fact = {
                "site": str(instruction.getAddress()),
                "target": str(reference.getToAddress()),
                "name": name,
                "instruction": str(instruction),
            }
            if reference.getReferenceType().isCall():
                calls.append(fact)
            elif reference.getReferenceType().isData():
                fact["access"] = reference.getReferenceType().getName()
                datum = listing.getDataAt(reference.getToAddress())
                if datum is not None and datum.hasStringValue():
                    fact["kind"] = "string"
                elif name.startswith("PTR_") and instruction.getMnemonicString().upper() == "CALL":
                    fact["kind"] = "import-pointer"
                elif program.getMemory().contains(reference.getToAddress()):
                    fact["kind"] = "program-data"
                else:
                    fact["kind"] = "raw"
                data.append(fact)
                folded = name.casefold()
                if "vftable" in folded:
                    vptrs.append(dict(fact))
                if any(value in folded for value in ("funcinfo", "unwind", "ehhandler")):
                    exception_metadata.append(dict(fact))
    return {
        "entry": str(function.getEntryPoint()),
        "name": function.getName(True),
        "calls": calls,
        "data_references": data,
        "vptr_references": vptrs,
        "exception_metadata": exception_metadata,
    }


def _class_fields(program: Any, names: list[str]) -> dict[str, Any]:
    manager = program.getDataTypeManager()
    classes = []
    for name in names:
        data_type = manager.getDataType(f"/wiz8/classes/{name}")
        fields = []
        if data_type is not None and hasattr(data_type, "getDefinedComponents"):
            for component in data_type.getDefinedComponents():
                if component.getFieldName() is not None:
                    fields.append(
                        {
                            "field": component.getFieldName(),
                            "offset": component.getOffset(),
                            "length": component.getLength(),
                            "type": component.getDataType().getDisplayName(),
                        }
                    )
        classes.append({"name": name, "fields": fields})
    return {"classes": classes}


def _class_facts(program: Any, names: list[str]) -> dict[str, Any]:
    tables = []
    symbols = program.getSymbolTable().getAllSymbols(True)
    while symbols.hasNext():
        symbol = symbols.next()
        name = symbol.getName(True)
        if "vftable" not in name.casefold() or not any(value + "::" in name for value in names):
            continue
        references = []
        iterator = program.getReferenceManager().getReferencesTo(symbol.getAddress())
        while iterator.hasNext():
            reference = iterator.next()
            instruction = program.getListing().getInstructionContaining(reference.getFromAddress())
            owner = program.getFunctionManager().getFunctionContaining(reference.getFromAddress())
            references.append(
                {
                    "from": str(reference.getFromAddress()),
                    "kind": reference.getReferenceType().getName(),
                    "instruction": str(instruction) if instruction is not None else "",
                    "function": str(owner.getEntryPoint()) if owner is not None else "",
                }
            )
        tables.append({"name": name, "address": str(symbol.getAddress()), "references": references})
    return {"schema": "wiz8.class-facts", "vtables": tables}


def _indirect_calls(program: Any, argument: str) -> dict[str, Any]:
    from .semantic import callsite, pcode

    normalized = pcode(program, argument, "normalize")
    sites = sorted(
        {
            str(operation["address"])
            for operation in normalized.get("operations", [])
            if operation.get("op") == "CALLIND"
        }
    )
    return {
        "calls": [call for site in sites for call in callsite(program, site).get("sites", [])],
        "normalized_pcode": normalized,
    }


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


def _function_inventory(program: Any) -> dict[str, Any]:
    functions = []
    iterator = program.getFunctionManager().getFunctions(True)
    while iterator.hasNext():
        function = iterator.next()
        if not function.isExternal():
            functions.append(
                {"entry": _hex_address(function.getEntryPoint()), "name": function.getName(True)}
            )
    return {"schema": "wiz8.function-inventory", "functions": functions}


def _function_exists(program: Any, entries: list[str]) -> dict[str, Any]:
    manager = program.getFunctionManager()
    missing = [
        entry for entry in entries if manager.getFunctionAt(_address(program, entry)) is None
    ]
    return {"schema": "wiz8.function-existence-audit", "ok": not missing, "missing": missing}


def _data_facts(program: Any, entries: list[str]) -> dict[str, Any]:
    facts = []
    for entry in entries:
        requested = _address(program, entry)
        data = program.getListing().getDataContaining(requested)
        fact: dict[str, Any] = {"address": _hex_address(requested)}
        if data is not None:
            fact.update(
                {
                    "defined_at": _hex_address(data.getAddress()),
                    "length": data.getLength(),
                    "type": data.getDataType().getDisplayName(),
                }
            )
        symbol = program.getSymbolTable().getPrimarySymbol(requested)
        fact["name"] = symbol.getName(True) if symbol is not None else ""
        fact["references"] = [
            {
                "from": _hex_address(reference.getFromAddress()),
                "kind": reference.getReferenceType().getName(),
            }
            for reference in program.getReferenceManager().getReferencesTo(requested)
        ]
        facts.append(fact)
    return {"schema": "wiz8.data-facts", "data": facts}


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
    if command == "function":
        return {"function": function_metadata(program, _function(program, arguments[0]))}
    if command == "function-facts":
        return _function_facts(program, arguments[0])
    if command == "class-fields":
        return _class_fields(program, arguments)
    if command == "class-facts":
        return _class_facts(program, arguments)
    if command == "indirect-calls":
        return _indirect_calls(program, arguments[0])
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
    if command == "function-inventory":
        return _function_inventory(program)
    if command == "function-exists":
        return _function_exists(program, arguments)
    if command == "data-facts":
        return _data_facts(program, arguments)
    if command == "imports":
        return _symbols(program, True)
    if command == "exports":
        return _symbols(program, False)
    if command == "sections":
        return _sections(program)
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
    raise ValueError(f"unsupported query command: {command}")


def validate_query_arguments(command: str, arguments: list[str]) -> None:
    arity = {
        "listing": 1,
        "decompile": 1,
        "xrefs": 1,
        "xrefs-to": 1,
        "xrefs-from": 1,
        "function": 1,
        "function-facts": 1,
        "indirect-calls": 1,
        "function-of": 1,
        "read-data": 2,
        "strings": 0,
        "string-refs": 1,
        "search": 1,
        "functions": 0,
        "function-inventory": 0,
        "imports": 0,
        "exports": 0,
        "sections": 0,
        "high-function": 1,
        "field-accesses": 2,
        "callsite": 1,
        "condition-accesses": 1,
    }
    if command in {"class-facts", "class-fields", "function-exists", "data-facts"}:
        if not arguments:
            raise ValueError(f"{command} expects one or more class names")
        return
    if command == "pcode":
        if len(arguments) not in {1, 2}:
            raise ValueError("pcode expects an address and an optional style")
        return
    if command not in arity:
        raise ValueError("unknown command; expected one of: " + ", ".join(sorted(arity)))
    if len(arguments) != arity[command]:
        raise ValueError(f"{command} expects {arity[command]} argument(s), got {len(arguments)}")


def query_many(
    settings: Any,
    selector: str,
    queries: list[tuple[str, list[str]]],
    *,
    function_seeds: list[str] | None = None,
) -> tuple[list[dict[str, Any]], str]:
    """Run an ordered query batch in one ordinary short-lived project owner."""

    if not queries:
        raise ValueError("at least one query is required")
    for command, arguments in queries:
        validate_query_arguments(command, arguments)
    from .env import open_program

    with open_program(settings, selector) as program:
        transaction = None
        try:
            if function_seeds:
                from ghidra.app.cmd.disassemble import DisassembleCommand
                from ghidra.app.cmd.function import CreateFunctionCmd

                transaction = program.startTransaction("disposable function seeds")
                for seed in function_seeds:
                    address = _address(program, seed)
                    if program.getFunctionManager().getFunctionContaining(address) is not None:
                        continue
                    if program.getListing().getInstructionAt(
                        address
                    ) is None and not DisassembleCommand(address, None, True).applyTo(program):
                        raise ValueError(f"could not disassemble function seed {address}")
                    if not CreateFunctionCmd(address).applyTo(program):
                        raise ValueError(f"could not create function seed {address}")
            results = [
                {
                    "command": command,
                    "arguments": arguments,
                    "result": execute_query(program, command, arguments),
                }
                for command, arguments in queries
            ]
        finally:
            from .semantic import dispose_sessions

            dispose_sessions()
            if transaction is not None:
                program.endTransaction(transaction, False)
    return results, "pyghidra"


def query(
    settings: Any,
    selector: str,
    command: str,
    arguments: list[str],
    *,
    function_seeds: list[str] | None = None,
) -> tuple[dict[str, Any], str]:
    results, transport = query_many(
        settings, selector, [(command, arguments)], function_seeds=function_seeds
    )
    return results[0]["result"], transport


def function_inventory(settings: Any, selector: str = "wiz8") -> list[dict[str, str]]:
    result, _ = query(settings, selector, "function-inventory", [])
    return list(result["functions"])


def validate_function_entries(
    settings: Any, entries: set[int], selector: str = "wiz8"
) -> dict[str, Any]:
    arguments = [f"0x{entry:08x}" for entry in sorted(entries)]
    result, _ = query(settings, selector, "function-exists", arguments)
    return result


def data_facts(settings: Any, entries: set[int], selector: str = "wiz8") -> list[dict[str, Any]]:
    arguments = [f"0x{entry:08x}" for entry in sorted(entries)]
    result, _ = query(settings, selector, "data-facts", arguments)
    return list(result["data"])
