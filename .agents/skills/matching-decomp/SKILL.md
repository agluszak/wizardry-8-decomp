---
name: matching-decomp
description: Recover Wizardry 8 C++ bodies and declarations against the pinned VC6 target; use for source-model changes and interpreting focused comparison mismatches.
---

# Matching decompilation

## Choose the primitive

- Ordinary linked recovered function: `just compare ADDRESS...` (examples below).
- COFF contributions, relocations, folding/aliases, data, vtables, or address mapping:
  [comparison](references/comparison.md).
- Native Ghidra reads/edits or divergent GZF checkpoints: [PyGhidra](references/pyghidra.md).
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
just recover ADDRESS...
```

`recover` writes candidate artifacts without editing source, building, or comparing. Neither command
is a prerequisite. Uncertain placement blocks insertion, not investigation.
If an incoming reviewed GZF diverges from the local checkpoint, use the
[checkpoint merge procedure](references/pyghidra.md#divergent-gzf-checkpoints); never replace one side wholesale.

## Compare the recovered function

```sh
just compare 0x0044e010
just compare 0x0044e010 0x0044db60
just compare --file src/wiz8/engine_code/Prop.cpp
just compare --changed
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

## Placement and output

`FUNCTION` sits immediately above its declaration. `TEMPLATE` is immediately followed by a comment
naming the emitted symbol and owns no body. `LIBRARY` is address-only. `SYNTHETIC` is immediately
followed by its exact generated identity comment and owns no declaration/body; separate it from the
next source entity or give that entity its own marker. An independently emitted ordinary destructor
uses `FUNCTION`, a template emission uses `TEMPLATE`; absent a standalone body, use only the
declaration/inline destructor required by the evidenced hierarchy. Keep `// GLOBAL` at canonical definitions.

Batch related Ghidra reads in one session. Keep native objects while computing; filter before printing.
Write large listings/decompilations to named `build/` artifacts and print the useful result/path.
Do not repeatedly dump whole files or parse textual output when a structured API/result exists.
