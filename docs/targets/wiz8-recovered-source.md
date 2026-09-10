# Wiz8.exe recovered source

The executable recovery target contains source-owned identities and compiler-enforced declarations.
Current identities and ownership coverage are generated from the source index, Ghidra and reccmp by:

```sh
just wiz8 report status
```

Owned definitions are split by original translation unit where ownership is established.
The recovered translation units compile once in an internal object library and are reused unchanged
by the comparison, runtime and semantic-test products. The object library is a CMake implementation
detail, not a separate supported product or a claim that the functions shared one source file.

Compare a selected recovered function (builds current inputs itself):

```sh
just compare 0x0044e010
```

The common first-party profile is the pinned VC6 SP5 `/O2 /G6 /MD` configuration. Compiler options
are changed only against aggregate translation-unit evidence, not to explain an isolated mismatch.
The source-backed SGP `Random.c` unit retains its separately proven project profile.

## Original translation-unit ownership

Assertion paths provide bounded translation-unit intervals without pretending to establish exact
object boundaries:

```sh
just wiz8 report translation-units
```

The command writes `build/reports/translation-units/translation-unit-intervals.csv` and
`gameplay-translation-units.csv`. A function is `direct` when its own assertion names the source,
`inferred` when its start lies within one non-overlapping assertion interval, and `gap` otherwise.
Gap functions remain unowned until more evidence arrives.

## Historical comparison observations

Earlier `GetLocationIDFromCode` and `AddLinesToMessageBox` experiments changed guards, register use,
operand order, and load placement through source factoring or expression changes. Those are compiler
observations, not independent proof of authored helpers, short-circuit spelling, or expression order.
The operational rules now live in [matching-decomp](../../.agents/skills/matching-decomp/SKILL.md) and
its [mismatch reference](../../.agents/skills/matching-decomp/references/mismatch-patterns.md).

PList-backed searches also showed loop peeling despite agreement on typed calls, field offsets,
bounds, and branch semantics. Historical `structurally-strong` observations are not current comparison
results and do not expand the requested task. Investigate TU context only when it supplies an
evidence-backed hypothesis for the selected functions.

Call-site evidence corrected extra inferred parameters on `PListIndexOf` and `GetOriginOfCharacterItem`:
raw ESP-relative reads and caller push counts constrained their signatures. C++ owns the recovered
declarations; Ghidra retains live analysis signatures for recovered and unrecovered bodies alike.
Correct demonstrated disagreements at both owners, using the
[type and layout reference](../../.agents/skills/matching-decomp/references/type-and-layout-evidence.md).

## What belongs elsewhere

- Recovered identity and signature: the address-marked C++ declaration.
- Identity provenance and atomic supporting facts: `evidence/reviewed/wiz8/claims.csv`.
- Class relationships, fields, virtuals, and layout ownership: C++ declarations with compiler gates.
- Native layouts and fields: the canonical Ghidra project, inspected through native PyGhidra.
- Current pairing and exact/effective status: live reccmp results under `build/`.
- Current source/matching statistics: `just wiz8 report status`; translation-unit attribution:
  `just wiz8 report translation-units`.

Do not copy those inventories back into this document when another function lands. Add prose only
when the recovery establishes a durable compiler, ABI, ownership, or reverse-engineering lesson.
