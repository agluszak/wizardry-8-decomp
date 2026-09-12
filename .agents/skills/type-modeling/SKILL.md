---
name: type-modeling
description: Recover and correct Wizardry 8 C++ types, prototypes, globals, fields, enums, layouts, class boundaries, inheritance, vtables, and ABI-facing declarations.
---

# Type modeling

Use this skill when the question is about the canonical C++/ABI model rather than one function body's
source spelling: parameter/return types, globals, fields, enums, bools, packing, object boundaries,
inheritance/subobjects, vtables, lifecycle families, or conflicting declarations.

Use [ghidra-analysis](../ghidra-analysis/SKILL.md) for live binary inspection/edits and
[matching-decomp](../matching-decomp/SKILL.md) for recovering/comparing a function body.

## Settle one canonical type

Trace the type across all relevant producers and consumers before editing:

- load/store widths, extensions and truncations;
- argument storage at multiple call sites and callee use;
- return production together with caller consumption;
- allocation size and stack/by-value extents;
- signed/unsigned branch behavior after integer promotions;
- serialized/external ABI storage versus in-memory semantic type;
- virtual overrides and every declaration of the same external symbol.

Correct the owning header/definition and the reviewed Ghidra model when the evidence establishes the
fact. Do not hide disagreement with casts, integer/pointer substitution, opaque wrappers, duplicate
externs, aliases, or a second local declaration.

Allocation size bounds the most-derived object but does not name fields. A byte operation establishes
width, not automatically C++ `bool`. Assertions may suggest names; their memory operands establish
placement. Preserve unknown spans rather than filling them with convenient invented structure.

## Object and class boundaries

First test whether an existing object, base, embedded member or generic/template definition already
explains the evidence. Adjacency, shared initialization, repeated offsets or a convenient access
pattern do not prove one aggregate. Require allocation/lifetime/subobject evidence before combining
independent globals or splitting an evidenced object.

A distinct vtable, deleting destructor, registry family or lifecycle body alone also does not prove an
authored class. Conversely, absence of known derived fields does not disprove a boundary: an apparently
empty derived class can be real when its own vtable identity is corroborated by construction,
destruction, receivers, registration or other lifecycle evidence.

For hierarchy/subobject changes read [inheritance evidence](references/inheritance-evidence.md). For
`srClassSupport`, clone, registry and compiler-emitted lifecycle families read
[template emission](references/template-emission.md).

Scalar/vector deleting destructors are MSVC ABI glue. Keep their retail identity as marker-only
`SYNTHETIC`; never hand-write the hidden flags parameter or a destruct-and-maybe-free helper. Recover an
ordinary destructor separately only when retail emits a standalone body.

## Byte-sized logical types

`unsigned char`, SGP `BOOLEAN` and C++ `bool` are all one byte but are not interchangeable source types.
Use provenance and behavior, not width alone:

- real SGP interfaces/results keep `BOOLEAN`;
- Wizardry/SurRender C++ predicates, logical state and logical arguments use `bool` when every producer
  is canonical 0/1 and consumers are truth tests;
- masks, enums, state codes, counts, serialized bytes and non-canonical values remain byte/integer
  storage or gain a proper enum;
- Win32 `BOOL` is 32-bit and belongs only where the external Windows ABI requires it.

A return such as `flags & 2` is not `bool`: changing the declared return type would normalize 0/2 into
0/1 and change behavior. When one member of a virtual family changes type, reconcile the complete
override family and the callers feeding it.

## Packing and external ABI

Retail/source ABI evidence outranks a lint warning. Preserve proven packing, calling conventions and
vendor interface shapes. If clang diagnoses a legitimate external construct, use the narrowest local
suppression needed; do not remove `#pragma pack`, invent an adapter/thunk, add C fallback APIs, or
weaken the global lint profile merely to silence it.

## Consistency checks

Clang is a consistency detector, not binary evidence:

```sh
uv run wiz8 lint
uv run wiz8 diagnostics
```

`lint` catches incompatible declarations, conversions, overrides and narrow reconstruction errors;
`diagnostics` is the broader non-gating lane. Cross-TU external declaration consistency is owned by
the compiler-backed source-index writer, which `uv run wiz8 check` runs and selected `compare`
refreshes. Do not pre-run `uv run wiz8 analyze source-index` unless debugging that projection itself.

A redundant same-type cast means the canonical types already agree: remove it. Do not dismiss a gating
diagnostic as pre-existing. Retail instructions, call sites and accepted source decide which model is
correct when two declarations disagree.

Validate changed declarations as an ABI bundle: focused comparisons for affected functions/callers,
`uv run wiz8 vtable CLASS` for hierarchy changes, and the relevant layout/compile gates. Do not expand
an ordinary member-type correction into class-identity triage unless the boundary itself changes.
