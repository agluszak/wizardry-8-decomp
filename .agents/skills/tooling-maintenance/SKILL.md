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
- runtime staging/debugging use the existing shared primitives; do not duplicate product launch trees.

Delete a layer when upstream/native APIs already express the operation. Do not add wrappers merely to
rename an API, duplicate source inventories, parallel validation graphs, generic report frameworks,
string-command query protocols, replay databases, or separate human/JSON modes. A small typed helper is
worth keeping only when it contains project-specific analysis or removes repeated logic.

## Source index

`uv run wiz8 check` and selected `uv run wiz8 compare ...` refresh the compiler-backed index when they
need it. `uv run wiz8 analyze source-index` is for inspecting/debugging that projection, not a generic
preflight.

The project may adapt the lint compile database to the host/analysis-image boundary, but collection and
cache semantics belong to reccmp. Keep one native collection across configured link namespaces; do not
reintroduce per-target collectors, hand-maintained cache-input trees, another marker database, or a
parallel declaration-consistency model.

Paths that can run both locally and under Docker must be matched by repository-relative identity. Do
not bake `/repo` into clang-tidy filters or compile-database consumers. Environment-specific failures
should be fixed at the path/mount/compile-database boundary, not hidden with diagnostic suppression.

## Lint and validation

Keep the gating profile narrow and reconstruction-relevant. A diagnostic becomes gating when it finds
real source-model defects with an acceptable false-positive rate. Proven retail/vendor ABI behavior
wins over a generic lint heuristic; suppress such cases narrowly at the affected construct instead of
weakening the global profile.

Do not add tests by default. Add a test for a concrete correctness bug or stable public behavior that
existing checks missed. Avoid tests of source spelling, documentation text, inventory counts, generated
snapshots, deleted files, or implementation-private helper order. Delete obsolete tests/helpers with
the machinery they protected.

## Commands and output

Prefer existing composed commands when they answer the task. New public CLI commands need a repeated,
durable workflow that cannot be expressed cleanly through an existing command or a small library call.
Exploratory scripts are disposable under `build/` or stdin Python.

Filter before printing. Large listings and diagnostic detail belong in named `build/` artifacts; the
command result should expose the useful bounded answer/path. Do not dump whole internal graphs merely
because the caller is an agent.

## Verification

Use the smallest checks that exercise the changed owner:

- Python helper/CLI logic: focused unit tests plus ruff/pyright for the touched area.
- validation/source-index changes: the focused validator tests and `uv run wiz8 check` when its shared
  execution path changed.
- CMake/clang/build graph changes: configure/build or lint target that uses the changed graph.
- runtime/debug plumbing: relevant runtime scenario or debugger transport test.
- docs/skill-only changes: inspect the diff.

Do not run broad product builds merely because tooling changed if a narrower existing check exercises
the same path. Reuse a successful result until a relevant input changes.
