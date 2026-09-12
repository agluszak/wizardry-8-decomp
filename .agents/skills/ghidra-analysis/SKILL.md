---
name: ghidra-analysis
description: Inspect or edit the canonical Wizardry 8 Ghidra analysis, including functions, prototypes, data types, references, decompilation, and reviewed checkpoints.
---

# Ghidra analysis

Use this skill when the task is about the retail analysis model itself: inspect instructions/P-code,
references, functions, prototypes, storage, data types, vtables, or make an evidence-backed correction
to the reviewed program. Source recovery and comparison remain in
[matching-decomp](../matching-decomp/SKILL.md); source type/layout decisions belong in
[type-modeling](../type-modeling/SKILL.md).

## Open the canonical program

For ordinary inspection or edits, use the existing checkout-owned project:

```python
import pyghidra
from wiz8decomp.config import load_settings
from wiz8decomp.ghidra.env import open_program

settings = load_settings()
with open_program(settings, "wiz8") as program:
    ...
```

Import `ghidra.*` / `java.*` only after the opener starts the JVM. Do not launch Ghidra manually,
start a daemon, copy the project, create a scratch project, or share a live project between checkouts.
Use `getFunctionAt()` for an entry and `getFunctionContaining()` for an interior address.

Prefer native `Program` APIs for the unanswered question. Batch related reads in one session, keep
native objects while computing, filter before printing, and put large listings/decompilations under
`build/`. Do not invent a string-command query protocol, generic report schema, or JSON mirror of the
program merely to access an API.

## Decompile efficiently

Reuse one `DecompInterface` for a related batch. After an analysis edit, call `flushCache()`, discard
old `DecompileResults`/`HighFunction` objects, and re-decompile only affected functions. Reopening the
interface also drops its cache. Do not rerun whole-program analysis by default.

A rooted high-level-flow result depends on the currently stored prototype: an empty result does not
prove a parameter or field is unused and cannot reveal a parameter omitted from the model. Reconcile
call sites, storage and instructions before changing a signature.

## Edit established facts

Apply only evidence-backed corrections. Use one native transaction for a coherent batch and save once:

```python
with pyghidra.transaction(program, "Correct reviewed analysis"):
    # native Function/DataTypeManager/SymbolTable edits
    ...
program.save("Correct reviewed analysis", pyghidra.task_monitor())
```

`pyghidra.transaction` commits on normal exit and rolls back on an escaping exception. For speculative
work, roll back instead of saving or cloning a project. Resolve existing data types by their actual
paths; do not assume one category, silently duplicate conflicting structures, or use parser failures as
a reason to add a wrapper API. `FunctionSignatureParser` is useful for ordinary declarations; custom
storage, templates and unsupported C++ syntax should use native `Function`, `ParameterImpl` and
`DataTypeManager` APIs directly.

Keep uncertain facts unknown. When source and Ghidra disagree, retail/source evidence decides which
owner is wrong; correct both owners when the fact is established. Do not repeatedly query a prototype
already known to be false.

## Checkpoints and bulk projection

Ordinary analysis edits need `program.save`, not a GZF ritual. Read
[checkpoints](references/checkpoints.md) only when sharing/restoring/reconciling reviewed Ghidra state.
Read [source import](references/source-import.md) only when explicitly regenerating canonical state
from the rebuilt source/PDB. Those are state-management operations, not prerequisites for normal
inspection or recovery.
