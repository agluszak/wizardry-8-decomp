"""One address/entity resolver for native Ghidra state and source identity."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Literal

from ..source_index import (
    AddressBoundIdentity,
    address_bound_identities,
    source_index_freshness,
    target_for_program,
    try_load_source_index,
)


class ResolveError(ValueError):
    """A selector did not identify exactly one entity."""


@dataclass(frozen=True)
class ResolvedFunction:
    entry: int
    function: Any
    selector: str


@dataclass(frozen=True)
class ResolvedSymbol:
    address: int
    kind: str
    name: str
    type_name: str | None
    defined_at: int | None
    length: int | None
    field: str | None
    residual: int
    aliases: tuple[str, ...]
    import_target: str | None
    extent_status: str | None
    detail: str | None
    views: tuple[str, ...] = ()


def hex_address(value: int | Any) -> str:
    offset = int(value.getOffset()) if hasattr(value, "getOffset") else int(value)
    return f"0x{offset:08x}"


def program_address(program: Any, text: str) -> Any:
    value = program.getAddressFactory().getAddress(text)
    if value is None:
        value = (
            program.getAddressFactory().getDefaultAddressSpace().getAddress(text.removeprefix("0x"))
        )
    if value is None:
        raise ResolveError(f"invalid address: {text}")
    return value


def available_program_selectors(settings: Any) -> list[str]:
    from .workspace import seed_records

    names: list[str] = []
    for record in seed_records(settings):
        program = str(record["program"])
        if "--gog-base--wiz8--" in program and "wiz8" not in names:
            names.append("wiz8")
        elif "--gog-base--sr--" in program and "sr" not in names:
            names.append("sr")
        names.append(program)
    return names


def resolve_program_selector(settings: Any, selector: str) -> str:
    from .workspace import resolve_seed_program

    try:
        return resolve_seed_program(settings, selector)
    except ValueError as error:
        choices = ", ".join(available_program_selectors(settings))
        raise ResolveError(
            f"Ghidra selector {selector!r} did not identify one tracked program; "
            f"available: {choices}"
        ) from error


def source_metadata(settings: Any, program_selector: str) -> dict[str, Any]:
    try:
        target = target_for_program(settings.repo_dir, program_selector)
    except Exception:  # noqa: BLE001 - keep native reads usable without a target
        target = "WIZ8"
    freshness = source_index_freshness(settings.repo_dir, target)
    document = try_load_source_index(settings.repo_dir)
    freshness["available"] = document is not None
    freshness["target"] = target
    return freshness


def identities_for_target(
    settings: Any, program_selector: str
) -> dict[int, tuple[AddressBoundIdentity, ...]]:
    metadata = source_metadata(settings, program_selector)
    if not metadata.get("available"):
        return {}
    try:
        return address_bound_identities(settings.repo_dir, str(metadata["target"]))
    except Exception:  # noqa: BLE001 - source attachment is optional on the read path
        return {}


def identities_at(
    settings: Any, program_selector: str, address: int
) -> tuple[AddressBoundIdentity, ...]:
    return identities_for_target(settings, program_selector).get(address, ())


def _functions_named(program: Any, text: str) -> list[Any]:
    matches = []
    iterator = program.getFunctionManager().getFunctions(True)
    while iterator.hasNext():
        candidate = iterator.next()
        if text in {candidate.getName(), candidate.getName(True)}:
            matches.append(candidate)
    return matches


def resolve_function(program: Any, selector: str) -> Any:
    """Resolve one function from an address, range singleton, or exact name."""

    manager = program.getFunctionManager()
    try:
        address = program_address(program, selector)
    except Exception:  # noqa: BLE001 - Ghidra throws AddressFormatException
        matches = _functions_named(program, selector)
        if not matches:
            raise ResolveError(f"unknown function selector: {selector}") from None
        if len(matches) > 1:
            candidates = ", ".join(
                f"{item.getName(True)} at {hex_address(item.getEntryPoint())}"
                for item in matches[:8]
            )
            raise ResolveError(
                f"ambiguous function selector {selector!r}; candidates: {candidates}"
            ) from None
        return matches[0]
    function = manager.getFunctionAt(address) or manager.getFunctionContaining(address)
    if function is None:
        raise ResolveError(f"no function contains {hex_address(address)}")
    return function


def resolve_function_entries(program: Any, values: list[str]) -> list[int]:
    """Resolve investigative names and ranges against the open Program."""

    selected: set[int] = set()
    manager = program.getFunctionManager()
    for value in values:
        start_text, separator, end_text = value.strip().partition(":")
        try:
            start = int(start_text, 0)
            end = int(end_text, 0) if separator else start
        except ValueError:
            selected.add(int(resolve_function(program, value).getEntryPoint().getOffset()))
            continue
        if start < 0 or end < start:
            raise ResolveError(f"invalid function selector range: {value}")
        if start == end:
            selected.add(int(resolve_function(program, value).getEntryPoint().getOffset()))
            continue
        matches = [
            int(function.getEntryPoint().getOffset())
            for function in manager.getFunctions(True)
            if start <= int(function.getEntryPoint().getOffset()) <= end
        ]
        if not matches:
            raise ResolveError(f"no Ghidra functions in selector range {value}")
        selected.update(matches)
    if not selected:
        raise ResolveError("pass one or more function selectors")
    return sorted(selected)


def _component_at(data_type: Any, offset: int) -> tuple[Any, int] | None:
    if data_type is None or not hasattr(data_type, "getLength"):
        return None
    length = int(data_type.getLength())
    if length > 0 and not (0 <= offset < length):
        return None
    if hasattr(data_type, "getDefinedComponents"):
        for component in data_type.getDefinedComponents():
            start = int(component.getOffset())
            size = int(component.getLength())
            if start <= offset < start + size:
                nested = _component_at(component.getDataType(), offset - start)
                if nested is not None:
                    child, residual = nested
                    name = component.getFieldName() or ""
                    child_name = child.getFieldName() if hasattr(child, "getFieldName") else None
                    if name and child_name:
                        try:
                            child.setFieldName  # noqa: B018 - existence probe
                        except Exception:  # noqa: BLE001,S110
                            pass
                    return child, residual
                return component, offset - start
        return None
    if hasattr(data_type, "getNumElements") and hasattr(data_type, "getElementLength"):
        element_length = int(data_type.getElementLength())
        if element_length <= 0:
            return None
        index = offset // element_length
        residual = offset % element_length
        if 0 <= index < int(data_type.getNumElements()):
            nested = _component_at(data_type.getDataType(), residual)
            return (nested[0], nested[1]) if nested is not None else (data_type, residual)
    return None


def _import_target(program: Any, address: Any) -> str | None:
    data = program.getListing().getDataAt(address)
    if data is None:
        function = program.getFunctionManager().getFunctionAt(address)
        if function is not None and function.isThunk():
            thunked = function.getThunkedFunction(True)
            if thunked is not None:
                return thunked.getName(True)
        return None
    value = data.getValue()
    if hasattr(value, "getOffset"):
        target = program.getFunctionManager().getFunctionAt(
            value
        ) or program.getSymbolTable().getPrimarySymbol(value)
        if target is None:
            return hex_address(value)
        if hasattr(target, "getName"):
            return target.getName(True) if hasattr(target, "getName") else str(target)
    if data.hasStringValue():
        return None
    return str(value) if value is not None else None


def _listing_views(program: Any, address: Any) -> tuple[str, ...]:
    """Every established function/instruction/data view covering this address."""

    offset = int(address.getOffset())
    listing = program.getListing()
    views: list[str] = []
    function = program.getFunctionManager().getFunctionContaining(address)
    if function is not None:
        views.append(f"function {function.getName(True)} @ {hex_address(function.getEntryPoint())}")
    instruction = listing.getInstructionAt(address)
    if instruction is None and hasattr(listing, "getInstructionContaining"):
        instruction = listing.getInstructionContaining(address)
    if instruction is not None:
        views.append(f"instruction {instruction}")
    seen_data: set[tuple[int, int]] = set()
    for getter in ("getDataAt", "getDataContaining"):
        data = getattr(listing, getter)(address) if hasattr(listing, getter) else None
        if data is None:
            continue
        start = int(data.getAddress().getOffset())
        length = int(data.getLength()) if hasattr(data, "getLength") else 0
        key = (start, length)
        if key in seen_data:
            continue
        seen_data.add(key)
        data_type = data.getDataType() if hasattr(data, "getDataType") else None
        type_name = data_type.getDisplayName() if data_type is not None else "data"
        views.append(f"{type_name} @ {hex_address(start)} len={length}")
    previous = (
        listing.getDefinedDataBefore(address) if hasattr(listing, "getDefinedDataBefore") else None
    )
    if previous is not None:
        start = int(previous.getAddress().getOffset())
        length = int(previous.getLength()) if hasattr(previous, "getLength") else 0
        if length and start <= offset < start + length and (start, length) not in seen_data:
            data_type = previous.getDataType() if hasattr(previous, "getDataType") else None
            type_name = data_type.getDisplayName() if data_type is not None else "data"
            views.append(f"{type_name} @ {hex_address(start)} len={length}")
    symbols = program.getSymbolTable().getSymbols(address)
    for symbol in symbols or ():
        name = symbol.getName(True) if hasattr(symbol, "getName") else ""
        if name:
            views.append(f"symbol {name}")
    return tuple(dict.fromkeys(views))


def resolve_symbol(program: Any, selector: str) -> ResolvedSymbol:
    """Resolve one address to its native symbol, field, import, or data view."""

    address = program_address(program, selector)
    offset = int(address.getOffset())
    listing = program.getListing()
    symbols = program.getSymbolTable()
    function = program.getFunctionManager().getFunctionAt(address)
    primary = symbols.getPrimarySymbol(address)
    aliases = tuple(
        sorted(
            {
                str(symbol.getName(True))
                for symbol in symbols.getSymbols(address)
                if symbol.getName(True)
            }
        )
    )
    data = listing.getDataContaining(address)
    views = _listing_views(program, address)
    if function is not None and (data is None or function.getEntryPoint().equals(address)):
        return ResolvedSymbol(
            address=offset,
            kind="function" if not function.isThunk() else "thunk",
            name=function.getName(True),
            type_name=function.getPrototypeString(False, False),
            defined_at=int(function.getEntryPoint().getOffset()),
            length=int(function.getBody().getNumAddresses()),
            field=None,
            residual=0,
            aliases=aliases,
            import_target=_import_target(program, address),
            extent_status=None,
            detail=None,
            views=views,
        )
    if data is not None:
        defined_at = int(data.getAddress().getOffset())
        residual = offset - defined_at
        data_type = data.getDataType()
        field_name = None
        extent_status = "within"
        detail = None
        component = _component_at(data_type, residual)
        if residual != 0 and component is None and hasattr(data_type, "getLength"):
            length = int(data_type.getLength())
            if length > 0 and residual >= length:
                extent_status = "beyond-known-extent"
                detail = (
                    f"offset {residual} is outside {data_type.getDisplayName()} length {length}"
                )
            elif residual != 0:
                extent_status = "unknown-field"
                detail = f"no field at residual offset {residual}"
        if component is not None:
            field, inner_residual = component
            field_name = field.getFieldName() if hasattr(field, "getFieldName") else None
            if hasattr(field, "getNumElements") and field_name is None:
                element_length = (
                    int(field.getElementLength()) if hasattr(field, "getElementLength") else 0
                )
                if element_length:
                    index = residual // element_length
                    field_name = f"[{index}]"
                    inner_residual = residual % element_length
            residual = inner_residual
        import_target = None
        kind = "data"
        name = primary.getName(True) if primary is not None else ""
        if name.startswith("PTR_") or (
            data_type is not None and "Pointer" in type(data_type).__name__
        ):
            kind = "import-cell" if _import_target(program, data.getAddress()) else "pointer"
            import_target = _import_target(program, data.getAddress())
        if data.hasStringValue():
            kind = "string"
        return ResolvedSymbol(
            address=offset,
            kind=kind,
            name=name,
            type_name=data_type.getDisplayName() if data_type is not None else None,
            defined_at=defined_at,
            length=int(data.getLength()),
            field=field_name,
            residual=residual,
            aliases=aliases,
            import_target=import_target,
            extent_status=extent_status,
            detail=detail,
            views=views,
        )
    return ResolvedSymbol(
        address=offset,
        kind="untyped",
        name=primary.getName(True) if primary is not None else "",
        type_name=None,
        defined_at=None,
        length=None,
        field=None,
        residual=0,
        aliases=aliases,
        import_target=_import_target(program, address),
        extent_status="unknown",
        detail="no defined data or function at this address",
        views=views,
    )


def symbol_record(symbol: ResolvedSymbol) -> dict[str, Any]:
    return {
        "address": hex_address(symbol.address),
        "kind": symbol.kind,
        "name": symbol.name,
        "type": symbol.type_name,
        "defined_at": hex_address(symbol.defined_at) if symbol.defined_at is not None else None,
        "length": symbol.length,
        "field": symbol.field,
        "residual": symbol.residual,
        "aliases": list(symbol.aliases),
        "import_target": symbol.import_target,
        "extent": symbol.extent_status,
        "detail": symbol.detail,
        "views": list(symbol.views),
    }


SourceState = Literal["current", "stale", "missing", "invalid", "unavailable"]
