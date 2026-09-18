"""Command-local Ghidra inspection: decompile, assembly, and symbol lookup."""

from __future__ import annotations

import hashlib
import re
from pathlib import Path
from typing import Any

from ..config import Settings
from ..paths import atomic_write
from .resolve import (
    ResolveError,
    hex_address,
    identities_at,
    program_address,
    resolve_function,
    resolve_program_selector,
    resolve_symbol,
    source_metadata,
    symbol_record,
)

_TIMEOUT_SECONDS = 120
_PROFILES = ("program", "analysis", "recovery")
_INTERPRET_FORMATS = {"float": "<f", "u32": "<I", "i32": "<i", "u16": "<H", "i16": "<h"}


class DecompileSession:
    """One DecompInterface per (style, profile, c-output) for a single command batch."""

    def __init__(self, program: Any, *, profile: str = "analysis"):
        if profile not in _PROFILES:
            raise ValueError(f"unknown decompiler profile: {profile}")
        self.program = program
        self.profile = profile
        self._interfaces: dict[tuple[str, bool], Any] = {}
        self._results: dict[tuple[str, bool, str], Any] = {}

    def _apply_profile(self, options: Any) -> None:
        if self.profile == "program":
            return
        if self.profile == "analysis":
            options.setInferConstantPointers(True)
            options.setRespectReadOnly(True)
            options.setAnalyzeForLoops(True)
            options.setSplitStructures(True)
            options.setSplitArrays(True)
            options.setSplitPointers(True)
            options.setEliminateUnreachable(True)
            return
        options.setInferConstantPointers(False)
        options.setRespectReadOnly(False)
        options.setAnalyzeForLoops(False)
        options.setSplitStructures(False)
        options.setSplitArrays(False)
        options.setSplitPointers(False)
        options.setEliminateUnreachable(False)

    def interface(self, style: str = "decompile", *, c_output: bool = True) -> Any:
        from ghidra.app.decompiler import DecompileOptions, DecompInterface

        key = (style, c_output)
        existing = self._interfaces.get(key)
        if existing is not None:
            return existing
        options = DecompileOptions()
        if self.profile != "recovery":
            options.grabFromProgram(self.program)
        self._apply_profile(options)
        interface = DecompInterface()
        interface.setOptions(options)
        interface.setSimplificationStyle(style)
        if not c_output:
            interface.toggleCCode(False)
            interface.toggleSyntaxTree(True)
        interface.openProgram(self.program)
        self._interfaces[key] = interface
        return interface

    def decompile(self, function: Any, style: str = "decompile", *, c_output: bool = True) -> Any:
        from ghidra.util.task import TaskMonitor

        key = (style, c_output, str(function.getEntryPoint()))
        if key not in self._results:
            self._results[key] = self.interface(style, c_output=c_output).decompileFunction(
                function, _TIMEOUT_SECONDS, TaskMonitor.DUMMY
            )
        return self._results[key]

    def close(self) -> None:
        import contextlib

        self._results.clear()
        while self._interfaces:
            _, interface = self._interfaces.popitem()
            with contextlib.suppress(Exception):
                interface.dispose()


def function_facts(program: Any, function: Any) -> dict[str, Any]:
    """One instruction traversal: identity, callers, calls, data, and EH facts."""

    from ghidra.program.model.listing import CodeUnit

    references = program.getReferenceManager()
    symbols = program.getSymbolTable()
    listing = program.getListing()
    body = function.getBody()
    instructions = list(listing.getInstructions(body, True))
    raw = bytearray()
    mnemonic_parts = []
    calls: list[dict[str, Any]] = []
    data: list[dict[str, Any]] = []
    vptrs: list[dict[str, Any]] = []
    exception_metadata: list[dict[str, Any]] = []
    strings = []
    for instruction in instructions:
        try:
            raw.extend(instruction.getBytes())
        except Exception:  # noqa: BLE001,S110 - unreadable instruction bytes stay absent
            pass
        operand_types = [
            str(instruction.getOperandType(index)) for index in range(instruction.getNumOperands())
        ]
        mnemonic_parts.append(instruction.getMnemonicString() + ":" + ",".join(operand_types))
        for reference in references.getReferencesFrom(instruction.getAddress()):
            target = reference.getToAddress()
            symbol = symbols.getPrimarySymbol(target)
            name = symbol.getName(True) if symbol is not None else ""
            fact = {
                "site": str(instruction.getAddress()),
                "target": str(target),
                "name": name,
                "instruction": str(instruction),
            }
            if reference.getReferenceType().isCall():
                callee = program.getFunctionManager().getFunctionAt(target)
                if callee is not None:
                    fact["function"] = {
                        "entry": str(callee.getEntryPoint()),
                        "name": callee.getName(True),
                        "prototype": callee.getPrototypeString(False, False),
                    }
                calls.append(fact)
                datum = listing.getDataAt(target)
                if datum is not None and datum.hasStringValue():
                    strings.append(str(datum.getValue()))
                continue
            if not reference.getReferenceType().isData():
                continue
            fact["access"] = reference.getReferenceType().getName()
            datum = listing.getDataAt(target)
            if datum is not None and datum.hasStringValue():
                fact["kind"] = "string"
                strings.append(str(datum.getValue()))
            elif name.startswith("PTR_") and instruction.getMnemonicString().upper() == "CALL":
                fact["kind"] = "import-pointer"
            elif program.getMemory().contains(target):
                fact["kind"] = "program-data"
            else:
                fact["kind"] = "raw"
            data.append(fact)
            folded = name.casefold()
            if "vftable" in folded:
                vptrs.append(dict(fact))
            if any(value in folded for value in ("funcinfo", "unwind", "ehhandler")):
                exception_metadata.append(dict(fact))
    caller_references = list(references.getReferencesTo(function.getEntryPoint()))
    callers = sorted({str(ref.getFromAddress()) for ref in caller_references})
    caller_functions = []
    seen_callers = set()
    for reference in caller_references:
        owner = program.getFunctionManager().getFunctionContaining(reference.getFromAddress())
        key = (str(reference.getFromAddress()), str(owner.getEntryPoint()) if owner else "")
        if key in seen_callers:
            continue
        seen_callers.add(key)
        caller_functions.append(
            {
                "site": key[0],
                "entry": key[1],
                "name": owner.getName(True) if owner else "",
                "prototype": owner.getPrototypeString(False, False) if owner else "",
            }
        )
    return {
        "entry": str(function.getEntryPoint()),
        "size": body.getNumAddresses(),
        "name": function.getName(),
        "qualified_name": function.getName(True),
        "namespace": str(function.getParentNamespace()),
        "thunk": bool(function.isThunk()),
        "thunk_target": str(function.getThunkedFunction(False).getEntryPoint())
        if function.isThunk() and function.getThunkedFunction(False)
        else None,
        "calling_convention": function.getCallingConventionName(),
        "prototype": function.getPrototypeString(False, False),
        "plate_comment": listing.getComment(CodeUnit.PLATE_COMMENT, function.getEntryPoint()),
        "caller_count": len(callers),
        "callers": callers,
        "caller_functions": sorted(caller_functions, key=lambda row: (row["entry"], row["site"])),
        "calls": calls,
        "data_references": data,
        "vptr_references": vptrs,
        "exception_metadata": exception_metadata,
        "referenced_strings": sorted(set(strings), key=str.casefold),
        "raw_body_sha256": hashlib.sha256(raw).hexdigest(),
        "instruction_fingerprint_sha256": hashlib.sha256(
            "\n".join(mnemonic_parts).encode()
        ).hexdigest(),
    }


def _call_facts_from_high(function: Any, high: Any) -> list[dict[str, Any]]:
    if high is None:
        return []
    sites: list[dict[str, Any]] = []
    iterator = high.getPcodeOps()
    while iterator.hasNext():
        operation = iterator.next()
        name = str(operation.getMnemonic())
        if name not in {"CALL", "CALLIND"}:
            continue
        target = operation.getInput(0)
        site = {
            "address": str(operation.getSeqnum().getTarget()),
            "op": name,
        }
        if target is not None and target.isAddress():
            site["target"] = hex_address(target.getOffset())
        high_target = target.getHigh() if target is not None else None
        if high_target is not None and high_target.getSymbol() is not None:
            site["name"] = high_target.getSymbol().getName()
        sites.append(site)
    return sites


def _parameter_records(function: Any, high: Any) -> list[dict[str, Any]]:
    records = []
    for parameter in function.getParameters():
        records.append(
            {
                "name": parameter.getName(),
                "ordinal": parameter.getOrdinal(),
                "type": parameter.getDataType().getDisplayName()
                if parameter.getDataType()
                else None,
                "storage": str(parameter.getVariableStorage())
                if parameter.getVariableStorage() is not None
                else None,
                "auto": bool(parameter.isAutoParameter()),
            }
        )
    if high is None:
        return records
    prototype = high.getFunctionPrototype()
    if prototype is None:
        return records
    high_count = prototype.getNumParams()
    for index in range(high_count):
        symbol = prototype.getParam(index)
        high_name = symbol.getName() if symbol is not None else None
        high_storage = (
            str(symbol.getStorage()) if symbol is not None and symbol.getStorage() else None
        )
        if index < len(records):
            records[index]["high_name"] = high_name
            records[index]["high_storage"] = high_storage
            continue
        records.append(
            {
                "name": high_name,
                "ordinal": index,
                "type": symbol.getDataType().getDisplayName()
                if symbol is not None and symbol.getDataType() is not None
                else None,
                "storage": high_storage,
                "auto": False,
                "high_name": high_name,
                "high_storage": high_storage,
                "source": "high-function",
            }
        )
    return records


def _defects(function: Any, decompiled: str | None, high: Any) -> list[dict[str, str]]:
    defects: list[dict[str, str]] = []
    text = decompiled or ""
    for match in re.finditer(r"\bin_stack_[A-Za-z0-9_]+\b", text):
        defects.append(
            {
                "kind": "phantom-stack-variable",
                "detail": match.group(0),
            }
        )
    if high is not None:
        symbols = high.getLocalSymbolMap().getSymbols()
        while symbols.hasNext():
            symbol = symbols.next()
            name = symbol.getName() or ""
            if name.startswith("in_stack_"):
                defects.append(
                    {
                        "kind": "phantom-stack-variable",
                        "detail": name,
                    }
                )
        prototype = high.getFunctionPrototype()
        stored = function.getParameterCount()
        high_count = prototype.getNumParams() if prototype is not None else None
        if high_count is not None and high_count != stored:
            defects.append(
                {
                    "kind": "parameter-count-mismatch",
                    "detail": f"stored={stored} high={high_count}",
                }
            )
    seen: set[tuple[str, str]] = set()
    unique = []
    for defect in defects:
        key = (defect["kind"], defect["detail"])
        if key in seen:
            continue
        seen.add(key)
        unique.append(defect)
    return unique


def candidate_text_with_defects(text: str | None, defects: list[dict[str, str]]) -> str | None:
    """Keep unresolved analysis defects in the generated candidate itself."""

    if not isinstance(text, str):
        return text
    if not defects:
        return text
    header = "\n".join(f"// defect: {row['kind']}: {row['detail']}" for row in defects)
    return header + "\n\n" + text


def _source_attachment(
    settings: Settings, program_name: str, entry: int, freshness: dict[str, Any]
) -> dict[str, Any]:
    identities = identities_at(settings, program_name, entry)
    if freshness["state"] != "current" and not identities:
        return {
            "state": freshness["state"],
            "detail": freshness.get("detail"),
            "identities": [],
        }
    return {
        "state": freshness["state"],
        "detail": freshness.get("detail"),
        "identities": [
            {
                "name": identity.qualified_name or identity.name,
                "kind": identity.kind,
                "source_file": identity.source_file,
                "line": identity.line,
                "signature": identity.source_signature,
                "calling_convention": identity.calling_convention,
                "definition": identity.is_definition,
                "folded": identity.folded,
            }
            for identity in identities
        ],
    }


def _artifact_dir(settings: Settings, kind: str, program_name: str) -> Path:
    directory = settings.build_dir / "ghidra" / kind / program_name.replace("/", "_")
    directory.mkdir(parents=True, exist_ok=True)
    return directory


def _write_artifact(path: Path, text: str) -> str:
    atomic_write(path, text if text.endswith("\n") else text + "\n")
    return str(path)


def listing_lines(program: Any, function: Any) -> list[str]:
    lines = []
    for instruction in program.getListing().getInstructions(function.getBody(), True):
        annotation = _instruction_annotation(program, instruction)
        rendered = f"{instruction.getAddress()}  {instruction}"
        if annotation:
            rendered = f"{rendered}  ; {annotation}"
        lines.append(rendered)
    return lines


def _instruction_annotation(program: Any, instruction: Any) -> str:
    notes: list[str] = []
    references = program.getReferenceManager().getReferencesFrom(instruction.getAddress())
    for reference in references:
        target = reference.getToAddress()
        try:
            symbol = resolve_symbol(program, hex_address(target))
        except ResolveError:
            continue
        if symbol.kind in {"function", "thunk"}:
            notes.append(f"call {symbol.name or hex_address(symbol.address)}")
        elif symbol.kind == "import-cell":
            notes.append(
                f"iat {symbol.name or hex_address(symbol.defined_at or symbol.address)}"
                + (f" -> {symbol.import_target}" if symbol.import_target else "")
            )
        elif symbol.field:
            residual = f"+{symbol.residual}" if symbol.residual else ""
            notes.append(f"{symbol.name}.{symbol.field}{residual}")
        elif symbol.name:
            notes.append(symbol.name)
    return "; ".join(dict.fromkeys(notes))


def decompile_functions(
    settings: Settings,
    selectors: list[str],
    *,
    program_selector: str = "wiz8",
    include_candidate: bool = True,
) -> dict[str, Any]:
    """Decompile a function batch from native ProgramDB without compiling source."""

    if not selectors:
        raise ValueError("pass at least one function address or name")
    from .env import open_program
    from .workspace import project_seed_freshness, seed_record

    program_name = resolve_program_selector(settings, program_selector)
    freshness = source_metadata(settings, program_name)
    seed = seed_record(settings, program_selector, validate_archive=False)
    seed_origin = project_seed_freshness(settings, seed)
    functions: list[dict[str, Any]] = []
    failures: list[dict[str, Any]] = []
    with open_program(settings, program_selector) as program:
        session = DecompileSession(program, profile="analysis")
        recovered: dict[int, dict[str, Any]] = {}
        try:
            resolved_selectors: list[tuple[str, Any | None, str | None]] = []
            for selector in selectors:
                try:
                    function = resolve_function(program, selector)
                    resolved_selectors.append((selector, function, None))
                except ResolveError as error:
                    resolved_selectors.append((selector, None, str(error)))
            if include_candidate:
                valid = [
                    hex_address(function.getEntryPoint())
                    for _, function, error in resolved_selectors
                    if function is not None and error is None
                ]
                if valid:
                    from .recovery import recover_on_program

                    recovered = recover_on_program(
                        settings, program, valid, program_selector=program_selector
                    )
            for selector, function, error in resolved_selectors:
                if error is not None or function is None:
                    failures.append(
                        {
                            "selector": selector,
                            "status": "missing-function",
                            "error": error or "no function",
                            "candidate": None,
                        }
                    )
                    continue
                entry = int(function.getEntryPoint().getOffset())
                result = session.decompile(function, c_output=True)
                completed = bool(result is not None and result.decompileCompleted())
                rendered = result.getDecompiledFunction() if completed else None
                decompiled = rendered.getC() if rendered is not None else None
                high = result.getHighFunction() if result is not None else None
                facts = function_facts(program, function)
                defects = _defects(function, decompiled, high)
                export = recovered.get(entry) or {}
                for defect in export.get("defects") or []:
                    defects.append({"kind": "recovery", "detail": str(defect)})
                artifact_dir = _artifact_dir(settings, "decompile", program_name)
                c_path = artifact_dir / f"{entry:08x}.c"
                artifacts = {
                    "c": _write_artifact(
                        c_path,
                        decompiled or f"/* decompilation unavailable for {hex_address(entry)} */\n",
                    )
                }
                listing_path = artifact_dir / f"{entry:08x}.asm"
                artifacts["listing"] = _write_artifact(
                    listing_path, "\n".join(listing_lines(program, function))
                )
                candidate_text = candidate_text_with_defects(
                    export.get("generated_code")
                    if isinstance(export.get("generated_code"), str)
                    else None,
                    defects,
                )
                if isinstance(candidate_text, str):
                    artifacts["candidate"] = _write_artifact(
                        artifact_dir / f"{entry:08x}.cpp", candidate_text
                    )
                row = {
                    "selector": selector,
                    "status": "ok" if completed else "decompile-failed",
                    "entry": hex_address(entry),
                    "program": program_name,
                    "native": {
                        **facts,
                        "decompiled": decompiled,
                        "parameters": _parameter_records(function, high),
                        "high_calls": _call_facts_from_high(function, high),
                        "error": None
                        if completed
                        else (result.getErrorMessage() if result is not None else "no result"),
                    },
                    "source": _source_attachment(settings, program_name, entry, freshness),
                    "recovery": {
                        "emission_kind": (export.get("recovery") or {}).get("emission_kind")
                        if isinstance(export.get("recovery"), dict)
                        else None,
                        "source_kind": (export.get("recovery") or {}).get("source_kind")
                        if isinstance(export.get("recovery"), dict)
                        else None,
                        "passes": list((export.get("recovery") or {}).get("passes") or [])
                        if isinstance(export.get("recovery"), dict)
                        else [],
                        "defects": list(export.get("defects") or []),
                    },
                    "defects": defects,
                    "artifacts": {
                        key: str(Path(value).relative_to(settings.repo_dir))
                        if Path(value).is_absolute()
                        else value
                        for key, value in artifacts.items()
                    },
                }
                functions.append(row)
        finally:
            session.close()
    return {
        "schema": "wiz8.ghidra-decompile",
        "program": program_name,
        "seed_origin": {
            "status": seed_origin.get("status"),
            "detail": seed_origin.get("detail"),
        },
        "source_index": freshness,
        "functions": functions,
        "failures": failures,
        "ok": not failures,
    }


def assemble_functions(
    settings: Settings,
    selectors: list[str],
    *,
    program_selector: str = "wiz8",
) -> dict[str, Any]:
    """Annotated assembly for a function batch; does not decompile."""

    if not selectors:
        raise ValueError("pass at least one function address or name")
    from .env import open_program
    from .workspace import project_seed_freshness, seed_record

    program_name = resolve_program_selector(settings, program_selector)
    freshness = source_metadata(settings, program_name)
    seed = seed_record(settings, program_selector, validate_archive=False)
    seed_origin = project_seed_freshness(settings, seed)
    functions: list[dict[str, Any]] = []
    failures: list[dict[str, Any]] = []
    with open_program(settings, program_selector) as program:
        for selector in selectors:
            try:
                function = resolve_function(program, selector)
            except ResolveError as error:
                failures.append(
                    {
                        "selector": selector,
                        "status": "missing-function",
                        "error": str(error),
                        "candidate": None,
                    }
                )
                continue
            entry = int(function.getEntryPoint().getOffset())
            lines = listing_lines(program, function)
            artifact = _write_artifact(
                _artifact_dir(settings, "asm", program_name) / f"{entry:08x}.asm",
                "\n".join(lines),
            )
            functions.append(
                {
                    "selector": selector,
                    "status": "ok",
                    "entry": hex_address(entry),
                    "name": function.getName(True),
                    "source": _source_attachment(settings, program_name, entry, freshness),
                    "listing": lines,
                    "artifacts": {"listing": str(Path(artifact).relative_to(settings.repo_dir))},
                }
            )
    return {
        "schema": "wiz8.ghidra-asm",
        "program": program_name,
        "seed_origin": {
            "status": seed_origin.get("status"),
            "detail": seed_origin.get("detail"),
        },
        "source_index": freshness,
        "functions": functions,
        "failures": failures,
        "ok": not failures,
    }


def lookup_symbols(
    settings: Settings,
    selectors: list[str],
    *,
    program_selector: str = "wiz8",
    interpret: str | None = None,
) -> dict[str, Any]:
    """Identity/data lookup for addresses; does not decompile."""

    if not selectors:
        raise ValueError("pass at least one address")
    if interpret is not None and interpret not in _INTERPRET_FORMATS:
        raise ValueError("--as must be float, u32, i32, u16, or i16")
    from .env import open_program
    from .workspace import project_seed_freshness, seed_record

    program_name = resolve_program_selector(settings, program_selector)
    freshness = source_metadata(settings, program_name)
    seed = seed_record(settings, program_selector, validate_archive=False)
    seed_origin = project_seed_freshness(settings, seed)
    symbols: list[dict[str, Any]] = []
    failures: list[dict[str, Any]] = []
    with open_program(settings, program_selector) as program:
        for selector in selectors:
            try:
                symbol = resolve_symbol(program, selector)
            except ResolveError as error:
                failures.append({"selector": selector, "error": str(error)})
                continue
            row = symbol_record(symbol)
            row["selector"] = selector
            row["source"] = _source_attachment(settings, program_name, symbol.address, freshness)
            if interpret is not None:
                row["interpretation"] = _interpret_bytes(
                    program, symbol.address, interpret, length=symbol.length
                )
            symbols.append(row)
    return {
        "schema": "wiz8.ghidra-sym",
        "program": program_name,
        "seed_origin": {
            "status": seed_origin.get("status"),
            "detail": seed_origin.get("detail"),
        },
        "source_index": freshness,
        "symbols": symbols,
        "failures": failures,
        "ok": not failures,
    }


def function_inventory(settings: Settings, selector: str = "wiz8") -> list[dict[str, str]]:
    from .env import open_program

    with open_program(settings, selector) as program:
        functions = []
        iterator = program.getFunctionManager().getFunctions(True)
        while iterator.hasNext():
            function = iterator.next()
            if not function.isExternal():
                functions.append(
                    {
                        "entry": hex_address(function.getEntryPoint()),
                        "name": function.getName(True),
                    }
                )
        return functions


def containing_functions(
    settings: Settings, addresses: list[int], selector: str = "wiz8"
) -> dict[int, int | None]:
    """Native function entries that contain the given addresses, if any."""

    from .env import open_program

    if not addresses:
        return {}
    with open_program(settings, selector) as program:
        manager = program.getFunctionManager()
        found: dict[int, int | None] = {}
        for address in addresses:
            try:
                target = program_address(program, hex_address(address))
            except ResolveError:
                found[address] = None
                continue
            function = manager.getFunctionContaining(target)
            found[address] = (
                int(function.getEntryPoint().getOffset()) if function is not None else None
            )
        return found


def validate_function_entries(
    settings: Settings, entries: set[int], selector: str = "wiz8"
) -> dict[str, Any]:
    from .env import open_program

    with open_program(settings, selector) as program:
        manager = program.getFunctionManager()
        missing = [
            hex_address(entry)
            for entry in sorted(entries)
            if manager.getFunctionAt(program_address(program, hex_address(entry))) is None
        ]
        return {"schema": "wiz8.function-existence-audit", "ok": not missing, "missing": missing}


def _interpret_bytes(
    program: Any, address: int, interpret: str, *, length: int | None = None
) -> dict[str, Any]:
    import struct

    size = struct.calcsize(_INTERPRET_FORMATS[interpret])
    wanted = max(int(length or size), size)
    raw = b""
    try:
        import jpype

        target = program_address(program, hex_address(address))
        buffer = jpype.JArray(jpype.JByte)(wanted)
        read = program.getMemory().getBytes(target, buffer)
        raw = bytes(b & 0xFF for b in buffer[:read])
    except Exception:  # noqa: BLE001
        raw = b""
    if len(raw) < size:
        raise ValueError(f"{hex_address(address)} has fewer than {size} readable bytes")
    return {
        "type": interpret,
        "value": struct.unpack(_INTERPRET_FORMATS[interpret], raw[:size])[0],
        "hex": raw[:size].hex(),
    }


def data_facts(
    settings: Settings, entries: set[int], selector: str = "wiz8"
) -> list[dict[str, Any]]:
    from .env import open_program

    with open_program(settings, selector) as program:
        facts = []
        for entry in sorted(entries):
            symbol = resolve_symbol(program, hex_address(entry))
            row = symbol_record(symbol)
            address = program_address(program, hex_address(entry))
            size = max(int(symbol.length or 4), 8)
            buffer = None
            try:
                import jpype

                buffer = jpype.JArray(jpype.JByte)(size)
                read = program.getMemory().getBytes(address, buffer)
                row["hex"] = bytes(b & 0xFF for b in buffer[:read]).hex()
            except Exception:  # noqa: BLE001
                row["hex"] = ""
            facts.append(row)
        return facts


def class_report(program: Any, names: list[str]) -> dict[str, Any]:
    from ..class_binding import find_class_structure, find_ghidra_class

    classes = []
    for name in names:
        data_type = None
        ghidra_class = find_ghidra_class(program, name)
        if ghidra_class is not None:
            data_type = find_class_structure(program, ghidra_class)
        if data_type is None:
            manager = program.getDataTypeManager()
            simple = name.split("::")[-1]
            for path in (f"/{simple}", f"/{name}", f"/Demangler/{simple}"):
                data_type = manager.getDataType(path)
                if data_type is not None:
                    break
        fields = []
        path = str(data_type.getPathName()) if data_type is not None else None
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
        classes.append({"name": name, "path": path, "fields": fields})
    tables = []
    symbols = program.getSymbolTable().getAllSymbols(True)
    while symbols.hasNext():
        symbol = symbols.next()
        symbol_name = symbol.getName(True)
        if "vftable" not in symbol_name.casefold() or not any(
            value + "::" in symbol_name for value in names
        ):
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
        tables.append(
            {"name": symbol_name, "address": str(symbol.getAddress()), "references": references}
        )
    return {"schema": "wiz8.class-report", "classes": classes, "vtables": tables}
