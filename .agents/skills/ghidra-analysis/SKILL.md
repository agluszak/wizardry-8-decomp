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

## Recovery preflight

Before the first Ghidra-derived recovery step in a genuinely new task, or after switching/rebasing to a
revision that changes `vendor/ghidra/exports/manifest.json` or a reviewed GZF, run:

```sh
uv run wiz8 doctor
```

Do not infer freshness from the program name, retail binary hash, an existing `ghidra-project/`, or the
fact that `open_program()` can find a program. A checkout can retain an older live analysis after a
newer reviewed GZF lands. `doctor` checks checkout ownership plus the reviewed-seed provenance recorded
when the project was restored; it does not repair or replace analysis state.

The Ghidra freshness states are intentional:

- `not-restored` is safe: no live project exists yet and the canonical opener will restore the current
  reviewed seed on first use;
- `current` is safe: the live project records the reviewed GZF hash required by this revision;
- `stale`, `untracked`, or `unknown` blocks retail-derived work. Do not continue analysis from that
  project until its state is explicitly reconciled or refreshed.

A legacy project can therefore fail doctor even when it may happen to contain equivalent analysis: the
point is that freshness is not provable. Do not silence or bypass that check. If live edits need to be
preserved, follow [checkpoints](references/checkpoints.md). If there is no live work to preserve,
replace the checkout-owned project only as an explicit state-management action rather than silently
having doctor/opening code overwrite it.

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

## Analysis enrichment

Whole-program decompiler quality comes from enriching the analysis database, not from pretty-printer
tweaks. Follow [analysis enrichment](references/analysis-enrichment.md) for the ordered roadmap
(benchmark → calling conventions → source projection → class Structures → globals/callbacks →
attributes → dual decompiler profiles). Score enrichment changes with
`uv run wiz8 analyze decompiler-quality` before promoting them into reviewed state.

## Checkpoints and bulk projection

Ordinary analysis edits need `program.save`, not a GZF ritual. Read
[checkpoints](references/checkpoints.md) only when sharing/restoring/reconciling reviewed Ghidra state.
Read [source import](references/source-import.md) for periodic high-confidence enrichment
checkpoints and for full canonical regeneration from the rebuilt source/PDB. Those are
state-management operations, not prerequisites for normal inspection or recovery.
