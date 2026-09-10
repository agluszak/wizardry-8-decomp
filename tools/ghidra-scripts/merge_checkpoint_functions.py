"""Merge reviewed, non-overlapping function analysis from two immutable GZF files.

Preview: uv run python tools/ghidra-scripts/merge_checkpoint_functions.py BASE INCOMING
Add --apply to save into this checkout's canonical live program. Export the
reviewed result separately with `uv run wiz8 ghidra seed refresh wiz8`.

This deliberately refuses other analysis changes. It is not a general Ghidra
merge engine: use native Ghidra inspection and resolve those changes explicitly.
"""

import argparse
import json
from pathlib import Path

import pyghidra
from wiz8decomp.config import load_settings
from wiz8decomp.ghidra.env import open_program
from wiz8decomp.ghidra.import_programs import HASH_OPTION


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("base", type=Path)
    parser.add_argument("incoming", type=Path)
    parser.add_argument("--program", default="wiz8")
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    with open_program(load_settings(), args.program) as program:
        from ghidra.framework.data import OpenMode
        from ghidra.framework.store.db import PackedDatabase
        from ghidra.program.database import ProgramDB
        from ghidra.program.model.address import AddressSet
        from ghidra.program.util import (
            ProgramDiff,
            ProgramDiffFilter,
            ProgramMerge,
            ProgramMergeFilter,
        )
        from java.io import File
        from java.lang import Object

        monitor = pyghidra.task_monitor()
        consumer = Object()
        archives = []
        databases = []
        try:
            for path in (args.base, args.incoming):
                packed = PackedDatabase.getPackedDatabase(File(str(path.resolve())), True, monitor)
                archives.append(packed)
                databases.append(
                    ProgramDB(packed.open(monitor), OpenMode.IMMUTABLE, monitor, consumer)
                )
            base, incoming = databases
            hashes = {
                item.getOptions("Program Information").getString(HASH_OPTION, None)
                for item in (base, incoming, program)
            }
            if None in hashes or len(hashes) != 1:
                raise ValueError("Checkpoints and live program must describe the same binary")

            base_types = {
                str(t.getPathName()): t for t in base.getDataTypeManager().getAllDataTypes()
            }
            incoming_types = {
                str(t.getPathName()): t for t in incoming.getDataTypeManager().getAllDataTypes()
            }
            changed_types = [
                path
                for path in sorted(base_types.keys() | incoming_types.keys())
                if path not in base_types
                or path not in incoming_types
                or not base_types[path].isEquivalent(incoming_types[path])
                or base_types[path].getLastChangeTime() != incoming_types[path].getLastChangeTime()
            ]
            if changed_types:
                raise ValueError(f"Data type changes require explicit resolution: {changed_types}")

            supported = (
                ProgramDiffFilter.FUNCTION_DIFFS
                | ProgramDiffFilter.SYMBOL_DIFFS
                | ProgramDiffFilter.FUNCTION_TAG_DIFFS
            )
            delta = ProgramDiff(base, incoming)
            unsupported = delta.getDifferences(
                ProgramDiffFilter(ProgramDiffFilter.ALL_DIFFS & ~supported), monitor
            )
            if not unsupported.isEmpty():
                raise ValueError(f"Unsupported incoming analysis changes: {unsupported}")
            selected = AddressSet(delta.getDifferences(ProgramDiffFilter(supported), monitor))
            for address in selected.getAddresses(True):
                if (
                    base.getFunctionManager().getFunctionAt(address) is None
                    and incoming.getFunctionManager().getFunctionAt(address) is None
                ):
                    raise ValueError(f"Non-function symbol change requires review: {address}")

            pending = AddressSet(
                ProgramDiff(program, incoming, selected).getDifferences(
                    ProgramDiffFilter(supported), monitor
                )
            )
            if not pending.isEmpty():
                local = ProgramDiff(base, program, pending).getDifferences(
                    ProgramDiffFilter(ProgramDiffFilter.ALL_DIFFS), monitor
                )
                if not local.isEmpty():
                    raise ValueError(
                        f"Concurrent analysis edits require explicit resolution: {local}"
                    )
                if args.apply:
                    with pyghidra.transaction(program, "Merge reviewed checkpoint functions"):
                        merge = ProgramMerge(program, incoming)
                        merge.mergeFunctions(pending, monitor)
                        merge.mergeLabels(pending, ProgramMergeFilter.REPLACE, monitor)
                        merge.applyFunctionTagChanges(
                            pending, ProgramMergeFilter.REPLACE, None, None, monitor
                        )
                        if str(merge.getErrorMessage()).strip():
                            raise RuntimeError(merge.getErrorMessage())
                        remaining = ProgramDiff(program, incoming, selected).getDifferences(
                            ProgramDiffFilter(supported), monitor
                        )
                        if not remaining.isEmpty():
                            raise ValueError(f"Incomplete merge: {remaining}")
                    program.save("Merge reviewed checkpoint functions", monitor)
            print(
                json.dumps(
                    {
                        "incoming": str(selected),
                        "pending": str(pending),
                        "applied": args.apply and not pending.isEmpty(),
                    }
                )
            )
        finally:
            for database in databases:
                database.release(consumer)
            for packed in archives:
                packed.dispose()


if __name__ == "__main__":
    main()
