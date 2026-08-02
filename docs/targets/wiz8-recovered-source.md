# Wiz8.exe recovered source

The executable recovery target contains source-owned identities and compiler-enforced declarations.
Current identities and ownership coverage are generated from the source index, Ghidra and reccmp by:

```sh
just wiz8 report status
```

Owned definitions are split by original translation unit where ownership is established. The
The recovered translation units compile once in an internal object library and are reused unchanged
by the comparison, runtime and semantic-test products. The object library is a CMake implementation
detail, not a separate supported product or a claim that the functions shared one source file.

Build and compare with:

```sh
just build WIZ8
just compare WIZ8
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

## Matching lessons worth retaining

### Control-flow factoring can determine register allocation

`GetLocationIDFromCode` converged only when the lookup loop was represented as a separate inline
helper returning `-1`. A flat loop introduced an extra bound block; a `goto` form allowed VC6 to
delete guards retained in the original. The factored helper preserved those apparently redundant
guards and naturally recovered the original register roles.

The two empty-string tests likewise had to be one short-circuit expression so both failures shared
one return epilogue. These are source-structure conclusions, not arbitrary compiler appeasement.

This case is the main reason structurally strong bodies should not be forced exact while they still
live in artificial units: register allocation may be a symptom of missing original factoring or
inlining context.

### Loop peeling is a shared unresolved pattern

Several PList-backed searches agree on typed calls, field offsets, bounds, and branch semantics but
emit a peeled first iteration rather than the original's single reused loop body. They stay
`structurally-strong` in the canonical comparison data. The shared mismatch should be revisited at
the owning translation-unit level rather than patched independently in each function.

### Raw disassembly outranks a bad decompiler signature

Ghidra initially assigned extra parameters to `PListIndexOf` and
`GetOriginOfCharacterItem`. Walking the raw `ESP`-relative reads against caller push counts proved
the correct signatures. For recovered functions the C++ declaration now owns that ABI; Ghidra keeps
the analysis signature for bodies that do not yet have owned source.

This is a useful boundary rule: decompiler pseudocode is an observation, not reviewed ABI evidence.

### Equivalent expressions can produce different canonical code

`AddLinesToMessageBox` required preserving comparison operand order and delaying the old-array load
until the grow branch. The alternatives were semantically equivalent but changed VC6's `cmp`
direction and register lifetime. Once those source-order constraints were restored, the body became
exact without casts or fake control flow.

### Type width controls branch signedness

Multiple recoveries became exact only after evidence-backed unsigned types reproduced `JB`/`JBE`
rather than signed branches. This includes party eligibility fields, PList counts, and shared item
pool bounds. Signedness must come from canonical consumers and assertions, not from the decompiler's
default type.

## What belongs elsewhere

- Recovered identity and signature: the address-marked C++ declaration.
- Identity provenance and atomic supporting facts: `evidence/reviewed/wiz8/claims.csv`.
- Class relationships, fields, virtuals, and layout ownership: C++ declarations with compiler gates.
- Native layouts and fields: the canonical Ghidra project, checked by focused read-only Java audits.
- Current pairing and exact/effective status: live reccmp results under `build/`.
- Current totals, ownership, and source-unit coverage:
  `build/reports/status.md`.

Do not copy those inventories back into this document when another function lands. Add prose only
when the recovery establishes a durable compiler, ABI, ownership, or reverse-engineering lesson.
