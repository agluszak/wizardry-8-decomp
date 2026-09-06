---
name: matching-decomp
description: Recover Wizardry 8 C++ bodies and declarations against the pinned VC6 target; use for source-model changes and interpreting focused comparison mismatches.
---

# Matching decompilation

Use existing source ownership and current compact context. Do not fetch the same evidence again unless
it is missing, stale, or contradictory. Inferred signatures and generated bodies can be wrong;
retail instructions and call sites decide.

Start with the narrow human view that answers the question:

- `just context ADDRESS... --view summary --no-match` for ownership and signature conflicts;
- `just context ADDRESS... --view code --no-match` for decompiled bodies;
- `just context ADDRESS... --view dependencies --no-match` for caller/callee boundaries;
- add `--listing --view listing` only when instructions are needed.

Use full context or `--deep` only when these views omit evidence needed for a concrete question. Reuse
a batch packet until its evidence changes. Automatic recovery is optional; an exporter decline does
not block manual recovery from retail evidence.

Recover ordinary C++ using canonical declarations. Make one coherent source-model change, then run
focused comparison with `just compare ADDRESS...` or `just compare --changed`. Use `--no-build` if
that source state was already built. Include unchanged callers when a header or ABI change affects
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
