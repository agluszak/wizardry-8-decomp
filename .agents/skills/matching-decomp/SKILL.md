---
name: matching-decomp
description: Run systematic Wizardry 8 recovery campaigns over paired retail/recomp mismatches and recover faithful C++ at shared source-model owners.
---

# Recovery campaigns

Wizardry is past initial bring-up. Use this skill to reduce paired mismatches systematically, not to
chase one address at a time.

Before editing recovered C++, read [source fidelity](references/source-fidelity.md). Read
[comparison](references/comparison.md) for saved-report and comparison semantics,
[difference patterns](references/difference-patterns.md) for mismatch-family workflows, and
[source oracles](references/source-oracles.md) only when an accepted source boundary is relevant.

Use [type-modeling](../type-modeling/SKILL.md) for declarations/layout/class ABI,
[ghidra-analysis](../ghidra-analysis/SKILL.md) for live ProgramDB work,
[runtime-bringup](../runtime-bringup/SKILL.md) for behavioral validation, and
[tooling-maintenance](../tooling-maintenance/SKILL.md) when changing the pipeline itself.

## Campaign workflow

1. Start from a current saved comparison inventory. Use `wiz8 report compare` and report artifacts
   before running new analysis.
2. Select a coherent family: repeated call delta, field offset/width, ABI, global identity, literal,
   lifecycle/template pattern, floating-point behavior, predicate/control-flow shape, or a shared
   source/class owner.
3. Identify the canonical owner before editing callers. Search source and accepted oracles first.
4. Inspect only unanswered retail facts with batched `wiz8 ghidra decompile|asm|sym ADDRESS...`.
5. Recover the shared source model and update all affected consumers coherently.
6. Keep working through a substantial batch — normally ~100+ affected functions — before expensive
   validation. A focused comparison during investigation must answer a concrete uncertainty; it is not
   a ritual after every edit.
7. Rebuild/compare the batch once, inspect every regression and major gain, then fix the batch-level
   problems before final gates.

Prefer one source-model correction affecting fifty callers over fifty caller-specific edits. Prefer a
real recovery that leaves decompiler noise over a normalization that merely turns fifty rows green.

## What to prioritize

Small high-signal queues are worth exhausting: referenced-data findings, literal/address-only
differences, and type/cast-only differences frequently expose real model defects.

For large buckets, cluster first:

- **Calls:** canonical callee differences, missing/extra calls, helper expansion/inlining, receiver or
  argument differences.
- **Fields/layouts:** same receiver with repeated offset/width disagreements; fix the record once.
- **ABI/signatures:** argument count, convention, receiver storage, return width/type. Provenance must
  be independent; source-projected retail ProgramDB facts cannot confirm the source that projected them.
- **Lifecycle/templates:** constructors, destructors, deleting destructors, vtables, copy/assignment,
  exception cleanup and inline/template emission should be audited by class/template owner.
- **Floating point:** constant width, promotion, x87 rounding, accumulation precision, NaN behavior and
  expression grouping are source-level questions, not generic normalization opportunities.
- **Control flow:** only after higher-level families are removed, classify predicates, guards, bounds,
  loop form, store ordering and genuinely large structural differences.

Tags and clusters are triage aids, never equivalence claims.

## Stop conditions

Keep straightforward faithful source when no evidence-backed correction remains. Do not rewrite a
counted loop, add a temporary, force inline/noinline, duplicate cleanup or reorder expressions merely
to reduce Ghidriff output.

Marker/TU rules, ICF, template emission, compiler storage and cast/union constraints are owned by the
[source-fidelity reference](references/source-fidelity.md); do not restate or weaken them here.

## Batch validation

For recovery campaigns, tests are rare. Normally do not run broad checks until the batch has covered
~100+ affected functions or another comparably substantial systemic family.

Near completion:

- run the affected `uv run wiz8 compare --build --changed` once (use `--program sr.dll` for
  SurRender);
- account for new/materially changed authored functions;
- format manually owned C/C++ once;
- run `uv run wiz8 pr-check`;
- run `uv run wiz8 report merge-preservation --base origin/main`;
- run runtime scenarios only when the batch changes behavior they actually observe.

Record report paths and unresolved questions under `build/` for continuation. A continuation resumes
the batch; it does not trigger another preflight, inventory run or validation cycle.
