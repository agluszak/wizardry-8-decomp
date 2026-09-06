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

Interpret the result:

- `exact` or `effective`: stop investigating the body unless the task requires more.
- `mismatch`: inspect the first structural divergence and test one source hypothesis.
- `inconclusive`: identify the missing evidence; do not assume a source defect.

Expand the comparison set only when shared layout, lifecycle, virtual dispatch, or inline visibility
can affect other functions. Do not tune scratch registers or compiler scheduling. Validation and
publication policy lives in `AGENTS.md`.

Read [mismatch patterns](references/mismatch-patterns.md) for a structural divergence or
[layout evidence](references/layout-evidence.md) when proving fields, widths, object size, or
inheritance layout. Do not load either for an ordinary exact result.
