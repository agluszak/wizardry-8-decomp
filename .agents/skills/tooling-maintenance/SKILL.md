---
name: tooling-maintenance
description: Change Wizardry 8 recovery tooling, CLI orchestration, CMake, reccmp integration, source indexing, clang diagnostics, build/runtime plumbing, or repository validation.
---

# Tooling maintenance

Use this skill for tooling/infrastructure changes rather than recovered game source. The tooling is
agent-only: optimize for reliable agent composition, not interactive human workflows or pretty output.

## Architecture

Keep one owner for each concern:

- `uv run wiz8` is the project workflow interface; do not add a second task runner.
- CMake owns compilation/linking graphs; Python composes workflows around them.
- reccmp owns matching and its native source-index collection/cache behavior.
- Ghidra owns retail analysis; [ghidra-analysis](../ghidra-analysis/SKILL.md) defines live access.
  `wiz8 ghidra sync` is the one established-source → ProgramDB apply path. `ghidra decompile` /
  `ghidra asm` / `ghidra sym` are reads and must not compile, index, or synchronize.
- runtime staging/debugging use the existing shared primitives; do not duplicate product launch trees.

Delete a layer when upstream/native APIs already express the operation. Do not add wrappers merely to
rename an API, duplicate source inventories, parallel validation graphs, generic report frameworks,
string-command query protocols, replay databases, or separate human/JSON modes. A small typed helper is
worth keeping only when it contains project-specific analysis or removes repeated logic.

A command named after an observation (`compare`, `status`, `addr`, `vtable`, `datacmp`, `debug`,
`runtime-test`, or `run`) must not compile, regenerate analysis metadata, or repair state unless the
caller requests an explicit build operation (`--build` where supported). Read existing artifacts,
warn when they may be stale, and fail with the exact build/metadata command when required state is
absent.

## Doctor and recovery preflight

`uv run wiz8 doctor` is the machine/recovery preflight, not a repair command. Keep it cheap enough to
run after adopting a new task base: it may inspect manifests and checkout-local metadata, but it should
not start a long analysis pass, mutate Ghidra, rebuild products, or replace user state.

Doctor owns checks that can invalidate essentially every recovery conclusion before work starts:
pinned tool/runtime versions, required inputs/work directory, repository hygiene, live Ghidra project
ownership, whether the live canonical Wiz8 program is provably based on the reviewed GZF tracked by
the current revision, and whether established source facts have been projected into that ProgramDB.
A live program with the same retail binary hash can still be stale analysis. A current reviewed seed
does not imply current source projection.

Restoring a reviewed GZF records its archive hash in the checkout-owned project marker. A later manifest
change therefore makes the previous live project detectably stale. Legacy/untracked live projects have
unknown provenance and must fail the freshness check rather than being silently blessed from weak
heuristics such as function count, timestamps, or matching binary hashes. Doctor reports the problem;
checkpoint reconciliation/replacement remains an explicit Ghidra state-management operation.

## Source index

`uv run wiz8 check` refreshes the compiler-backed index. `uv run wiz8 analyze source-index`
is the index-only owner for debugging that projection; inspection and recovery do not run it.
Use `--jobs N` on the index-only command to tune native collector concurrency (default: up to eight
workers). Reuse a valid projection; when inputs change, retain native per-TU cache invalidation.
Inspection commands consume the existing projection and never refresh it implicitly.
`wiz8 lint` refreshes a missing or stale index through that same owner when header
dependency selection needs it; it does not require the entire `check` graph.

The project may adapt the lint compile database to the host/analysis-image boundary, but collection and
cache semantics belong to reccmp. Keep one native collection across configured link namespaces; do not
reintroduce per-target collectors, hand-maintained cache-input trees, another marker database, or a
parallel declaration-consistency model.

Paths that can run both locally and under Docker must be matched by repository-relative identity. Do
not bake `/repo` into clang-tidy filters or compile-database consumers. Environment-specific failures
should be fixed at the path/mount/compile-database boundary, not hidden with diagnostic suppression.

## Lint and validation

Keep the gating profile narrow and reconstruction-relevant. A diagnostic becomes gating when it finds
real source-model defects with an acceptable false-positive rate. Prove that with a whole-tree trial
over every recovered TU before gating a check, and record the trial's evidence and exclusions in
`.clang-tidy`; the checks already gated there were each clean across all 295 recovered translation
units. Proven retail/vendor ABI behavior wins over a generic lint heuristic; suppress such cases
narrowly at the affected construct instead of weakening the global profile.

`uv run wiz8 tidy-audit` is the non-gating counterpart to `wiz8 lint`, in the same relationship
`wiz8 diagnostics` has to it. `.clang-tidy-audit` holds checks that are too ambiguous to gate — enum
sentinels, `memset`/`memcpy`/`memcmp` over records, union layout heuristics. It reports the whole
corpus, never filters to changed lines, and never fails; treat a finding as evidence about the source
model, not a defect to repair. Never "fix" a finding to turn this lane green, and never promote a
check into `.clang-tidy` without the whole-tree trial above. The audit invokes the real clang-tidy
directly rather than the `clang-tidy` wrapper, because the wrapper's boolean-fact post-processing
belongs to the gating lane and exits non-zero.

Do not add tests by default. Add a test for a concrete correctness bug or stable public behavior that
existing checks missed. Avoid tests of source spelling, documentation text, inventory counts, generated
snapshots, deleted files or implementation-private helper order. Delete obsolete tests/helpers with
the machinery they protected.

Do not add compatibility aliases for removed command/query surfaces. `analyze parameter-id`
is the read-only planner; it is never a project apply path. Convention repair stays inside
`ghidra sync` (`prototype_repair` collect/apply), not as a standalone command.

## Commands and output

Prefer existing composed commands when they answer the task. New public CLI commands need a repeated,
durable workflow that cannot be expressed cleanly through an existing command or a small library call.
Exploratory scripts are disposable under `build/` or stdin Python.

Filter before printing. Large listings and diagnostic detail belong in named `build/` artifacts; the
command result should expose the useful bounded answer/path. Do not dump whole internal graphs merely
because the caller is an agent. `ghidra decompile` / `asm` / `sym` / `class` print compact text by
default; `--json` serializes that same result rather than running a second path.

Use `wiz8 report compare` to inspect saved comparisons by address, source file, outcome or
`--against` an earlier summary. It never builds or starts Ghidra. Comparison runs retain separate
directories and publish a latest pointer; keep the returned run path in task notes rather than
copying full snapshots. reccmp owns completed-result reuse and invalidation, not the project wrapper.

Return counts, bounded findings and artifact paths. Full comparison details stay in run reports;
`pr-check` saves its detailed lane results under build/reports/pr-check.json. Do not add a generic
query language, another report inventory or separate human/machine output modes.

For builds/analysis, use a 30-second initial tool yield and 30–60-second follow-up waits. Batch
independent reads while a process runs. Avoid one-second polling and model round trips that return
no new evidence. Keep user progress updates timely; do not solve polling with a repository scheduler.

Reuse successful checks while their relevant inputs agree. A continuation or compaction does not
invalidate a result or require rereading unchanged loaded skills. Keep a short handoff with instruction
paths, revision, inputs and result paths; reread when the task or instructions actually change.

## Verification

`wiz8 check` / `pr-check` run unit and repository tests excluding `integration`. Real VC6
link/trap checks run explicitly with `uv run pytest tests/unit/test_runtime_stubs.py -m integration`
and in runtime CI; live Ghidra tests belong in `tests/ghidra`, which has its own CI lane.
Keep external-tool integration coverage there rather than duplicating it in the fast suite.
Check runs type checking alongside indexing, then tests alongside read-only validators after the
completed index is available. All lane failures still propagate; successful gates are not omitted.

Use the smallest checks that exercise the changed owner:

- Python helper/CLI logic: focused unit tests plus ruff/pyright for the touched area.
- validation/source-index changes: the focused validator tests and `uv run wiz8 check` when its shared
  execution path changed.
- CMake/clang/build graph changes: configure/build or lint target that uses the changed graph.
- runtime/debug plumbing: relevant runtime scenario or debugger transport test.
- docs/skill-only changes: inspect the diff.

Do not run broad product builds merely because tooling changed if a narrower existing check exercises
the same path. Reuse a successful result until a relevant input changes.
