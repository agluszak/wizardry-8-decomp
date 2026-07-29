from __future__ import annotations

from typing import Any

from ..config import Settings
from ..paths import atomic_json
from .environment import start_pyghidra
from .workspace import ensure_seed


def _address_ranges(body: Any) -> list[dict[str, str]]:
    iterator = body.getAddressRanges()
    ranges = []
    while iterator.hasNext():
        item = iterator.next()
        ranges.append({"start": str(item.getMinAddress()), "end": str(item.getMaxAddress())})
    return ranges


def function_record(function: Any) -> dict[str, Any]:
    body = function.getBody()
    return {
        "entry": str(function.getEntryPoint()),
        "name": function.getName(),
        "qualified_name": function.getName(True),
        "namespace": str(function.getParentNamespace()),
        "signature": function.getPrototypeString(False, False),
        "calling_convention": function.getCallingConventionName(),
        "size": body.getNumAddresses(),
        "ranges": _address_ranges(body),
        "thunk_target": (
            str(function.getThunkedFunction(False).getEntryPoint())
            if function.isThunk() and function.getThunkedFunction(False)
            else None
        ),
    }


def _component_record(component: Any) -> dict[str, Any]:
    return {
        "offset": component.getOffset(),
        "length": component.getLength(),
        "field": component.getFieldName(),
        "type": component.getDataType().getDisplayName(),
        "comment": component.getComment(),
    }


def type_record(data_type: Any) -> dict[str, Any] | None:
    from ghidra.program.model.data import Enum, Structure, Union

    if isinstance(data_type, Structure):
        kind = "structure"
    elif isinstance(data_type, Union):
        kind = "union"
    elif isinstance(data_type, Enum):
        kind = "enum"
    else:
        return None
    record: dict[str, Any] = {
        "path": str(data_type.getDataTypePath()),
        "name": data_type.getName(),
        "kind": kind,
        "length": data_type.getLength(),
    }
    if kind in {"structure", "union"}:
        record["components"] = [
            _component_record(component) for component in data_type.getComponents()
        ]
    else:
        record["values"] = [
            {"name": name, "value": data_type.getValue(name)}
            for name in sorted(data_type.getNames())
        ]
    return record


def _functions(program: Any) -> list[dict[str, Any]]:
    iterator = program.getFunctionManager().getFunctions(True)
    values = []
    while iterator.hasNext():
        values.append(function_record(iterator.next()))
    return values


def _types(program: Any) -> list[dict[str, Any]]:
    iterator = program.getDataTypeManager().getAllDataTypes()
    values = []
    while iterator.hasNext():
        record = type_record(iterator.next())
        if record is not None:
            values.append(record)
    return sorted(values, key=lambda item: item["path"].casefold())


def _vtables(program: Any) -> list[dict[str, Any]]:
    listing = program.getListing()
    references = program.getReferenceManager()
    symbols = program.getSymbolTable().getAllSymbols(True)
    values: dict[str, dict[str, Any]] = {}
    while symbols.hasNext():
        symbol = symbols.next()
        name = symbol.getName(True)
        folded = name.casefold()
        address = symbol.getAddress()
        data = listing.getDataAt(address)
        data_type = data.getDataType().getDisplayName() if data is not None else ""
        if (
            "vftable" not in folded
            and "vtable" not in folded
            and "vtable" not in data_type.casefold()
        ):
            continue
        values[str(address)] = {
            "address": str(address),
            "name": name,
            "type": data_type or None,
            "length": data.getLength() if data is not None else None,
            "references": sorted(
                str(ref.getFromAddress()) for ref in references.getReferencesTo(address)
            ),
        }
    return [values[address] for address in sorted(values)]


def export_index(settings: Settings, selector: str = "wiz8") -> dict[str, Any]:
    from ..evidence.claims import validate_claims_against_documents
    from ..source_model import validate_source_names_against_index

    program_name = ensure_seed(settings, selector)
    start_pyghidra(settings)
    import pyghidra

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            documents = {
                "functions": {
                    "schema": "wiz8.ghidra-index.functions",
                    "program": program_name,
                    "functions": _functions(program),
                },
                "types": {
                    "schema": "wiz8.ghidra-index.types",
                    "program": program_name,
                    "types": _types(program),
                },
                "vtables": {
                    "schema": "wiz8.ghidra-index.vtables",
                    "program": program_name,
                    "vtables": _vtables(program),
                },
            }
    finally:
        project.close()

    output = settings.build_dir / "ghidra-index"
    paths = []
    counts = {}
    for name, document in documents.items():
        path = output / f"{name}.json"
        atomic_json(path, document)
        paths.append(str(path.relative_to(settings.repo_dir)))
        counts[name] = len(document[name])
    claim_counts = validate_claims_against_documents(settings.repo_dir, documents)
    source_count = validate_source_names_against_index(settings.repo_dir, documents["functions"])
    return {
        "schema": "wiz8.ghidra-index",
        "program": program_name,
        "counts": counts,
        "claims": claim_counts,
        "source_functions": source_count,
        "outputs": paths,
    }
