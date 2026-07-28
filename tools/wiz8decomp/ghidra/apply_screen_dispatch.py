"""Replay the observed finite screen-dispatch targets into ProgramDB.

The tracked 62-slot table is an observation, not a speculative inference
seed. Computed calls to its 44 real handler bodies therefore belong in every
reviewed materialization; folded trivial handlers remain represented by the
table evidence rather than as invented semantic identities.
"""

from __future__ import annotations

from typing import Any

from ..config import Settings
from ..indirect import resolve_handler_table
from .project import resolve_program_name

SCREEN_DISPATCH_FUNCTION = 0x004E3340
SCREEN_HANDLER_FIELD = 0x00647BD4


def _reaches(node: Any, address: int, visited: set[Any] | None = None) -> bool:
    if node is None:
        return False
    visited = visited or set()
    marker = (str(node.getAddress()), str(node.getDef().getSeqnum()) if node.getDef() else None)
    if marker in visited:
        return False
    visited.add(marker)
    space = node.getAddress().getAddressSpace().getName()
    if (node.isConstant() or space == "ram") and int(node.getOffset()) == address:
        return True
    definition = node.getDef()
    return bool(
        definition is not None
        and any(
            _reaches(definition.getInput(index), address, visited)
            for index in range(definition.getNumInputs())
        )
    )


def apply_screen_dispatch(
    settings: Settings,
    selector: str,
    *,
    materialize: bool = True,
) -> dict[str, Any]:
    """Add the deterministic handler table's computed-call references."""

    from .cache import open_for_mutation

    settings = open_for_mutation(settings, selector, materialize=materialize)
    import pyghidra
    from ghidra.program.model.symbol import RefType, SourceType

    from .semantic import _high_function, dispose_sessions

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    report: dict[str, Any] = {"program": program_name, "sites": [], "references_added": 0}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            space = program.getAddressFactory().getDefaultAddressSpace()
            function = program.getFunctionManager().getFunctionAt(
                space.getAddress(f"{SCREEN_DISPATCH_FUNCTION:08x}")
            )
            if function is None:
                return report
            high = _high_function(program, function, "normalize")
            sites = []
            iterator = high.getPcodeOps()
            while iterator.hasNext():
                op = iterator.next()
                if op.getMnemonic() == "CALLIND" and _reaches(op.getInput(0), SCREEN_HANDLER_FIELD):
                    sites.append(op.getSeqnum().getTarget())
            targets = sorted(resolve_handler_table(settings.repo_dir)["handler_targets"])
            references = program.getReferenceManager()
            transaction = program.startTransaction("replay observed screen dispatch")
            try:
                for site in sites:
                    existing = {
                        str(reference.getToAddress()).lower()
                        for reference in references.getReferencesFrom(site)
                    }
                    for target in targets:
                        destination = space.getAddress(target)
                        if str(destination).lower() in existing:
                            continue
                        references.addMemoryReference(
                            site,
                            destination,
                            RefType.COMPUTED_CALL,
                            SourceType.ANALYSIS,
                            -1,
                        )
                        report["references_added"] += 1
                program.endTransaction(transaction, True)
            except Exception:
                program.endTransaction(transaction, False)
                raise
            report["sites"] = [str(site) for site in sites]
            report["handlers"] = len(targets)
            program.save("replay observed screen dispatch", None)
    finally:
        dispose_sessions()
        project.close()
    return report
