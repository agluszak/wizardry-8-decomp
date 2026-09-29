---
name: matching-decomp
description: Recover Wizardry 8 C++ bodies and source placement; triage and audit focused or batched retail/recomp discrepancies.
---

# Matching decompilation

Recover ordinary circa-2000 C++ from retail and accepted source oracles. AGENTS.md owns evidence,
source fidelity and acceptance policy; this skill owns the recovery workflow.

Before editing recovered C++, read [source fidelity](references/source-fidelity.md). For prototypes,
fields, layouts, inheritance, vtables or lifecycle ABI use [type-modeling](../type-modeling/SKILL.md).
For live ProgramDB edits use [ghidra-analysis](../ghidra-analysis/SKILL.md); native decompile/asm/sym
reads alone do not require loading that skill. For tooling use
[tooling-maintenance](../tooling-maintenance/SKILL.md), runtime behavior
[runtime-bringup](../runtime-bringup/SKILL.md), and history/publication
[jujutsu-workflow](../jujutsu-workflow/SKILL.md).

Read [comparison](references/comparison.md) for report selection/inspection and outcome meanings,
[difference patterns](references/difference-patterns.md) for unexplained diffs, or
[source oracles](references/source-oracles.md) for SGP/MSVC/zlib/IJG/Info-ZIP source.
Load only references needed for the current question; retain loaded instruction paths in the handoff.

## Recovery

1. Identify the entity's canonical source/header/TU owner. Search source and accepted oracles first;
   reuse reviewed evidence unless missing, stale or contradictory.
2. Inspect only unanswered retail facts using `wiz8 ghidra decompile|asm|sym ADDRESS...`. Batch related
   addresses in one call. SurRender addresses require `--program sr.dll`; WIZ8 is the default.
   Uncertain placement blocks insertion, not investigation. Reads never compile or synchronize.
3. Establish parameter contracts from callers and uses; audit touched types, abstractions, lifetime,
   pointer identity and visibility before writing a nontrivial body. Allocation -> insertion/removal ->
   destruction is one family. Machine-width accesses may be aggregate-copy lowering.
4. Correct established ProgramDB facts at their owner; project source facts with `wiz8 ghidra sync`
   when necessary. Missing source metadata does not block native reads.
5. Reconstruct the affected source model and its dependents coherently. Then compare the affected
   bundle. An intermediate comparison should resolve a concrete uncertainty, not follow every edit.
6. Investigate logical differences through source/type/ABI/ownership/TU facts. Keep faithful source
   when no evidence-backed correction remains. Large unstable diffs need focused retail CFG/call/branch
   review or the relevant runtime observation, not attempts to sculpt decompiler text.

Comments record non-obvious evidence, established retail oddities and unresolved facts. Avoid intent
claims or narrating obvious control flow; remove stale provenance after renames and TU moves.

## Discrepancy campaigns

Start from a current saved comparison inventory. Filter before reading details; prioritize concrete
branch, call, constant, field and referenced-data changes, grouping candidates by shared owner or ABI.
A full run is useful once for triage, not before every candidate. Explained representation/lowering
and already-matched bodies need revisiting only when new evidence or relevant inputs change.

Keep one mutable change across a substantial coherent group. User batch-size preferences determine
publication cadence; do not publish a few isolated edits by default or invent fixes to fill a quota.
Complete source corrections first, then validate a coherent group. Compilation, layout/vtable checks
and focused comparisons may be needed during recovery when they resolve actual uncertainty.

Keep concise task-local notes under build/: revision, source owners/addresses, conclusions and evidence
paths, unresolved questions, report paths and successful verification inputs. These notes are a handoff,
not another symbol database or an authority above retail/source. A continuation resumes this batch;
it is not a new preflight, inventory run, skill reload or publication cycle.

## Final audit and formatting

After the final source edit, run `uv run wiz8 compare --build --changed` for affected products
(SurRender: `--program sr.dll`). If products/index are already current, omit --build; do not pre-run
check/source-index merely to prepare this explicit build-and-compare command. Account for every new
or materially changed FUNCTION: no differences, explained representation/established lowering, or an
unstable diff with retail CFG/call/branch review. Do not publish unaccounted bodies.

Format changed manually owned C/C++ with `uv run clang-format --style=file -i PATH...`, then
`uv run clang-format --style=file --dry-run --Werror --fail-on-incomplete-format PATH...`.
Do not format imported/vendor SGP source. Formatting alone needs no new build/comparison.

Successful validation remains useful while its relevant inputs agree. Source/headers/ABI/build flags
can invalidate products and comparisons; retail/analysis/catalog/tool changes invalidate affected
analysis results. Record what ran and against which inputs. A rebase requires rechecking affected
inputs and marker preservation, not automatically replaying every successful lane.

## Marker binding and placement

FUNCTION binds immediately to its following declaration/definition; pragmas and unrelated comments
belong above the marker. TEMPLATE is followed by its emitted-symbol comment, with behavior at the
primary template owner. SYNTHETIC is followed by its compiler-identity comment; LIBRARY is address-only.
SYNTHETIC/LIBRARY own no handwritten implementation. GLOBAL belongs at its canonical definition.

Preserve TU ownership/order in src/wiz8/sources.cmake or src/surrender/CMakeLists.txt. Compiler-emission
TUs contain provenance only. Do not create a .cpp merely to park markers; unknown ownership remains
an unresolved fragment. An ordinary independently emitted destructor is FUNCTION; deleting helpers
are SYNTHETIC. Read the fidelity reference for template, ICF and header-visibility evidence rules.
