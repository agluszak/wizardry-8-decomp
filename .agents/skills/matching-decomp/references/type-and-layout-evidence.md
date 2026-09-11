# Type and layout evidence

Settle the canonical type across its uses, not one convenient declaration:

- Check all writers and consumers of a global/member, including load/store widths and extensions.
- Check argument storage and use at multiple call sites before fixing a prototype; inspect return
  production together with caller consumption. Pointer/integer disagreement needs particular care.
- Allocation size before construction bounds the most-derived object; stack-frame use bounds a
  by-value local. Neither alone assigns semantic field types.
- Signed/unsigned branches constrain comparisons after promotions. A byte operation establishes
  storage/operation width, not automatically C++ `bool`; distinguish width from semantic type.
- Base calls, vptr replacement/restoration, adjustor thunks, and normalized receivers establish
  subobject layout. Normalize each receiver to the complete-object root before interpreting offsets.
  Nearby vptr writes can describe an embedded polymorphic member rather than a second base.
- Assertions can suggest field names; their memory operands place fields. Preserve unknown spans.

Trace producer, consumer, caller, and callee evidence, then correct the owning header/definition and
Ghidra model. A `W8PList*` cast to a `W8IList*` API may reveal a wrong canonical API or prototype;
inspect both families before choosing the proper typed call. Do not use casts, opaque wrappers,
integer/pointer substitutions, or duplicate declarations to hide disagreement.

For evidence-backed Ghidra corrections, use [native transactions](pyghidra.md#edit-established-facts).
If the evidence changes a class boundary/inheritance/subobjects, use
[class-triage](../../class-triage/SKILL.md); ordinary member-type fixes do not require identity triage.

## Byte-sized logical types

`unsigned char`, SGP `BOOLEAN` (`typedef unsigned char`), and C++ `bool` are one byte but different
source types. A typedef is erased, so the binary cannot distinguish `BOOLEAN` from `unsigned char`;
provenance and interface evidence decide. Reject Win32 `BOOL` for ordinary game state: it is 32-bit
and only belongs where a Windows API declaration requires it.

Decide each byte-valued parameter, return, and field by its role, not its width, name, or observed
0/1 values:

- Genuine SGP results and interfaces keep SGP vocabulary: `FileRead`/`FileWrite` and functions that
  propagate their `BOOLEAN` result unchanged.
- Wizardry/SurRender C++ logical state, predicates, and logical arguments use native `bool` when the
  uses are truth tests and all return paths produce canonical 0/1. A comparison result, a masked
  single low bit, or a literal `1`/`0` is already canonical and may compile identically as either
  type; prefer `bool` for C++ state unless SGP provenance says `BOOLEAN`.
- Bit masks, enums, small counts, state codes, subcycle indices, capacities, and serialized bytes
  keep a byte type or gain a proper enum. `flag_` naming and adjacent booleans are not evidence.
- A byte returned as a non-canonical value (for example `flags & 2`, which yields 0 or 2) is not a
  C++ `bool`: changing the return type would force normalization retail does not contain.

Changing a declared type changes caller argument conversion even when the parameter itself is not
read differently, so settle fields and virtual families as one bundle: all overrides of a virtual
predicate/setter take the same type, and byte locals that feed a newly logical parameter usually
become `bool` in the same change. Validate with focused comparison (the canonical return/store
sequence, not the score) and `uv run wiz8 vtable CLASS` for the affected hierarchy.

## Clang consistency checks

Clang is a consistency detector, not binary evidence.

```sh
uv run wiz8 lint
uv run wiz8 diagnostics
```

`lint` is the structural clang-cl compile lane: use it for incompatible declarations, conversions,
overrides, and related source-model problems. `diagnostics` emits additional non-gating recovery
diagnostics. Use them when those questions matter, not for every exact body. Retail instructions,
call sites, and accepted source decide which side is wrong; do not silence diagnostics with
`reinterpret_cast`. Validate the affected ABI bundle when declaration/layout changes reach callers.
