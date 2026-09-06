---
name: matching-decomp
description: Recover Wizardry 8 C++ bodies and declarations against the pinned VC6 target; use for source-model changes and interpreting focused comparison mismatches.
---

# Matching decompilation

Use existing source ownership and current compact context. Do not fetch the same evidence again unless
it is missing, stale, or contradictory. Inferred signatures and generated bodies can be wrong;
retail instructions and call sites decide.

Start with `just context ADDRESS...` for bounded source ownership, declarations, immediate
dependencies, and unresolved facts. It writes substantial decompiled code to the artifact path in
each function record. Reuse the batch until its evidence changes. Use the supported direct Ghidra
data, instruction, or rooted-flow query only for a concrete unanswered question.

`just recover ADDRESS...` generates persistent source-aware candidate C++ without editing the source
tree, building, or comparing. Inspect the candidate artifacts and blockers, then integrate a
connected batch with ordinary editing tools. Uncertain placement blocks insertion, not generation.

Recover ordinary C++ using canonical declarations. Make one coherent source-model change, then run
focused comparison with `just compare ADDRESS...` or `just compare --changed`; comparison establishes
incremental build freshness itself. Include unchanged callers when a header or ABI change affects
them.

A comparison mismatch is not automatically a source defect. Interpret the result:

- `exact` or `effective`: stop investigating the body unless the task requires more.
- `mismatch`: inspect the first structural divergence. Test a concrete behavior, type, ABI, lifetime,
  or source-ownership hypothesis only when supported by evidence, not just a different branch shape.
- `inconclusive`: identify the missing evidence; do not assume a source defect.

Equivalent compiler lowering is not a source correction. Preserve the ordinary source forms required
by `AGENTS.md`; do not experiment with equivalent loop, scope, or return spellings to chase a score.
When no evidence-backed correction is available, stop source-shape experiments and report the
remaining mismatch. This does not establish equivalence or waive the task's acceptance criteria:
classify a difference as codegen-only or relocation/classifier noise only with supporting evidence.

Revert demonstrated regressions. Expand the comparison set only when shared layout, lifecycle,
virtual dispatch, or inline visibility can affect other functions. Do not tune scratch registers or
compiler scheduling. Validation and publication policy lives in `AGENTS.md`.

Read [mismatch patterns](references/mismatch-patterns.md) for an unexplained comparison divergence or
[layout evidence](references/layout-evidence.md) when proving fields, widths, object size, or
inheritance layout. Do not load either for an ordinary exact result.
