"""Project source ownership projected into reviewed Ghidra function tags."""

from __future__ import annotations

from typing import Any

from ..source_model import (
    SourceFunction,
    build_source_model,
    load_source_index,
    target_for_program,
)
from ..surrender_abi import parse_decorated_name

TAG_PREFIXES = ("wiz8:source:", "wiz8:role:", "wiz8:origin:")
VIRTUAL_SLOT_PROPERTY = "wiz8.virtual-slot.decorated"


def source_vtable_symbols(document: dict[str, Any], *, target: str) -> dict[int, tuple[str, str]]:
    """Return compiler-backed class vftable labels keyed by image address."""

    source_stem = "surrender" if target == "SURRENDER" else "wiz8"
    prefixes = (f"src/{source_stem}/", f"include/{source_stem}/")
    symbols: dict[int, tuple[str, str]] = {}
    for record in document["classes"]:
        owner = record.get("qualified_name")
        source_file = record.get("source_file")
        table = record.get("vtable_address")
        if (
            isinstance(owner, str)
            and isinstance(source_file, str)
            and source_file.startswith(prefixes)
            and isinstance(table, int)
        ):
            symbols[table] = (owner, "'vftable'")
        for base in record.get("base_vtables", ()):
            address = base.get("address")
            base_class = base.get("base_class")
            if (
                isinstance(owner, str)
                and isinstance(source_file, str)
                and source_file.startswith(prefixes)
                and isinstance(address, int)
                and isinstance(base_class, str)
            ):
                symbols[address] = (owner, f"'vftable'{{for_'{base_class}'}}")
    return symbols


def _apply_source_vtable_symbols(program: Any, document: dict[str, Any], *, target: str) -> int:
    """Materialize exact source-index vftable labels in their class namespaces."""

    from ghidra.program.model.symbol import SourceType, SymbolType, SymbolUtilities

    table = program.getSymbolTable()
    space = program.getAddressFactory().getDefaultAddressSpace()
    applied = 0
    for offset, (owner, name) in source_vtable_symbols(document, target=target).items():
        name = str(SymbolUtilities.replaceInvalidChars(name, True))
        leaf = owner.rsplit("::", 1)[-1]
        classes = [
            symbol.getObject()
            for symbol in table.getSymbols(leaf)
            if symbol.getSymbolType() == SymbolType.CLASS
            and symbol.getObject().getName(True) == owner
        ]
        if len(classes) != 1:
            continue
        namespace = classes[0]
        address = space.getAddress(offset)
        if any(
            symbol.getName() == name and symbol.getParentNamespace() == namespace
            for symbol in table.getSymbols(address)
        ):
            continue
        table.createLabel(address, name, namespace, SourceType.USER_DEFINED)
        applied += 1
    return applied


def source_virtual_slots(document: dict[str, Any], *, target: str) -> dict[int, str]:
    """Return compiler-owned decorated identities keyed by vftable slot address."""

    source_stem = "surrender" if target == "SURRENDER" else "wiz8"
    prefixes = (f"src/{source_stem}/", f"include/{source_stem}/")
    slots: dict[int, str] = {}
    for class_record in document["classes"]:
        source_file = class_record.get("source_file")
        table = class_record.get("vtable_address")
        if not isinstance(source_file, str) or not source_file.startswith(prefixes):
            continue
        if not isinstance(table, int):
            continue
        for index, decorated in enumerate(class_record.get("virtual_declarations", ())):
            if not isinstance(decorated, str) or not decorated.startswith("?"):
                continue
            address = table + index * 4
            previous = slots.setdefault(address, decorated)
            if previous != decorated:
                raise ValueError(f"vftable slot 0x{address:08x} has conflicting source identities")
    return slots


def _validated_virtual_slots(
    program: Any, document: dict[str, Any], *, target: str
) -> dict[int, str]:
    """Keep only source tables whose concrete slots align with program memory."""

    candidates = source_virtual_slots(document, target=target)
    space = program.getAddressFactory().getDefaultAddressSpace()
    memory = program.getMemory()
    manager = program.getFunctionManager()
    approved: dict[int, str] = {}
    for class_record in document["classes"]:
        table = class_record.get("vtable_address")
        declarations = class_record.get("virtual_declarations", ())
        if not isinstance(table, int) or table not in candidates:
            continue
        compared = 0
        aligned = True
        for index, decorated in enumerate(declarations):
            parsed = parse_decorated_name(decorated)
            if parsed.parse_status != "ok" or "destructor" in parsed.kind:
                continue
            slot = space.getAddress(table + index * 4)
            if not memory.contains(slot):
                aligned = False
                break
            target_address = space.getAddress(memory.getInt(slot) & 0xFFFFFFFF)
            function = manager.getFunctionAt(target_address)
            if function is None or function.getName().replace("_", "").casefold() == "purecall":
                continue
            compared += 1
            if function.getName() != parsed.member_name:
                aligned = False
                break
        if aligned and compared >= 2:
            for index, decorated in enumerate(declarations):
                address = table + index * 4
                if address in candidates:
                    approved[address] = decorated
    return approved


def source_role_tags(function: SourceFunction, *, target: str) -> tuple[str, ...]:
    """Return the source/emission/origin tags proved by one source marker."""

    origin = "surrender" if target == "SURRENDER" else "first-party"
    parsed = (
        parse_decorated_name(function.semantic_id)
        if function.semantic_id and function.semantic_id.startswith("?")
        else None
    )
    if function.kind == "LIBRARY":
        return (
            "wiz8:source:library-entity",
            "wiz8:role:library-body",
            "wiz8:origin:library",
        )
    if function.kind == "TEMPLATE":
        return (
            "wiz8:source:template-member",
            "wiz8:role:template-emission",
            f"wiz8:origin:{origin}",
        )
    if function.kind == "SYNTHETIC":
        normalized = function.name.replace("_", " ").casefold()
        if "scalar deleting destructor" in normalized:
            role = "scalar-deleting-destructor"
        elif "vector deleting destructor" in normalized:
            role = "vector-deleting-destructor"
        else:
            # A synthetic marker proves compiler ownership, but not one of the
            # semantic roles the Java resolver understands. Do not guess.
            return ()
        return (
            "wiz8:source:destructor",
            f"wiz8:role:{role}",
            f"wiz8:origin:{origin}",
        )

    if parsed is not None and parsed.parse_status == "ok":
        special = {
            "vbase-destructor": ("destructor", "vbase-destructor", origin),
            "scalar-deleting-destructor": (
                "destructor",
                "scalar-deleting-destructor",
                origin,
            ),
            "vector-deleting-destructor": (
                "destructor",
                "vector-deleting-destructor",
                origin,
            ),
            "default-constructor-closure": (
                "constructor",
                "default-constructor-closure",
                origin,
            ),
            "vector-constructor-iterator": (
                "library-entity",
                "vector-constructor-iterator",
                "msvc-crt",
            ),
            "vector-destructor-iterator": (
                "library-entity",
                "vector-destructor-iterator",
                "msvc-crt",
            ),
            "local-static-guard": ("none", "local-static-guard", origin),
        }.get(parsed.kind)
        if special is not None:
            source, role, special_origin = special
            return (
                f"wiz8:source:{source}",
                f"wiz8:role:{role}",
                f"wiz8:origin:{special_origin}",
            )
        if parsed.adjustor_thunk:
            source = (
                "destructor"
                if "destructor" in parsed.kind
                else "constructor"
                if parsed.kind == "constructor"
                else "member-function"
            )
            return (
                f"wiz8:source:{source}",
                "wiz8:role:adjustor-thunk",
                f"wiz8:origin:{origin}",
            )

    semantic = function.semantic_kind
    if (
        parsed is not None
        and parsed.parse_status == "ok"
        and parsed.kind in {"constructor", "destructor"}
    ):
        semantic = parsed.kind
    if semantic == "constructor":
        source, role = "constructor", "authored-body"
    elif semantic == "destructor":
        # A source marker proves which emission owns the authored source body,
        # but it does not prove whether Microsoft named that binary emission
        # base, complete, or vbase destructor/constructor machinery.
        source, role = "destructor", "authored-body"
    elif function.owning_class:
        source, role = "member-function", "authored-body"
    else:
        source, role = "free-function", "authored-body"
    return (
        f"wiz8:source:{source}",
        f"wiz8:role:{role}",
        f"wiz8:origin:{origin}",
    )


def apply_source_role_tags(settings: Any, program_name: str) -> dict[str, int]:
    """Apply compiler-backed marker roles transactionally to one live program."""

    target = target_for_program(program_name)
    functions = build_source_model(settings.repo_dir, target).functions
    source_index = load_source_index(settings.repo_dir)

    from .environment import start_pyghidra

    start_pyghidra(settings)
    import pyghidra
    from ghidra.util.task import TaskMonitor

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    applied = 0
    missing = 0
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            space = program.getAddressFactory().getDefaultAddressSpace()
            manager = program.getFunctionManager()
            virtual_slots = _validated_virtual_slots(program, source_index, target=target)
            transaction = program.startTransaction("apply source-owned function roles")
            commit = False
            try:
                properties = program.getUsrPropertyManager()
                properties.removePropertyMap(VIRTUAL_SLOT_PROPERTY)
                slot_properties = properties.createStringPropertyMap(VIRTUAL_SLOT_PROPERTY)
                for address, decorated in virtual_slots.items():
                    slot_properties.add(space.getAddress(address), decorated)
                vtable_symbols = _apply_source_vtable_symbols(program, source_index, target=target)
                for entry, source in functions.items():
                    function = manager.getFunctionAt(space.getAddress(entry))
                    if function is None:
                        missing += 1
                        continue
                    for tag in tuple(function.getTags()):
                        if str(tag.getName()).startswith(TAG_PREFIXES):
                            function.removeTag(str(tag.getName()))
                    for tag in source_role_tags(source, target=target):
                        function.addTag(tag)
                    applied += 1
                commit = True
            finally:
                program.endTransaction(transaction, commit)
            program.save("Apply compiler-backed source function roles", TaskMonitor.DUMMY)
    finally:
        project.close()
    return {
        "applied": applied,
        "missing": missing,
        "virtual_slots": len(virtual_slots),
        "vtable_symbols": vtable_symbols,
    }
