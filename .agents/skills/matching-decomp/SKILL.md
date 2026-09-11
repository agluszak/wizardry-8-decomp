---
name: matching-decomp
description: Recover Wizardry 8 C++ bodies and declarations against the pinned VC6 target; use for source-model changes and interpreting focused comparison mismatches.
---

# Matching decompilation

## Choose the primitive

- Ordinary linked recovered function: `uv run wiz8 compare ADDRESS...` (examples below).
- COFF contributions, relocations, folding/aliases, data, vtables, or address mapping:
  [comparison](references/comparison.md).
- Native Ghidra reads/edits, divergent GZF checkpoints, or regenerating canonical state from the
  source PDB: [PyGhidra](references/pyghidra.md).
- Type/prototype/layout disagreement or Clang diagnostics:
  [type and layout evidence](references/type-and-layout-evidence.md).
- Unexplained focused divergence: [mismatch patterns](references/mismatch-patterns.md).
- Possible SGP, MSVC/runtime, zlib, IJG, or Info-ZIP source: [source oracles](references/source-oracles.md).

Read only the relevant reference. Prefer documented project/reccmp entry points. Do not inspect
implementation code, `site-packages`, console-script metadata, or CLI parser internals merely to
discover an already documented workflow. Inspect implementation only to debug the tool itself or
resolve insufficient/contradictory documentation; fix stale invocation instructions at their owner.

## Recovery loop

1. Identify the requested entity and existing C++/header/TU owner. Search source and known oracles
   before recovering anything new; reuse evidence unless missing, stale, or contradictory.
   Consult the normal recovery context for original-TU ownership before placing a recovered game
   function; do not manually rediscover source ownership unless the returned evidence is ambiguous.
2. Inspect only unanswered binary facts. Direct PyGhidra through
   `wiz8decomp.ghidra.env.open_program(settings, "wiz8")` is the normal native inspection/edit path.
3. Correct demonstrably wrong Ghidra facts with native transactions; save the coherent batch and
   invalidate stale decompiler results. No separate permission round is needed within recovery.
   Keep uncertain facts unknown; do not repeatedly query a known-bad prototype.
4. Update one coherent source-model batch in its canonical owners, including affected declarations
   and consumers. Another similar occurrence alone does not expand the task (`AGENTS.md`).
5. Run focused linked comparison. Include callers affected by a shared declaration/layout/ABI change.
6. If final linkage obscures the question, choose the modality in [comparison](references/comparison.md).
   Stop when no evidence-backed source correction remains; report any unmet acceptance criterion.

Optional source-aware conveniences, when their joined output helps:

```sh
uv run wiz8 report context ADDRESS...
uv run wiz8 recover function ADDRESS...
```

`recover` writes candidate artifacts without editing source, building, or comparing. Neither command
is a prerequisite. Uncertain placement blocks insertion, not investigation.
If an incoming reviewed GZF diverges from the local checkpoint, use the
[checkpoint merge procedure](references/pyghidra.md#divergent-gzf-checkpoints); never replace one side wholesale.

## Compare the recovered function

```sh
uv run wiz8 compare 0x0044e010
uv run wiz8 compare 0x0044e010 0x0044db60
uv run wiz8 compare --file src/wiz8/engine_code/Prop.cpp
uv run wiz8 compare --changed
```

This builds the comparison product itself; do not separately build first. It returns structured
focused results, including the first reported divergence and available bounded instruction window.
Use that result instead of a second homemade triage command.

- `exact` / `effective`: stop investigating that body unless another task requirement remains.
- `mismatch`: inspect the first meaningful divergence; formulate a concrete source/type/ABI/lifetime/
  ownership hypothesis before editing. A percentage or changed CFG is not authored-source evidence.
- `inconclusive` / `missing`: identify absent evidence, pairing, or unsupported analysis; do not assume
  a source defect or claim equivalence.

Do not tweak source spelling for scores. Revert demonstrated regressions; retain straightforward
C++ when evidence supplies no correction, and report the unresolved result honestly.

## Accepting a substantial new body

A large newly recovered body - a dispatcher, a multi-branch switch, anything long enough that one
mismatch can hide another - is accepted only after all of the following hold:

- the cast, identity and structural gates are green for the affected tree;
- no unexplained byte-pointer escape survives from an object whose fields are already typed;
- every pre-existing address identity is reconciled to one name, one normalized prototype and one
  calling convention;
- the comparison is meaningful, or every inconclusive region has explicit manual CFG verification
  against retail: instruction stream, call targets and branch roles.

This is exactly the "inconclusive is acceptable" boundary that needs tightening. Inconclusive
comparison status is matching evidence, not semantic acceptance: it never excuses wrong assertion
control flow, a wrong field read, or an unverified branch. A 700-line dispatcher needs the manual
CFG pass; a small wrapper or a straightforward body does not.

## Placement and output

`FUNCTION` sits immediately above its declaration. `TEMPLATE` is immediately followed by a comment
naming the emitted symbol and owns no body. `LIBRARY` is address-only. `SYNTHETIC` is immediately
followed by its exact generated identity comment and owns no declaration/body; separate it from the
next source entity or give that entity its own marker. An independently emitted ordinary destructor
uses `FUNCTION`, a template emission uses `TEMPLATE`; absent a standalone body, use only the
declaration/inline destructor required by the evidenced hierarchy. Keep `// GLOBAL` at canonical definitions.

## Header visibility and inline

An inlined copy by itself does not place a body in a header or make it `inline` in the source:

- inlined copies in one proven translation unit only: no conclusion about header/source placement;
- inlined copies in multiple independently proven units with no out-of-line body: strong evidence the
  body was header-visible;
- an out-of-line emission plus some inlined copies: ordinary compiler behavior; do not restructure
  source to reproduce which calls were inlined;
- never manually inline a body at individual call sites, and never use an explicit specialization,
  explicit instantiation, or similar mechanism only to force an out-of-line copy.

SGP/DLL exports are declared, never defined inline in a product header: an exported symbol in the
import surface proves the original call went out of line.

Batch related Ghidra reads in one session. Keep native objects while computing; filter before printing.
Write large listings/decompilations to named `build/` artifacts and print the useful result/path.
Do not repeatedly dump whole files or parse textual output when a structured API/result exists.
