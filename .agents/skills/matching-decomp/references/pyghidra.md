# Direct PyGhidra

For ordinary binary inspection/editing, do not launch or locate Ghidra manually. Use
`open_program(settings, "wiz8")`. It handles repository configuration, startup, seed restoration when
needed, and exclusive project ownership, yielding the existing canonical native `Program`.
Do not start daemons, copy projects, or create a scratch Ghidra checkout.

## Open and inspect

Run from the repository root in the existing environment. This example reads the stored prototype;
it does not establish that the prototype is correct.

```sh
uv run python - <<'PY'
import pyghidra
from wiz8decomp.config import load_settings
from wiz8decomp.ghidra.env import open_program

settings = load_settings()
with open_program(settings, "wiz8") as program:
    # Import ghidra.* / java.* here, after the opener has started the JVM.
    entry = program.getAddressFactory().getAddress("00488650")
    function = program.getFunctionManager().getFunctionAt(entry)
    if function is None:
        raise LookupError(f"No function at {entry}")
    print(function.getPrototypeString(False, True))
    print(function.getCallingConventionName(), function.hasCustomVariableStorage())
    print("return", function.getReturnType(), function.getReturn().getVariableStorage())
    for parameter in function.getParameters():
        print(parameter.getOrdinal(), parameter.getName(),
              parameter.getDataType().getPathName(), parameter.getVariableStorage())
PY
```

Use `getFunctionContaining(entry)` when the address is inside a function rather than at its entry.
Import `wiz8decomp`, not `tools.wiz8decomp`. A saved scratch script can be run with
`uv run python - < build/inspect.py`; keep it disposable. Do not name scratch modules `ghidra.py`,
`java.py`, or `jpype.py`. Consult the pinned installation's API documentation/source or matching
`ghidra-stubs` for exact overloads rather than guessing them through repeated exceptions.

The opener normally restores the seed itself. If the canonical local project is absent and an
explicit restore is needed, the current CLI takes an option (not a positional program name):

```sh
uv run wiz8 ghidra restore --program wiz8
```

Do not restore over ordinary live work or open the same live project in a competing process.

## Native inspection and decompilation

Inside the open-program block, select native objects for the unanswered question:

```python
listing = program.getListing()
references = program.getReferenceManager()
types = program.getDataTypeManager()
instruction = listing.getInstructionAt(entry)
data = listing.getDataAt(entry)
incoming = references.getReferencesTo(entry)
record = types.getDataType("/actual/category/ExistingType")  # use its established path
```

Inspect call-site instructions, references, and storage when a prototype looks wrong. A rooted-flow
query uses the current inferred prototype: empty results do not prove an argument/field is unused,
and cannot reveal a parameter the model omitted. Search the actual data-type manager before declaring
anything; missing fields in a projection do not establish an absent type.

Reuse one decompiler for a related batch; keep large output in named artifacts:

```python
from pathlib import Path
from ghidra.app.decompiler import DecompInterface

decompiler = DecompInterface()
try:
    if not decompiler.openProgram(program):
        raise RuntimeError(decompiler.getLastMessage())
    for function in functions:  # selected native Function objects
        result = decompiler.decompileFunction(function, 60, pyghidra.task_monitor())
        if not result.decompileCompleted():
            raise RuntimeError(result.getErrorMessage())
        output = Path("build") / f"decompile-{function.getEntryPoint()}.c"
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(str(result.getDecompiledFunction().getC()), encoding="utf-8")
        print(output)
finally:
    decompiler.dispose()
```

After analysis edits, call `decompiler.flushCache()` on a reused interface, discard stale
`DecompileResults`/`HighFunction` objects, and regenerate affected caller/callee output. Reopening an
interface also discards its cache. Do not rerun whole-program analysis by default.

## Edit established facts

For an ordinary prototype edit, put the evidence-backed declaration in a disposable
`build/signature.txt`. Inside the open-program block above, after resolving the function, use:

```python
from pathlib import Path
from ghidra.app.cmd.function import ApplyFunctionSignatureCmd
from ghidra.app.util.parser import FunctionSignatureParser
from ghidra.program.model.symbol import SourceType

with pyghidra.transaction(program, "Correct reviewed signature"):
    parser = FunctionSignatureParser(program.getDataTypeManager(), None)
    signature = parser.parse(
        function.getSignature(), Path("build/signature.txt").read_text(encoding="utf-8")
    )
    command = ApplyFunctionSignatureCmd(entry, signature, SourceType.USER_DEFINED)
    if not command.applyTo(program, pyghidra.task_monitor()):
        raise RuntimeError(command.getStatusMsg())
program.save("Correct reviewed signature", pyghidra.task_monitor())
```

Keep the existing function name and calling convention unless the evidence calls for changing them.
The parser is not a full C++ parser. For unsupported template names, custom storage, or new records,
use native `DataTypeManager`, `ParameterImpl`, and `Function` APIs directly, not another parser or
wrapper command. Resolve existing data types by their actual paths; do not assume every type is
under `/wiz8/classes`, choose an arbitrary duplicate, or silently replace a conflicting structure.
`Function.updateFunction` takes a return `Variable` (or null to preserve it), not `getReturnType()`;
use an explicit JPype Java array when the selected overload requires one.

Batch related type and caller/callee edits in one transaction and save once after success.
`pyghidra.transaction` commits on normal exit and rolls back on an escaping exception; for a purely
speculative edit, explicitly roll back a native transaction instead of saving or cloning a project.
Keep unresolved facts unknown; correct only established facts without a separate permission round
within the recovery task. Preserve valid observations when changing inspection tools. Filter before
printing; keep native objects while computing rather than serializing whole graphs or inventing JSON
query interfaces.

## Share a reviewed checkpoint

After a coherent reviewed batch, close the program session and export accepted live state:

```sh
uv run wiz8 ghidra seed refresh wiz8
```

This refreshes the tracked reviewed GZF checkpoint. It is a sharing/checkpoint operation, not a
per-signature-edit ritual or a substitute for `program.save`.

## Regenerate the canonical state from the source PDB

When the task owner asks to update or regenerate the Ghidra state, the direction is source ->
Ghidra: project the current compiled model into the canonical live program before refreshing the
checkpoint. This is bulk, one-shot regeneration, distinct from both a checkpoint merge and an
ordinary `seed refresh`.

1. Commit or refresh the current checkpoint first; it is the rollback point.
2. Build a current VC6 PDB:

   ```sh
   just build wiz8
   ```

3. Import the matched entities into the canonical project. `reccmp-ghidra-import` is the applier;
   there is no CLI wrapper, so call the wired helper with the real settings:

   ```sh
   uv run python - <<'PY'
   from wiz8decomp.config import load_settings
   from wiz8decomp.ghidra.reccmp_import import import_reccmp_source

   print(import_reccmp_source(load_settings(), "wiz8"))
   PY
   ```

   It matches recompiled entities to original addresses and applies names, signatures, types and
   source lines. Review the reported `Statistics` (successes/functions changed) and note the
   tolerated `CodeUnitInsertionException`/`TypeNotImplementedError`/`ParameterMismatchError`
   counts rather than demanding zero.
4. Spot-check recovered identities with `uv run wiz8 report context ADDRESS`, then re-export and
   commit the checkpoint:

   ```sh
   uv run wiz8 ghidra seed refresh wiz8
   ```

Do not point the importer at a derived or cached project, and do not run it speculatively: it
mutates the reviewed program. `uv run wiz8 analyze source-layouts` uses the same importer against a
hash-cached derived project under `build/ghidra-verify/` and deliberately leaves the canonical state
untouched; use that when only the audit is wanted.

## Divergent GZF checkpoints

Use the common reviewed ancestor as `BASE` and the other reviewed archive as `INCOMING`. Preview:

```sh
uv run python tools/ghidra-scripts/merge_checkpoint_functions.py BASE INCOMING
```

Review the selected/pending function changes. Apply only after reviewing that preview, then export
the reconciled canonical live state:

```sh
uv run python tools/ghidra-scripts/merge_checkpoint_functions.py BASE INCOMING --apply
uv run wiz8 ghidra seed refresh wiz8
```

BASE and INCOMING open immutably and must describe the same binary as the live program. The merge
targets the canonical live program in this checkout; it handles non-overlapping function-analysis
changes (functions, function-entry symbols, and tags). It refuses overlapping local edits, incoming
type changes, non-function symbol changes, and other unsupported analysis changes.

Refusal is deliberate: reconcile those changes explicitly using native Ghidra and retail/source
evidence. Never choose one entire GZF arbitrarily or by age. Do not invent a general merge engine,
signature ledger, replay log, or per-edit archive system.

Upstream reference: [PyGhidra API](https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_12.1.2_build/Ghidra/Features/PyGhidra/src/main/py/README.md).
