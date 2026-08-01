"""Project source ownership projected into reviewed Ghidra function tags."""

from __future__ import annotations

from typing import Any

from ..source_model import SourceFunction, build_source_model, target_for_program
from ..surrender_abi import parse_decorated_name

TAG_PREFIXES = ("wiz8:source:", "wiz8:role:", "wiz8:origin:")


def source_role_tags(function: SourceFunction, *, target: str) -> tuple[str, ...]:
    """Return the source/emission/origin tags proved by one source marker."""

    origin = "surrender" if target == "SURRENDER" else "first-party"
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

    semantic = function.semantic_kind
    if function.semantic_id and function.semantic_id.startswith("?"):
        parsed = parse_decorated_name(function.semantic_id)
        if parsed.parse_status == "ok" and parsed.kind in {"constructor", "destructor"}:
            semantic = parsed.kind
    if semantic == "constructor":
        source, role = "constructor", "constructor-body"
    elif semantic == "destructor":
        source, role = "destructor", "complete-destructor"
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
            transaction = program.startTransaction("apply source-owned function roles")
            commit = False
            try:
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
    return {"applied": applied, "missing": missing}
