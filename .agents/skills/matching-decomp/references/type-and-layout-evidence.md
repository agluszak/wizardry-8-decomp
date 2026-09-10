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

## Clang consistency checks

Clang is a consistency detector, not binary evidence.

```sh
just wiz8 lint
just wiz8 diagnostics
```

`lint` is the structural clang-cl compile lane: use it for incompatible declarations, conversions,
overrides, and related source-model problems. `diagnostics` emits additional non-gating recovery
diagnostics. Use them when those questions matter, not for every exact body. Retail instructions,
call sites, and accepted source decide which side is wrong; do not silence diagnostics with
`reinterpret_cast`. Validate the affected ABI bundle when declaration/layout changes reach callers.
