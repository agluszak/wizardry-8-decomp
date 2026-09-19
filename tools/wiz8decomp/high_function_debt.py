"""Census HighFunction facts that C-text pain metrics cannot see.

This is a read-only Ghidra report: CAST/CALLIND ops, untyped ``this``, default
conventions, suspicious PTRADD/PTRSUB, unaff/in_/extraout locals, and untyped
indirect calls. Typed PTRSUB to a known component and PTRADD whose element size
matches the pointer type are not debt. It does not mutate the program and is
not part of the decompiler-quality debt gate. Rows are ranked by debt × caller
fanout.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Iterable
from typing import Any

from .config import Settings
from .decompiler_quality import select_corpus
from .paths import atomic_json
from .source_index import source_functions

_SCHEMA = "wiz8.high-function-debt-v2"
METRIC_KEYS: tuple[str, ...] = (
    "undefined_this",
    "undefined_params",
    "untyped_return",
    "default_convention",
    "cast_ops",
    "callind_ops",
    "untyped_callind",
    "suspicious_ptr_ops",
    "unaff_vars",
)
_HIGH_DEBT = frozenset({"undefined_this", "undefined_params", "untyped_return", "untyped_callind"})


def _is_undefined_type(data_type: Any) -> bool:
    if data_type is None:
        return True
    name = (
        str(data_type.getDisplayName()) if hasattr(data_type, "getDisplayName") else str(data_type)
    )
    lowered = name.casefold()
    return lowered.startswith("undefined") or lowered in {"void *", "void*"}


def _unwrap_typedef(data_type: Any) -> Any:
    current = data_type
    while current is not None and "TypeDef" in type(current).__name__:
        if not hasattr(current, "getBaseDataType"):
            break
        current = current.getBaseDataType()
    return current


def _is_function_pointer_type(data_type: Any) -> bool:
    current = _unwrap_typedef(data_type)
    if current is None:
        return False
    if "Pointer" in type(current).__name__ and hasattr(current, "getDataType"):
        current = _unwrap_typedef(current.getDataType())
    if current is None:
        return False
    return "FunctionDefinition" in type(current).__name__ or (
        hasattr(current, "getArguments") and hasattr(current, "getReturnType")
    )


def _callind_input_type(op: Any) -> Any | None:
    if not hasattr(op, "getInput"):
        return None
    node = op.getInput(0)
    if node is None:
        return None
    high = node.getHigh() if hasattr(node, "getHigh") else None
    if high is not None and hasattr(high, "getDataType"):
        return high.getDataType()
    if hasattr(node, "getDataType"):
        return node.getDataType()
    return None


def _constant_int(node: Any) -> int | None:
    if node is None:
        return None
    if hasattr(node, "isConstant") and node.isConstant() and hasattr(node, "getOffset"):
        return int(node.getOffset())
    high = node.getHigh() if hasattr(node, "getHigh") else None
    if high is not None and hasattr(high, "isConstant") and high.isConstant():
        if hasattr(high, "getOffset"):
            return int(high.getOffset())
        if hasattr(node, "getOffset"):
            return int(node.getOffset())
    return None


def _structure_has_offset(data_type: Any, offset: int) -> bool:
    current = _unwrap_typedef(data_type)
    if current is None:
        return False
    if "Pointer" in type(current).__name__ and hasattr(current, "getDataType"):
        current = _unwrap_typedef(current.getDataType())
    if current is None or not hasattr(current, "getDefinedComponents"):
        return False
    return any(int(component.getOffset()) == offset for component in current.getDefinedComponents())


def _pointer_element_size(data_type: Any) -> int | None:
    current = _unwrap_typedef(data_type)
    if current is None:
        return None
    if "Pointer" in type(current).__name__ and hasattr(current, "getDataType"):
        pointed = _unwrap_typedef(current.getDataType())
        if pointed is None or _is_undefined_type(pointed):
            return None
        if hasattr(pointed, "getLength"):
            length = int(pointed.getLength())
            return length if length > 0 else None
    return None


def _ptr_op_is_suspicious(op: Any) -> bool:
    """True for void*/undefined bases or offsets that miss a known component."""

    mnemonic = op.getMnemonic()
    base_type = _callind_input_type(op)
    if mnemonic == "PTRSUB":
        if _is_undefined_type(base_type):
            return True
        offset = _constant_int(op.getInput(1) if hasattr(op, "getInput") else None)
        if offset is None:
            return True
        return not _structure_has_offset(base_type, offset)
    if mnemonic == "PTRADD":
        if _is_undefined_type(base_type):
            return True
        element = _pointer_element_size(base_type)
        if element is None:
            return True
        if hasattr(op, "getNumInputs") and int(op.getNumInputs()) > 2:
            scale = _constant_int(op.getInput(2))
            if scale is not None and scale != element:
                return True
        return False
    return False


def score_high_function(high: Any) -> dict[str, int]:
    """Count HighFunction residuals on one already-decompiled function."""

    counts = {key: 0 for key in METRIC_KEYS}
    prototype = high.getFunctionPrototype() if high is not None else None
    if high is None or prototype is None:
        return counts
    convention = str(prototype.getModelName() or "unknown").casefold()
    if convention in {"default", "unknown", ""}:
        counts["default_convention"] = 1
    return_type = prototype.getReturnType()
    if _is_undefined_type(return_type):
        counts["untyped_return"] = 1
    param_count = int(prototype.getNumParams())
    for index in range(param_count):
        symbol = prototype.getParam(index)
        data_type = symbol.getDataType() if symbol is not None else None
        if _is_undefined_type(data_type):
            counts["undefined_params"] += 1
        name = str(symbol.getName()) if symbol is not None else ""
        if index == 0 and name == "this" and _is_undefined_type(data_type):
            counts["undefined_this"] = 1
    if hasattr(high, "getLocalSymbolMap"):
        symbols = high.getLocalSymbolMap().getSymbols()
        while symbols.hasNext():
            symbol = symbols.next()
            name = str(symbol.getName()) if symbol is not None else ""
            if name.startswith(("unaff_", "in_", "extraout")):
                counts["unaff_vars"] += 1
    iterator = high.getPcodeOps()
    while iterator.hasNext():
        op = iterator.next()
        mnemonic = op.getMnemonic()
        if mnemonic == "CAST":
            counts["cast_ops"] += 1
        elif mnemonic == "CALLIND":
            counts["callind_ops"] += 1
            if not _is_function_pointer_type(_callind_input_type(op)):
                counts["untyped_callind"] += 1
        elif mnemonic in {"PTRADD", "PTRSUB"} and _ptr_op_is_suspicious(op):
            counts["suspicious_ptr_ops"] += 1
    return counts


def debt_total(counts: dict[str, int]) -> int:
    total = 0
    for key in METRIC_KEYS:
        weight = 4 if key in _HIGH_DEBT else 1
        total += int(counts.get(key, 0)) * weight
    return total


def _caller_count(function: Any) -> int:
    try:
        from ghidra.util.task import TaskMonitor  # type: ignore[import-not-found]
    except ImportError:
        return 0
    callers = function.getCallingFunctions(TaskMonitor.DUMMY)
    if callers is None:
        return 0
    return len(list(callers))


def run_high_function_debt(
    settings: Settings,
    *,
    target: str = "WIZ8",
    program_name: str = "wiz8",
    limit: int = 200,
    seed: int = 1,
    addresses: Iterable[int] | None = None,
    corpus_kind: str = "pain",
    profile: str = "analysis",
) -> dict[str, Any]:
    """Decompile a corpus and census HighFunction residuals."""

    from .decompiler_quality import _resolve_function
    from .ghidra.env import open_program
    from .ghidra.inspect import DecompileSession
    from .ghidra.semantic import _high_function

    address_list = list(addresses) if addresses is not None else None
    recovered = {
        address
        for address, marker in source_functions(settings.repo_dir, target).items()
        if marker.marker_kind == "FUNCTION"
    }
    corpus = select_corpus(
        settings.repo_dir,
        target=target,
        limit=limit,
        seed=seed,
        addresses=address_list,
        require_match=False if address_list is None else None,
        corpus_kind=corpus_kind,
    )
    functions: list[dict[str, Any]] = []
    totals: Counter[str] = Counter()
    failures = 0
    with open_program(settings, program_name) as program:
        session = DecompileSession(program, profile=profile)
        try:
            for address in corpus["addresses"]:
                function = _resolve_function(program, address)
                row: dict[str, Any] = {
                    "address": f"0x{address:08x}",
                    "recovered": address in recovered,
                }
                if function is None:
                    row["status"] = "missing-function"
                    row["metrics"] = {key: 0 for key in METRIC_KEYS}
                    row["callers"] = 0
                    failures += 1
                    functions.append(row)
                    continue
                row["name"] = function.getName(True)
                row["callers"] = _caller_count(function)
                try:
                    high = _high_function(program, function, profile=profile, session=session)
                    metrics = score_high_function(high)
                    row["status"] = "ok"
                    row["metrics"] = metrics
                    row["debt"] = debt_total(metrics)
                    row["priority"] = int(row["debt"]) * int(row["callers"])
                    totals.update(metrics)
                except Exception as exc:  # noqa: BLE001 — census must survive one-function failure
                    row["status"] = "decompiler-failure"
                    row["error"] = str(exc)
                    row["metrics"] = {key: 0 for key in METRIC_KEYS}
                    row["debt"] = 0
                    row["priority"] = 0
                    failures += 1
                functions.append(row)
        finally:
            session.close()

    functions.sort(key=lambda row: (-int(row.get("priority") or 0), str(row.get("address") or "")))
    destination = settings.build_dir / "high-function-debt"
    report = {
        "schema": _SCHEMA,
        "non_gating": True,
        "target": target,
        "program": program_name,
        "profile": profile,
        "corpus": {
            "size": len(corpus["addresses"]),
            "seed": corpus.get("seed"),
            "limit": corpus.get("limit"),
            "corpus_kind": corpus.get("corpus_kind"),
        },
        "summary": {
            "ok": sum(1 for row in functions if row.get("status") == "ok"),
            "failures": failures,
            "requested": len(functions),
            "totals": dict(totals),
            "debt_total": debt_total(dict(totals)),
        },
        "functions": functions,
    }
    atomic_json(destination / "report.json", report)
    return report
