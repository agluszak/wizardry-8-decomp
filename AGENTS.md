# Wizardry 8 decompilation

This is an evidence-driven reconstruction of Wizardry 8 and SurRender using the original VC6-era
toolchain.

## Current stage

Initial bring-up is over. Nearly all authored functions are paired and the primary task is now
**systematic reduction of paired retail/recomp mismatches**. Work by shared source-model cause:
types/layouts, ABI, ownership, globals, classes/templates/lifecycle, constants, floating-point
semantics and control flow. Do not treat thousands of remaining differences as independent functions.

A comparison is evidence about the current model, not an objective function. A clean decompilation
does not prove equivalence or original spelling, and a differing decompilation does not prove a bug.

## Evidence and source fidelity

- Retail instructions/call sites and accepted original-source oracles outrank inferred Ghidra types,
  generated output, comparison results and workflow documentation. Keep unresolved facts unknown.
- Recover plausible authored circa-2000 C++. Never shape source solely to influence registers, stack
  layout, instruction scheduling, CFG shape or decompiler text.
- Fix type/layout/ABI/ownership disagreements at their canonical owner. Do not hide them with casts,
  duplicate declarations, wrappers, aliases, invented unions or local byte-offset tricks.
- Compiler output is not authored source. ICF, storage reuse, widened copies, inlining, template
  emissions, thunks and deleting destructors do not establish source aliases or handwritten helpers.
- Preserve established retail bugs/UB. Do not add guards, initialization or safer behavior merely
  because the recovered code looks suspicious.
- Search existing source and accepted oracles before declaring a new abstraction or implementation.
  One entity has one canonical owner and one evidence-backed type.
- SurRender import/export spelling is ABI evidence, not source ownership. Follow
  `docs/libraries/surrender-import-visibility.md`.
- Do not incidentally edit released SGP source. SGP changes need accepted-source/retail evidence and
  the required modification notice.
- Never commit extracted trees, live Ghidra projects or build products. Only reviewed GZF checkpoints
  listed in `vendor/ghidra/exports/manifest.json` may be tracked.
- Work stays in our Wizardry/reccmp/Ghidriff forks. Do not prepare or suggest upstream submissions
  unless explicitly requested.

Detailed fidelity rules live only in
[matching-decomp/references/source-fidelity.md](.agents/skills/matching-decomp/references/source-fidelity.md).

## Tool ownership

- Git/C++ owns recovered source, declarations, placement and build configuration.
- Ghidra owns retail analysis. `ghidra decompile|asm|sym` are reads; `ghidra sync` projects
  established source facts into ProgramDB.
- reccmp owns entity identity/pairing and comparison preparation.
- Ghidriff owns decompiled-code differencing.
- `uv run wiz8` composes project workflows.

Do not create a second differ, semantic-equivalence engine, symbol database, parallel inventory,
generic query protocol or alternate runtime harness when the existing owner can answer the question.

**Provenance matters:** a fact projected from current recovered source or recomp PDB into retail
ProgramDB is useful analysis annotation, but it is not independent retail evidence confirming that
same source fact.

## Work by recovery campaign

For a mismatch campaign:

1. Start from saved comparison reports; do not launch a fresh whole-image run just to begin.
2. Choose a coherent cluster/family and identify the shared owner before editing callers.
3. Inspect only unanswered retail facts, batching related Ghidra reads.
4. Fix the canonical source model and all affected consumers coherently.
5. Continue through a substantial batch — normally at least ~100 affected functions — before expensive
   verification.
6. Rebuild/compare once for the batch, inspect gains/regressions, and fix batch-level problems.
7. Stop when no evidence-backed correction remains; retain faithful source even if representation or
   established lowering still differs.

The goal is to reduce **unclassified mismatches and high-fanout source-model defects**, not merely to
raise the clean-function count.

## Verification discipline

Tests and full comparisons are expensive and should be **rare** during recovery campaigns.

- During investigation prefer saved reports, source search, Ghidra reads and narrowly focused
  comparisons only when they resolve a concrete uncertainty.
- Do not run `pr-check`, broad test suites, runtime suites, full comparisons or formatting suites
  after individual edits.
- For mismatch work, normally wait until a coherent batch affects ~100+ functions before rebuilding
  and running the relevant comparison.
- Reuse successful results while their relevant inputs have not changed.
- Near completion of a source batch run `uv run wiz8 compare --changed`, then
  `uv run wiz8 pr-check` and
  `uv run wiz8 report merge-preservation --base origin/main`.
- Run runtime scenarios only when the batch plausibly changes the behavior they observe.
- Tooling-only work uses the smallest focused tests for the changed owner. Documentation-only work
  needs diff inspection, not product tests.

A gating failure is a source/model/tooling defect to fix, not a reason to accumulate waivers.

## Task skills

Load only the skill needed for the current task and only the references it routes to:

- [matching-decomp](.agents/skills/matching-decomp/SKILL.md): systematic recovery campaigns,
  comparison triage, source placement and source oracles.
- [type-modeling](.agents/skills/type-modeling/SKILL.md): prototypes, fields, globals, layouts,
  inheritance, vtables and lifecycle ABI.
- [ghidra-analysis](.agents/skills/ghidra-analysis/SKILL.md): live retail analysis, provenance,
  ProgramDB edits and checkpoints.
- [runtime-bringup](.agents/skills/runtime-bringup/SKILL.md): runtime/behavioral validation and
  debugger use.
- [tooling-maintenance](.agents/skills/tooling-maintenance/SKILL.md): Python/CMake/reccmp/Ghidriff,
  source indexing, reports and validation infrastructure.
- [jujutsu-workflow](.agents/skills/jujutsu-workflow/SKILL.md): load only for rebasing, existing PR
  history, conflicts or publication; ordinary edits in the current change do not require it.

## Scope and publication

Fix forward when the task exposes a concrete shared defect, but do not turn a focused task into
unrelated repository cleanup. Repository-owned APIs/formats have no compatibility contract: replace
obsolete machinery instead of adding compatibility layers.

Use the provided checkout; do not create another workspace/clone unless explicitly requested. Preserve
unrelated work. Before relying on retail/Ghidra state after adopting a genuinely new base, run
`uv run wiz8 doctor`. Jujutsu mechanics and publication live in the dedicated skill.
