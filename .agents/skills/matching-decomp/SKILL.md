---
name: matching-decomp
description: Recover Wizardry 8 C++ bodies and declarations against the pinned VC6 target; use for source-model changes and interpreting focused comparison mismatches.
---

# Matching decompilation

Use existing source ownership and current evidence. Do not fetch the same evidence again unless
it is missing, stale, or contradictory. Inferred signatures and generated bodies can be wrong;
retail instructions and call sites decide.

Use direct PyGhidra for the unanswered question: inspect native functions, parameter storage,
callers, instructions/P-code, references, or data types in one open-program session. The
[PyGhidra reference](references/pyghidra.md) has the bootstrap and edit recipe. Batch related work in
ordinary Python; do not discover or extend the custom query command catalogue first.

When call sites or incoming storage contradict a prototype, correct its established parts in
Ghidra with a native transaction, save the coherent edit, and regenerate affected caller/callee
output. Keep uncertain types unknown and preserve already-established types. A rooted-flow query
uses the current inferred prototype: it cannot discover an omitted parameter, and an empty access
list does not prove an argument is unused. Do not repeatedly query a known-bad model or ask for
separate permission to correct it within the recovery task.

Use `wiz8 report context ADDRESS...` only when its joined source/provenance view helps. `just recover
ADDRESS...` optionally generates source-aware candidate artifacts without editing source, building,
or comparing. Inspect useful candidates, then integrate a connected batch with ordinary editing
tools. Neither command is a prerequisite for direct analysis or editing. Uncertain placement blocks
insertion, not investigation or candidate generation.

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

A `mismatch` whose only difference is a `call_target` can be a linker artifact rather than a source
defect. Retail's linker folded identical helper bodies (ICF), while the comparison image deliberately
links `/OPT:NOICF`, so a source call by the correct name resolves to a different final address than
retail's folded call. Before changing source to match one, check whether the two symbols are a known
linker-equivalence class (for example `{PLLength, ILLength}`, retained at `0x005E2C70`) and verify the
contribution with reccmp's relocation-masked object comparison:

```sh
uv run reccmp-reccmp --target WIZ8 --object build/decomp/CMakeFiles/wiz8_recovered_objects.dir/src/wiz8/engine_code/Prop.cpp.obj \
    --symbol '?UpdateWorldProps0044E010@@YAXPAUW8World@@@Z' --orig-address 0x0044E010 --size 133
```

If that reports `exact`/`relocation-masked-object`, the body, ABI, and argument handling already match
and only the fold alias differs: keep the type-correct call. Never introduce a type-incorrect call,
such as casting a `W8PList*` to `W8IList*`, to reproduce an ICF-folded address.

Revert demonstrated regressions. Expand the comparison set only when shared layout, lifecycle,
virtual dispatch, or inline visibility can affect other functions. Do not tune scratch registers or
compiler scheduling. Validation and publication policy lives in `AGENTS.md`.

Read [mismatch patterns](references/mismatch-patterns.md) for an unexplained comparison divergence or
[layout evidence](references/layout-evidence.md) when proving fields, widths, object size, or
inheritance layout. Do not load either for an ordinary exact result.
