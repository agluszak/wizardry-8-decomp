"""Command-local Ghidra inspection: decompile, assembly, and symbol lookup."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path
from typing import Any

from ..config import Settings
from ..paths import atomic_write
from .resolve import (
    ResolveError,
    hex_address,
    identities_for_target,
    parse_address_span,
    program_address,
    resolve_function,
    resolve_function_entries,
    resolve_program_selector,
    resolve_symbol,
    source_metadata,
    symbol_record,
)

_TIMEOUT_SECONDS = 120
_PROFILES = ("program", "analysis", "recovery")
_INTERPRET_FORMATS = {"float": "<f", "u32": "<I", "i32": "<i", "u16": "<H", "i16": "<h"}
_DEFAULT_WINDOW = 64
_VFPTR_FIELD_NAMES = frozenset({"vfptr", "vptr", "vftable", "__vftable"})
_VBPTR_FIELD_NAMES = frozenset({"vbptr", "vbtable", "__vbtable"})


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


_GHIDRA_TYPE_ALIASES = {
    "uchar": "unsignedchar",
    "ushort": "unsignedshort",
    "uint": "unsignedint",
    "ulong": "unsignedlong",
    "byte": "unsignedchar",
    "bool": "bool",
}


def _types_agree(left: str | None, right: str | None) -> bool:
    if not left or not right:
        return False
    first = left.replace(" ", "").casefold()
    second = right.replace(" ", "").casefold()
    if first == second:
        return True
    return _GHIDRA_TYPE_ALIASES.get(first, first) == _GHIDRA_TYPE_ALIASES.get(second, second)


def _defects(
    function: Any,
    decompiled: str | None,
    high: Any,
    source_identities: tuple[Any, ...] = (),
) -> list[dict[str, str]]:
    defects: list[dict[str, str]] = []
    text = decompiled or ""
    for match in re.finditer(r"\bin_stack_[A-Za-z0-9_]+\b", text):
        defects.append(
            {
                "kind": "phantom-stack-variable",
                "detail": match.group(0),
            }
        )
    stored_explicit = 0
    stored_all = 0
    if hasattr(function, "getParameters"):
        parameters = list(function.getParameters())
        stored_all = len(parameters)
        stored_explicit = sum(
            1
            for parameter in parameters
            if not (hasattr(parameter, "isAutoParameter") and parameter.isAutoParameter())
        )
    elif hasattr(function, "getParameterCount"):
        stored_all = stored_explicit = int(function.getParameterCount())
    high_count = None
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
        high_count = prototype.getNumParams() if prototype is not None else None
        if high_count is not None and high_count != stored_all:
            defects.append(
                {
                    "kind": "parameter-count-mismatch",
                    "detail": f"stored={stored_all} high={high_count}",
                }
            )
    source_identity = next(
        (
            identity
            for identity in source_identities
            if getattr(identity, "kind", None) in {"definition", "declaration"}
        ),
        None,
    )
    if source_identity is not None:
        parameter_types = tuple(getattr(source_identity, "parameter_types", None) or ())
        source_explicit = len(parameter_types)
        source_signature = getattr(source_identity, "source_signature", None)
        qualified = getattr(source_identity, "qualified_name", None) or getattr(
            source_identity, "name", None
        )
        if not source_signature and not parameter_types:
            defects.append(
                {
                    "kind": "source-signature-unresolved",
                    "detail": (
                        f"{qualified or 'source identity'} "
                        "has no source signature or explicit parameter types."
                    ),
                }
            )
        if stored_explicit == 0 and source_explicit > 0:
            defects.append(
                {
                    "kind": "programdb-prototype-empty",
                    "detail": (
                        f"Source declaration: {source_explicit} explicit arguments. "
                        "ProgramDB prototype: 0 explicit arguments. "
                        "Argument recovery is inconsistent; do not infer argument order from this C."
                    ),
                }
            )
        elif source_explicit != stored_explicit:
            defects.append(
                {
                    "kind": "source-parameter-count-mismatch",
                    "detail": (
                        f"Source declaration: {source_explicit} explicit arguments. "
                        f"ProgramDB prototype: {stored_explicit} explicit arguments. "
                        "Argument recovery is inconsistent; do not infer argument order from this C."
                    ),
                }
            )
        source_cc = getattr(source_identity, "calling_convention", None) or (
            "__thiscall" if getattr(source_identity, "has_this", False) else None
        )
        stored_cc = None
        if hasattr(function, "getCallingConventionName"):
            stored_cc = str(function.getCallingConventionName() or "")
        if source_cc and stored_cc and source_cc != stored_cc:
            defects.append(
                {
                    "kind": "source-calling-convention-mismatch",
                    "detail": f"source={source_cc} programdb={stored_cc}",
                }
            )
        source_return = getattr(source_identity, "return_type", None)
        stored_return = None
        if hasattr(function, "getReturnType") and function.getReturnType() is not None:
            ret = function.getReturnType()
            stored_return = ret.getDisplayName() if hasattr(ret, "getDisplayName") else str(ret)
        if source_return and stored_return and not _types_agree(source_return, stored_return):
            defects.append(
                {
                    "kind": "source-return-type-mismatch",
                    "detail": f"source={source_return} programdb={stored_return}",
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
    identities: dict[int, tuple[Any, ...]],
    freshness: dict[str, Any],
    entry: int,
) -> dict[str, Any]:
    found = identities.get(entry, ())
    if freshness.get("state") != "current" and not found:
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
            }
            for identity in found
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
    return listing_range(
        program,
        int(function.getBody().getMinAddress().getOffset()),
        int(function.getBody().getMaxAddress().getOffset()),
        body=function.getBody(),
    )


def listing_range(
    program: Any,
    start: int,
    end: int,
    *,
    body: Any | None = None,
) -> list[str]:
    listing = program.getListing()
    if body is None:
        start_addr = program_address(program, hex_address(start))
        end_addr = program_address(program, hex_address(end))
        from ghidra.program.model.address import AddressSet

        body = AddressSet(start_addr, end_addr)
    lines = []
    for instruction in listing.getInstructions(body, True):
        annotation = _instruction_annotation(program, instruction)
        rendered = f"{instruction.getAddress()}  {instruction}"
        if annotation:
            rendered = f"{rendered}  ; {annotation}"
        lines.append(rendered)
    if lines:
        return lines
    return _hex_window(program, start, end)


def _hex_window(program: Any, start: int, end: int) -> list[str]:
    wanted = max(end - start + 1, 1)
    raw = b""
    try:
        import jpype

        target = program_address(program, hex_address(start))
        buffer = jpype.JArray(jpype.JByte)(wanted)
        read = program.getMemory().getBytes(target, buffer)
        raw = bytes(b & 0xFF for b in buffer[:read])
    except Exception:  # noqa: BLE001
        raw = b""
    if not raw:
        return [f"{hex_address(start)}  ; no defined instructions or readable bytes"]
    lines = [f"{hex_address(start)}  ; raw bytes, ProgramDB has no instruction units"]
    for index in range(0, len(raw), 16):
        chunk = raw[index : index + 16]
        lines.append(f"{hex_address(start + index)}  {chunk.hex()}")
    return lines


def bounded_window(program: Any, start: int, end: int | None = None) -> tuple[int, int]:
    finish = end if end is not None and end != start else start + _DEFAULT_WINDOW - 1
    try:
        address = program_address(program, hex_address(start))
        block = program.getMemory().getBlock(address)
        if block is not None:
            finish = min(finish, int(block.getEnd().getOffset()))
    except Exception:  # noqa: BLE001,S110 - window still usable without a memory block
        pass
    return start, max(start, finish)


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
        elif symbol.access_path:
            residual = f"+{symbol.residual}" if symbol.residual else ""
            notes.append(f"{symbol.access_path}{residual}")
        elif symbol.union_members:
            notes.append("union " + " | ".join(symbol.union_members))
        elif symbol.field:
            residual = f"+{symbol.residual}" if symbol.residual else ""
            notes.append(f"{symbol.name}.{symbol.field}{residual}")
        elif symbol.name:
            notes.append(symbol.name)
    return "; ".join(dict.fromkeys(notes))


def _relative_artifact(settings: Settings, path: str) -> str:
    value = Path(path)
    return str(value.relative_to(settings.repo_dir)) if value.is_absolute() else path


def _inspect_targets(program: Any, selectors: list[str]) -> list[dict[str, Any]]:
    targets: list[dict[str, Any]] = []
    for selector in selectors:
        span = parse_address_span(selector)
        if span is not None and span[0] != span[1]:
            try:
                for entry in resolve_function_entries(program, [selector]):
                    function = program.getFunctionManager().getFunctionAt(
                        program_address(program, hex_address(entry))
                    )
                    targets.append(
                        {
                            "selector": selector,
                            "function": function,
                            "window": None,
                            "error": None,
                        }
                    )
                continue
            except ResolveError as error:
                start, end = bounded_window(program, span[0], span[1])
                targets.append(
                    {
                        "selector": selector,
                        "function": None,
                        "window": (start, end),
                        "error": str(error),
                    }
                )
                continue
        try:
            function = resolve_function(program, selector)
        except ResolveError as error:
            window = None
            if span is not None:
                try:
                    window = bounded_window(program, span[0])
                except Exception:  # noqa: BLE001
                    window = (span[0], span[0])
            targets.append(
                {
                    "selector": selector,
                    "function": None,
                    "window": window,
                    "error": str(error),
                }
            )
            continue
        targets.append(
            {
                "selector": selector,
                "function": function,
                "window": None,
                "error": None,
            }
        )
    return targets


def decompile_functions(
    settings: Settings,
    selectors: list[str],
    *,
    program_selector: str = "wiz8",
    include_candidate: bool = False,
) -> dict[str, Any]:
    """Decompile a function batch from native ProgramDB without compiling source."""

    if not selectors:
        raise ValueError("pass at least one function address or name")
    from .env import open_program
    from .workspace import project_seed_freshness, seed_record

    program_name = resolve_program_selector(settings, program_selector)
    freshness = source_metadata(settings, program_name)
    identities = identities_for_target(settings, program_name)
    seed = seed_record(settings, program_selector, validate_archive=False)
    seed_origin = project_seed_freshness(settings, seed)
    functions: list[dict[str, Any]] = []
    failures: list[dict[str, Any]] = []
    with open_program(settings, program_selector) as program:
        session = DecompileSession(program, profile="analysis")
        recovered: dict[int, dict[str, Any]] = {}
        try:
            targets = _inspect_targets(program, selectors)
            valid = [
                hex_address(target["function"].getEntryPoint())
                for target in targets
                if include_candidate and target.get("function") is not None
            ]
            recovery_error = None
            if valid:
                try:
                    from .recovery import recover_on_program

                    recovered = recover_on_program(
                        settings, program, valid, program_selector=program_selector
                    )
                except Exception as error:  # noqa: BLE001 - native C is still useful
                    recovery_error = str(error)
                    recovered = {}
            for target in targets:
                selector = target["selector"]
                function = target.get("function")
                window = target.get("window")
                if function is None:
                    lines: list[str] = []
                    artifacts: dict[str, str] = {}
                    if window is not None:
                        try:
                            lines = listing_range(program, window[0], window[1])
                            artifacts["listing"] = _relative_artifact(
                                settings,
                                _write_artifact(
                                    _artifact_dir(settings, "decompile", program_name)
                                    / f"{window[0]:08x}.asm",
                                    "\n".join(lines),
                                ),
                            )
                        except Exception:  # noqa: BLE001 - keep the missing-function error
                            lines = []
                    failure = {
                        "selector": selector,
                        "status": "missing-function",
                        "error": target.get("error") or "no function",
                        "listing": lines,
                        "artifacts": artifacts,
                        "candidate": None,
                    }
                    failures.append(failure)
                    functions.append(failure)
                    continue
                entry = int(function.getEntryPoint().getOffset())
                result = session.decompile(function, c_output=True)
                completed = bool(result is not None and result.decompileCompleted())
                rendered = result.getDecompiledFunction() if completed else None
                decompiled = rendered.getC() if rendered is not None else None
                high = result.getHighFunction() if result is not None else None
                facts = function_facts(program, function)
                source = _source_attachment(identities, freshness, entry)
                source_idents = identities.get(entry, ())
                defects = _defects(function, decompiled, high, source_idents)
                export = recovered.get(entry) or {}
                for defect in export.get("defects") or []:
                    defects.append({"kind": "recovery", "detail": str(defect)})
                artifact_dir = _artifact_dir(settings, "decompile", program_name)
                listing = listing_lines(program, function)
                rewrite_failed = bool(include_candidate and recovery_error)
                if (
                    include_candidate
                    and completed
                    and not isinstance(export.get("generated_code"), str)
                ):
                    rewrite_failed = True
                if rewrite_failed:
                    defects.append(
                        {
                            "kind": "recovery-unavailable",
                            "detail": recovery_error or "Java recovery exporter failed",
                        }
                    )
                presented = candidate_text_with_defects(decompiled, defects)
                artifacts = {
                    "c": _write_artifact(
                        artifact_dir / f"{entry:08x}.c",
                        presented
                        or decompiled
                        or f"/* decompilation unavailable for {hex_address(entry)} */\n",
                    ),
                    "listing": _write_artifact(
                        artifact_dir / f"{entry:08x}.asm", "\n".join(listing)
                    ),
                }
                if include_candidate:
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
                if completed:
                    status = "ok"
                else:
                    status = "decompile-failed"
                relative = {
                    key: _relative_artifact(settings, value) for key, value in artifacts.items()
                }
                row = {
                    "selector": selector,
                    "status": status,
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
                    "source": source,
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
                        "error": recovery_error,
                    },
                    "defects": defects,
                    "listing": listing,
                    "artifacts": relative,
                }
                row["artifacts"]["result"] = _relative_artifact(
                    settings,
                    _write_artifact(
                        artifact_dir / f"{entry:08x}.json",
                        json.dumps(row, indent=2, ensure_ascii=False),
                    ),
                )
                functions.append(row)
                if status != "ok":
                    failures.append(
                        {
                            "selector": selector,
                            "status": status,
                            "error": row["native"]["error"] or recovery_error or status,
                            "entry": hex_address(entry),
                            "artifacts": row["artifacts"],
                        }
                    )
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
    """Annotated assembly for a function or bounded address window; does not decompile."""

    if not selectors:
        raise ValueError("pass at least one function address or name")
    from .env import open_program
    from .workspace import project_seed_freshness, seed_record

    program_name = resolve_program_selector(settings, program_selector)
    freshness = source_metadata(settings, program_name)
    identities = identities_for_target(settings, program_name)
    seed = seed_record(settings, program_selector, validate_archive=False)
    seed_origin = project_seed_freshness(settings, seed)
    functions: list[dict[str, Any]] = []
    failures: list[dict[str, Any]] = []
    with open_program(settings, program_selector) as program:
        for target in _inspect_targets(program, selectors):
            selector = target["selector"]
            function = target.get("function")
            window = target.get("window")
            if function is not None:
                entry = int(function.getEntryPoint().getOffset())
                lines = listing_lines(program, function)
                name = function.getName(True)
                source = _source_attachment(identities, freshness, entry)
                status = "ok"
                error = None
            elif window is not None:
                entry = window[0]
                try:
                    lines = listing_range(program, window[0], window[1])
                except Exception:  # noqa: BLE001
                    lines = []
                name = ""
                source = _source_attachment(identities, freshness, entry)
                status = "no-function"
                error = target.get("error")
            else:
                failures.append(
                    {
                        "selector": selector,
                        "status": "missing-function",
                        "error": target.get("error") or "no function",
                        "candidate": None,
                    }
                )
                continue
            artifact = _write_artifact(
                _artifact_dir(settings, "asm", program_name) / f"{entry:08x}.asm",
                "\n".join(lines),
            )
            row = {
                "selector": selector,
                "status": status,
                "entry": hex_address(entry),
                "name": name,
                "source": source,
                "listing": lines,
                "error": error,
                "artifacts": {"listing": _relative_artifact(settings, artifact)},
            }
            functions.append(row)
            if status != "ok":
                failures.append(
                    {
                        "selector": selector,
                        "status": status,
                        "error": error or status,
                        "entry": hex_address(entry),
                        "listing": lines,
                        "artifacts": row["artifacts"],
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
    identities = identities_for_target(settings, program_name)
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
            row["source"] = _source_attachment(identities, freshness, symbol.address)
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
    from ..vftable_typing import parse_subobject_view

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
        unknown = []
        bases = []
        size = (
            int(data_type.getLength())
            if data_type is not None and hasattr(data_type, "getLength")
            else None
        )
        path = str(data_type.getPathName()) if data_type is not None else None
        covered: list[tuple[int, int]] = []
        if data_type is not None and hasattr(data_type, "getDefinedComponents"):
            for component in data_type.getDefinedComponents():
                offset = int(component.getOffset())
                length = int(component.getLength())
                covered.append((offset, offset + length))
                nested = component.getDataType()
                nested_path = (
                    str(nested.getPathName())
                    if nested is not None and hasattr(nested, "getPathName")
                    else ""
                )
                description = (
                    nested.getDescription()
                    if nested is not None and hasattr(nested, "getDescription")
                    else None
                )
                view = parse_subobject_view(description)
                field_name = component.getFieldName()
                record = {
                    "field": field_name,
                    "offset": offset,
                    "length": length,
                    "type": nested.getDisplayName() if nested is not None else None,
                }
                if field_name is not None:
                    fields.append(record)
                if view or (
                    field_name and (field_name == "base" or str(field_name).startswith("base_"))
                ):
                    bases.append(
                        {
                            **record,
                            "base": (view or {}).get("base"),
                            "view_offset": (view or {}).get("offset"),
                            "view_path": nested_path or None,
                            "vtable": (view or {}).get("vtable"),
                        }
                    )
            cursor = 0
            for start, stop in sorted(covered):
                if start > cursor:
                    unknown.append({"offset": cursor, "length": start - cursor})
                cursor = max(cursor, stop)
            if size is not None and cursor < size:
                unknown.append({"offset": cursor, "length": size - cursor})
        classes.append(
            {
                "name": name,
                "path": path,
                "size": size,
                "fields": fields,
                "unknown": unknown,
                "bases": bases,
                "vfptrs": [field for field in fields if field.get("field") in _VFPTR_FIELD_NAMES],
                "vbptrs": [field for field in fields if field.get("field") in _VBPTR_FIELD_NAMES],
            }
        )
    tables = []
    symbols = program.getSymbolTable().getAllSymbols(True)
    while symbols.hasNext():
        symbol = symbols.next()
        symbol_name = symbol.getName(True)
        if "vftable" not in symbol_name.casefold() or not any(
            value + "::" in symbol_name or symbol_name.endswith(value) for value in names
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
        slots = []
        data = program.getListing().getDataAt(symbol.getAddress())
        table_type = data.getDataType() if data is not None else None
        if table_type is not None and hasattr(table_type, "getDefinedComponents"):
            for component in table_type.getDefinedComponents():
                pointed = component.getDataType()
                if pointed is not None and hasattr(pointed, "getDataType"):
                    pointed = pointed.getDataType()
                prototype = None
                if pointed is not None and hasattr(pointed, "getPrototypeString"):
                    prototype = pointed.getPrototypeString()
                elif pointed is not None and hasattr(pointed, "getDisplayName"):
                    prototype = pointed.getDisplayName()
                slots.append(
                    {
                        "field": component.getFieldName(),
                        "offset": int(component.getOffset()),
                        "contract": prototype,
                    }
                )
        folded = symbol_name.casefold()
        if "ctor" in folded:
            role = "construction"
        elif "{for" in folded or "_for_" in folded:
            role = "base"
        else:
            role = "primary"
        tables.append(
            {
                "name": symbol_name,
                "address": str(symbol.getAddress()),
                "path": str(table_type.getPathName())
                if table_type is not None and hasattr(table_type, "getPathName")
                else None,
                "role": role,
                "size": int(table_type.getLength())
                if table_type is not None and hasattr(table_type, "getLength")
                else None,
                "slots": slots,
                "references": references,
            }
        )
    return {"schema": "wiz8.class-report", "classes": classes, "vtables": tables}


def format_decompile_text(payload: dict[str, Any]) -> str:
    blocks: list[str] = []
    freshness = payload.get("source_index") or {}
    if freshness:
        blocks.append(
            "Source index: "
            f"{freshness.get('state') or 'unavailable'}"
            + (f" ({freshness.get('detail')})" if freshness.get("detail") else "")
        )
    for row in payload.get("functions") or []:
        native = row.get("native") or {}
        source = row.get("source") or {}
        identities = source.get("identities") or []
        identity = identities[0] if identities else {}
        lines = [
            f"{row.get('entry') or row.get('selector')}  {native.get('qualified_name') or identity.get('name') or ''}".rstrip(),
            f"status: {row.get('status')}",
        ]
        if native.get("calling_convention") or native.get("prototype"):
            convention = native.get("calling_convention") or ""
            prototype = native.get("prototype") or ""
            lines.append(f"ProgramDB prototype: {prototype}  {convention}".rstrip())
        if identity:
            location = (
                f"{identity.get('source_file')}:{identity.get('line')}"
                if identity.get("source_file")
                else ""
            )
            lines.append(f"Source owner: {identity.get('name')}  {location}".rstrip())
            if identity.get("signature"):
                lines.append(f"Source declaration: {identity['signature']}")
            if identity.get("calling_convention"):
                lines.append(f"Source calling convention: {identity['calling_convention']}")
        elif source.get("state") and source.get("state") != "current":
            lines.append(
                f"Source metadata: {source.get('state')} ({source.get('detail') or 'unavailable'})"
            )
        inferred = None
        parameters = native.get("parameters") or []
        high_names = [item.get("high_name") for item in parameters if item.get("high_name")]
        if high_names:
            inferred = ", ".join(str(name) for name in high_names)
            lines.append(f"Decompilation inferred: {inferred}")
        for defect in row.get("defects") or []:
            lines.append(f"Warning: {defect.get('kind')}: {defect.get('detail')}")
        if native.get("error"):
            lines.append(f"Decompiler error: {native['error']}")
        if row.get("error") and row.get("status") == "missing-function":
            lines.append(f"No function: {row['error']}")
        code = native.get("decompiled")
        if isinstance(code, str) and code.strip():
            presented = candidate_text_with_defects(code, list(row.get("defects") or []))
            lines.append("")
            lines.append(presented or code)
        listing = row.get("listing") or []
        if listing and not code:
            lines.append("")
            lines.extend(listing[:32])
        artifacts = row.get("artifacts") or {}
        if artifacts:
            lines.append("")
            lines.append("Artifacts:")
            for key, value in artifacts.items():
                lines.append(f"  {key}: {value}")
        blocks.append("\n".join(lines).rstrip())
    if not blocks and payload.get("failures"):
        blocks = [str(row.get("error") or row) for row in payload["failures"]]
    return "\n\n".join(blocks) + ("\n" if blocks else "")


def format_asm_text(payload: dict[str, Any]) -> str:
    blocks: list[str] = []
    for row in payload.get("functions") or []:
        header = f"{row.get('entry')}  {row.get('name') or ''}".rstrip()
        lines = [header, f"status: {row.get('status')}"]
        if row.get("error"):
            lines.append(f"note: {row['error']}")
        lines.append("")
        lines.extend(row.get("listing") or [])
        artifacts = row.get("artifacts") or {}
        if artifacts:
            lines.append("")
            lines.append("Artifacts:")
            for key, value in artifacts.items():
                lines.append(f"  {key}: {value}")
        blocks.append("\n".join(lines).rstrip())
    for row in payload.get("failures") or []:
        if row.get("listing"):
            continue
        blocks.append(f"{row.get('selector')}: {row.get('error')}")
    return "\n\n".join(blocks) + ("\n" if blocks else "")


def format_sym_text(payload: dict[str, Any]) -> str:
    blocks: list[str] = []
    for row in payload.get("symbols") or []:
        path = row.get("access_path") or row.get("name") or row.get("address")
        lines = [
            f"{row.get('address')}  {path}",
            f"kind: {row.get('kind')}",
        ]
        if row.get("containing"):
            lines.append(f"containing: {row['containing']}")
        if row.get("field_type"):
            lines.append(f"field type: {row['field_type']}")
        if row.get("type"):
            lines.append(f"type: {row['type']}")
        if row.get("residual"):
            lines.append(f"residual: {row['residual']}")
        if row.get("union_members"):
            lines.append("union members:")
            lines.extend(f"  {member}" for member in row["union_members"])
        if row.get("import_target"):
            lines.append(f"import: {row['import_target']}")
        if row.get("detail"):
            lines.append(row["detail"])
        blocks.append("\n".join(lines))
    for row in payload.get("failures") or []:
        blocks.append(f"{row.get('selector')}: {row.get('error')}")
    return "\n\n".join(blocks) + ("\n" if blocks else "")


def format_class_text(payload: dict[str, Any]) -> str:
    blocks: list[str] = []
    for row in payload.get("classes") or []:
        lines = [f"{row.get('name')}  size={row.get('size')}  {row.get('path') or ''}"]
        for field in row.get("fields") or []:
            lines.append(
                f"  +0x{int(field['offset']):x}  {field.get('field')}  {field.get('type')}  len={field.get('length')}"
            )
        for gap in row.get("unknown") or []:
            lines.append(f"  +0x{int(gap['offset']):x}  <unknown>  len={gap.get('length')}")
        for base in row.get("bases") or []:
            lines.append(
                f"  base {base.get('base') or base.get('field')} at +0x{int(base['offset']):x}"
                + (f"  vtable={base['vtable']}" if base.get("vtable") else "")
            )
        for field in row.get("vfptrs") or []:
            lines.append(
                f"  vfptr {field.get('field')} at +0x{int(field['offset']):x}  {field.get('type')}"
            )
        for field in row.get("vbptrs") or []:
            lines.append(
                f"  vbptr {field.get('field')} at +0x{int(field['offset']):x}  {field.get('type')}"
            )
        blocks.append("\n".join(lines).rstrip())
    for table in payload.get("vtables") or []:
        lines = [
            f"{table.get('name')}  {table.get('address')}  size={table.get('size')}"
            + (f"  {table.get('role')}" if table.get("role") else "")
        ]
        for slot in table.get("slots") or []:
            lines.append(
                f"  +0x{int(slot['offset']):x}  {slot.get('field')}  {slot.get('contract') or ''}".rstrip()
            )
        blocks.append("\n".join(lines).rstrip())
    return "\n\n".join(blocks) + ("\n" if blocks else "")
