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
2. Inspect native analysis with `uv run wiz8 ghidra decompile ADDRESS...`, `ghidra asm ADDRESS...`,
   or `ghidra sym ADDRESS...`. These reads do not compile source. Uncertain placement blocks insertion,
   not investigation. If ProgramDB is missing an established declaration, run `uv run wiz8 ghidra sync`.
3. Before writing a nontrivial body, do the source-model audit below: establish parameter contracts,
   search for existing abstractions/inlined helpers, and settle touched ownership/lifetime/type facts.
4. Inspect only unanswered retail facts. Capstone, objdump and raw-byte inspection remain valid for
   independent verification. When falling back because of a tooling defect, record the specific missing
   information in the task handoff.
5. Correct established analysis facts before relying on them. Project them with `wiz8 ghidra sync`;
   for type/layout source changes follow type-modeling and update the canonical declarations/consumers
   as one coherent batch.
6. Recover straightforward authored circa-2000 C++; do not reproduce compiler lowering or tweak source
   spelling merely to manipulate registers/CFG/score.
7. Run focused linked comparison, including affected callers when a shared declaration/ABI changed.
8. For an almost-match dominated by stack-offset/local-order differences, use `reccmp-stackcmp` as a
   diagnostic before guessing at source changes. If final linkage obscures the question, select the
   appropriate object/data/vtable modality from the comparison reference. Stop when no evidence-backed
   source correction remains.

`uv run wiz8 ghidra decompile ADDRESS...` prints readable C with address, ProgramDB prototype,
attached source declaration, source-index freshness, ABI warnings, and artifact paths. Named
source/ProgramDB defects (`programdb-prototype-empty`, `source-parameter-count-mismatch`, and
related kinds) are also prepended as `// defect:` comments. `--json` serializes that same result.
The command does not edit source, build or compare and is never a prerequisite.

## Before writing source

For a substantial body, reconstruct the source contract before transcribing control flow:

- **Parameters:** inspect representative callers and the callee's uses. Record which arguments are
  inputs, outputs, in/out values, flags and optional pointers. Do not overwrite an input before its
  first semantic use merely because VC6 reused its stack slot later.
- **Compiler storage:** stack-slot/register/spill/temporary reuse belongs to VC6 lowering. Never alias a
  parameter or local to reproduce it; use the logical source variables even if the score gets worse.
- **Abstractions:** search existing source and accepted oracles for matching container methods, math
  operations, traversals, conversions and lifecycle helpers. A repeated nontrivial sequence in
  independently proven TUs is a reason to investigate a header/inline helper, not to duplicate it.
- **Inlining:** an inlined instruction sequence does not authorize manual inlining. Recover the likely
  helper/source abstraction first, then let the compiler decide where to inline it.
- **Types and raw offsets:** if a touched repository-owned object already has a canonical owner, model
  the field/subobject there instead of adding byte-pointer arithmetic or an overlay cast. Leave raw
  storage only when the fact genuinely remains unresolved and say why.
- **Lifetime:** for code that allocates, adopts, inserts, removes, completes, destroys or releases
  pointers, inspect sibling operations as a family. Trace allocation -> ownership transfer -> removal
  -> destruction before deciding between `new/delete`, `malloc/free`, container ownership, or no free.
- **Retail oddities:** preserve established bugs and UB. Do not add initialization, bounds checks,
  clamping, guards or deterministic defaults unless retail/source evidence says they existed.

Comments should record non-obvious evidence, intentional retail oddities and unresolved facts. Do not
narrate obvious control flow. Re-read comments after renames/TU moves and delete stale provenance or
claims that no longer agree mechanically with the body.

Do not incidentally edit `src/sgp` during ordinary Wizardry/SurRender recovery. If the evidence points
to an SGP source difference, treat that as an SGP/source-oracle task and preserve its modification
notice requirements.

## Compare the recovered function

```sh
uv run wiz8 compare 0x0044e010
uv run wiz8 compare 0x0044e010 0x0044db60
uv run wiz8 compare --file src/wiz8/engine_code/Prop.cpp
uv run wiz8 compare --changed
```

`compare` reads the existing source index and comparison product. Pass `--build` when source edits
require refreshing them, for example `uv run wiz8 compare --build --changed`. Do not pre-run
`analyze source-index` or `check` merely to prepare that explicit build-and-compare operation.

- `exact` / `effective`: stop investigating that body unless another acceptance criterion remains.
- `mismatch`: inspect the first meaningful divergence and form a concrete source/type/ABI/lifetime/
  ownership hypothesis before editing. A percentage or changed CFG is not source evidence.
- `inconclusive` / `missing`: identify the absent pairing/evidence/analysis; do not claim equivalence.

When a `mismatch` is already structurally close and the repeated differences are stack operands or
local-slot offsets, run `uv run reccmp-stackcmp --target TARGET ADDRESS` on the original address. Use
its 1:1/reordered/non-bijective mappings to decide what source fact to investigate next. It is not a
pass/fail gate and stack layout is not source evidence: never alias locals/parameters or distort
lifetimes merely to reproduce VC6 storage reuse. See the comparison reference for interpretation.

Revert demonstrated semantic/ABI regressions. When no evidence-backed correction remains, keep the
straightforward source and report the unresolved mismatch rather than inventing compiler folklore.

## Accepting a substantial new body

A large dispatcher/multi-branch body is accepted only when:

- structural/identity/cast gates for the affected tree are green;
- typed objects do not escape through unexplained byte-pointer arithmetic;
- existing address identities agree on one name, normalized prototype and calling convention;
- parameter contracts and ownership/lifetime transitions have been checked against callers/siblings;
- comparison is meaningful, or inconclusive regions received explicit retail CFG/call/branch review.

Inconclusive matching evidence never excuses a wrong branch, field read, assertion path, call target or
side effect. Small straightforward bodies do not need a ritual manual CFG pass.

For a multi-function/batch recovery, focused compares done while coding are not enough. After the last
source edit run `uv run wiz8 compare --changed` and account for every new or materially changed
`FUNCTION`: exact/effective, explained compiler-lowering mismatch, or explicitly inconclusive with the
retail review that justifies accepting it. Do not publish a batch with an unaccounted changed body.

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

Never manually inline a function at call sites. A concrete `TEMPLATE` emission proves only that the
compiler instantiated the primary template for those arguments; retail codegen is not evidence of an
authored `template <>`, explicit instantiation, or per-type body. Recover the operation at the primary
template owner and keep the concrete address as marker-only emission provenance. If only one
instantiation is observed, leave unsupported generic facts uncertain rather than manufacturing a
specialization. Explicit specialization/instantiation requires an accepted original-source oracle that
directly shows it; the template-model gate deliberately has no comment waiver. SGP/DLL exports are
declarations, not product-header inline definitions; an exported symbol proves the original call crosses
that binary interface.

Write large decompilations/listings to named `build/` artifacts and print only the useful result/path.
Do not repeatedly dump whole files or inspect implementation internals merely to discover a documented
workflow.
