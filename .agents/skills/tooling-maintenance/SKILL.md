---
name: tooling-maintenance
description: Maintain Wizardry recovery tooling, reccmp/Ghidriff integration, reports, source indexing, CMake and validation without duplicating pipeline owners.
---

# Tooling maintenance

Use this skill for the recovery pipeline rather than recovered game source.

## Architecture

Keep one owner per concern:

- `uv run wiz8`: project workflow composition;
- CMake: compile/link graph;
- reccmp: entity catalog/pairing and comparison preparation;
- Ghidra: retail analysis;
- Ghidriff: decompiled-code differencing;
- runtime tooling: existing staging/debug/scenario infrastructure.

Delete redundant layers. Do not add a second differ, equivalence engine, marker database, source
inventory, runtime harness, generic query language or parallel report framework.

Observation commands (`compare`, `report compare`, `status`, `addr`, `vtable`, `datacmp`,
Ghidra reads) should consume existing state and must not silently rebuild/mutate it unless an explicit
`--build`/apply operation requests that.

## Current-stage mismatch tooling

New mismatch tooling should **classify and index existing comparison evidence**. It must not become
another semantic comparator.

Prefer extending existing structured comparison/report results with:

- factual call-sequence deltas;
- repeated field/width/global/literal fingerprints;
- class/template/lifecycle ownership;
- provenance of analysis corrections/normalizations;
- cluster counts and bounded representative examples.

A cluster/tag is triage metadata, never an equivalence verdict. Prefer one durable report over another
inventory subsystem.

## Ghidra and source projection

`wiz8 ghidra sync` is the one established-source -> ProgramDB apply path. Keep provenance explicit:
source/PDB-projected retail facts are not independent retail evidence for confirming that source.

`wiz8 check` owns compiler-backed source-index refresh. `uv run wiz8 analyze source-index --jobs N`
is the index-only owner when that projection itself needs work; reuse a valid projection and retain
native per-TU cache invalidation. Inspection/recovery commands consume the existing projection instead
of rebuilding it implicitly.

## Reports and output

Use saved comparison runs and `wiz8 report compare` for triage. Preserve detailed output under
`build/`; terminal output should be bounded counts, representative findings and artifact paths.
Filter before printing.

Exploratory scripts belong under `build/` or stdin Python. Create a public CLI only for a repeated
durable workflow.

Do not add compatibility shims for removed internal tooling; update current producers/consumers
together.

## Tests and validation

Tests are not a progress ritual.

- Add tests for a concrete tooling correctness bug or stable workflow contract existing checks missed.
- During substantial work, batch implementation before running broad suites.
- Use the smallest focused tests/lint/type check for the changed owner.
- Do not run product builds or runtime scenarios when a focused tooling test exercises the same path.
- Reuse successful checks until relevant inputs change.
- Documentation/skill-only changes need diff/link inspection, not product tests.

`wiz8 check` / `pr-check` use the fast non-integration test lane. Real VC6/runtime-stub integration
checks and live Ghidra tests remain explicit dedicated lanes; do not duplicate them in the fast suite.

Broad `pr-check` belongs near completion of the coherent change, not after each helper edit.
