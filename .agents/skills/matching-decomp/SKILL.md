---
name: matching-decomp
description: Recover Wizardry 8 C++ bodies against the pinned VC6 target and interpret focused comparison mismatches.
---

# Matching decompilation

Use this skill for function-body recovery, source placement, matching annotations, inlining/header
visibility, source-oracle lookup, and focused reccmp mismatches.

Route other questions instead of expanding this skill:

- live Ghidra inspection/edits/checkpoints: [ghidra-analysis](../ghidra-analysis/SKILL.md);
- prototypes, fields, globals, enums, layouts, classes, inheritance or vtables:
  [type-modeling](../type-modeling/SKILL.md);
- runtime/UI/loading behavior: [runtime-bringup](../runtime-bringup/SKILL.md);
- build/reccmp/clang/source-index/CLI infrastructure: [tooling-maintenance](../tooling-maintenance/SKILL.md).

For comparison details use [comparison](references/comparison.md); for unexplained codegen differences
use [mismatch patterns](references/mismatch-patterns.md); for possible SGP/MSVC/zlib/IJG/Info-ZIP
source use [source oracles](references/source-oracles.md). Read only the reference needed for the task.

## Recovery loop

1. Identify the requested entity and its existing C++/header/TU owner. Search source and accepted
   oracles before recovering anything new. Reuse reviewed evidence unless missing, stale or contradictory.
2. Consult `uv run wiz8 report context ADDRESS...` when TU/source/provenance context helps. Uncertain
   placement blocks insertion, not investigation.
3. Inspect only unanswered retail facts. Use the Ghidra-analysis skill rather than rediscovering or
   wrapping native APIs.
4. Correct established analysis facts before relying on them. For type/layout changes follow
   type-modeling and update the canonical declarations/consumers as one coherent batch.
5. Recover straightforward authored circa-2000 C++; do not reproduce compiler lowering or tweak source
   spelling merely to manipulate registers/CFG/score.
6. Run focused linked comparison, including affected callers when a shared declaration/ABI changed.
7. If final linkage obscures the question, select the appropriate object/data/vtable modality from the
   comparison reference. Stop when no evidence-backed source correction remains.

`uv run wiz8 recover function ADDRESS...` is an optional candidate generator. It writes disposable
artifacts; it does not edit source, build or compare and is never a prerequisite.

## Compare the recovered function

```sh
uv run wiz8 compare 0x0044e010
uv run wiz8 compare 0x0044e010 0x0044db60
uv run wiz8 compare --file src/wiz8/engine_code/Prop.cpp
uv run wiz8 compare --changed
```

Selected `compare` refreshes the compiler-backed source index and builds the comparison product itself.
Do not pre-run `analyze source-index`, `check` or `build` merely to prepare it.

- `exact` / `effective`: stop investigating that body unless another acceptance criterion remains.
- `mismatch`: inspect the first meaningful divergence and form a concrete source/type/ABI/lifetime/
  ownership hypothesis before editing. A percentage or changed CFG is not source evidence.
- `inconclusive` / `missing`: identify the absent pairing/evidence/analysis; do not claim equivalence.

Revert demonstrated semantic/ABI regressions. When no evidence-backed correction remains, keep the
straightforward source and report the unresolved mismatch rather than inventing compiler folklore.

## Accepting a substantial new body

A large dispatcher/multi-branch body is accepted only when:

- structural/identity/cast gates for the affected tree are green;
- typed objects do not escape through unexplained byte-pointer arithmetic;
- existing address identities agree on one name, normalized prototype and calling convention;
- comparison is meaningful, or inconclusive regions received explicit retail CFG/call/branch review.

Inconclusive matching evidence never excuses a wrong branch, field read, assertion path, call target or
side effect. Small straightforward bodies do not need a ritual manual CFG pass.

## Markers and placement

`FUNCTION` sits immediately above the declaration/definition it owns. Put pragmas, explanatory
comments and unrelated preprocessor lines above the marker, never between marker and entity.
`TEMPLATE` is immediately followed by the emitted-symbol comment and owns no body. `LIBRARY` is
address-only. `SYNTHETIC` is immediately followed by its generated-identity comment and owns no
declaration/body. Keep `GLOBAL` at the canonical definition.

Preserve TU ownership/order in `src/wiz8/sources.cmake`. An independently emitted ordinary destructor
uses `FUNCTION`; compiler deleting wrappers are `SYNTHETIC`; template emissions are `TEMPLATE`.

## Header visibility and inlining

An inlined copy does not by itself prove an authored `inline` or header body:

- copies only in one proven TU: no placement conclusion;
- copies in multiple independently proven TUs with no out-of-line body: strong header-visibility evidence;
- an out-of-line emission plus inlined copies: ordinary compiler behavior; keep normal source structure.

Do not create a `.cpp` solely to park `VTABLE`, `TEMPLATE`, `SYNTHETIC`, globals, or unrelated recovered
bodies. A normal `.cpp` should represent a proved retail translation unit. Unknown ownership stays an
unresolved fragment. Compiler-emission files are exceptional and contain no arbitrary game logic.

Never manually inline a function at call sites or add explicit specialization/instantiation solely to
force one VC6 emission. SGP/DLL exports are declarations, not product-header inline definitions; an
exported symbol proves the original call crosses that binary interface.

Write large decompilations/listings to named `build/` artifacts and print only the useful result/path.
Do not repeatedly dump whole files or inspect implementation internals merely to discover a documented
workflow.
