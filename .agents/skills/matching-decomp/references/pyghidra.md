# Direct PyGhidra

Use native Ghidra objects for exploratory reads and edits. The existing
`wiz8decomp.ghidra.env.open_program` supplies checkout configuration, startup, seed restoration when
needed, and exclusive project access. It yields the native `Program`; it is not a query protocol.
Do not open the same live project in a competing process or clone it for an ordinary investigation.

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
    for parameter in function.getParameters():
        print(parameter.getOrdinal(), parameter.getName(),
              parameter.getDataType().getPathName(), parameter.getVariableStorage())
PY
```

Import `wiz8decomp`, not `tools.wiz8decomp`. A saved scratch script can be run with
`uv run python - < build/inspect.py`; keep it disposable. Do not name scratch modules `ghidra.py`,
`java.py`, or `jpype.py`. Consult the pinned installation's API documentation/source or matching
`ghidra-stubs` for exact overloads rather than guessing them through repeated exceptions.

## Edit the established facts

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
Keep unresolved facts unknown. Refresh the existing tracked GZF checkpoint when sharing reviewed
analysis, not after every parameter edit; do not create a signature ledger or replay framework.

## Inspect only what the question needs

Use native listing/reference/data-type managers and `DecompInterface`; reuse one interface for a
related function batch and dispose it in `finally`. Check decompilation success before consuming
results. After editing analysis, flush a reused interface's cache and discard old `DecompileResults`
and `HighFunction` objects before regenerating the affected caller/callee output. Do not rerun
whole-program analysis by default.

Filter before printing. Put large decompilations or listings in named `build/` files and print their
paths. Inspect concrete call-site instructions and incoming storage when the inferred prototype is
wrong; repeated `param_N` queries against that prototype cannot recover an omitted argument.
Existing rooted-flow/lifecycle algorithms are optional helpers, not exhaustive evidence or mandatory
access paths. Do not mistake their empty results for proof that a parameter or field is unused.

Upstream reference: [PyGhidra API](https://github.com/NationalSecurityAgency/ghidra/blob/Ghidra_12.1.2_build/Ghidra/Features/PyGhidra/src/main/py/README.md).
