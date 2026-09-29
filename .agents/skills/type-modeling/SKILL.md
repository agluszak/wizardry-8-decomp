---
name: type-modeling
description: Recover Wizardry 8 C++ types, prototypes, fields, globals, layouts, inheritance, vtables and lifecycle ABI across whole consumer families.
---

# Type modeling

Use this skill when a mismatch points to the canonical C++/ABI model rather than one function's source
spelling.

Detailed source-fidelity rules live in
[matching-decomp/source fidelity](../matching-decomp/references/source-fidelity.md). Use
[ghidra-analysis](../ghidra-analysis/SKILL.md) for live ProgramDB facts and
[matching-decomp](../matching-decomp/SKILL.md) for the recovery campaign.

## Recover the owner, not the use site

When one mismatch suggests a type/layout defect:

1. Inventory all relevant producers and consumers before editing.
2. Group accesses by receiver identity, offset, width, read/write and call role.
3. Reconcile conflicting evidence at the owning declaration/layout.
4. Update all affected consumers coherently.
5. Validate the affected family only after the substantial batch is complete.

A repeated field-offset or width mismatch across many functions is one model problem.

## Scalar and ABI evidence

Use multiple observations:

- load/store width, extension and truncation;
- argument storage at callers and callee consumption;
- return production and caller use;
- signed/unsigned branch behavior after promotions;
- stack/by-value extent and allocation size;
- serialized/external storage versus in-memory semantic type;
- virtual overrides and every declaration of the same external symbol.

A byte operation establishes width, not automatically C++ `bool`. `unsigned char`, SGP
`BOOLEAN`, C++ `bool` and Win32 `BOOL` are distinct source types.

Do not treat source-projected retail ProgramDB signatures as independent confirmation of current source.

## Fields and overlapping storage

Machine-width copies are compiler/aggregate evidence, not automatic source fields. Before adding a
wider member, overlay or union, trace the complete source/destination extent and test whether an
existing embedded record or ordinary assignment explains it.

A union requires positive source-level overlap evidence such as a discriminant or mutually exclusive
lifetime/state. Coincident offsets, equal widths and decompiler disagreement are insufficient.

Allocation size bounds a most-derived object but does not name its fields. Preserve unknown spans.

## Classes and lifecycle

Test existing bases, embedded members and canonical templates before creating a new class boundary.
For hierarchy/subobject work read
[inheritance evidence](references/inheritance-evidence.md). For `srClassSupport`, clone and
compiler-emitted lifecycle families read [template emission](references/template-emission.md).

Audit hierarchy changes as a family: constructors, ordinary/desleting destructors, copy/assignment,
vtable slots, adjustor thunks and affected receivers.

Compiler-generated deleting destructors are marker-only `SYNTHETIC`; do not hand-write them.

## External ABI and packing

Preserve proven calling conventions, packing and vendor interface shapes. Consumer/provider declarations
may differ only when independently established ABI evidence requires it; do not use that as an excuse
for ordinary source disagreement.

Clang is a consistency detector, not retail evidence. Use diagnostics to find model contradictions,
then decide from retail/source evidence.

## Validation

Type/layout work belongs in the same rare batch cadence as recovery work. Use focused inspection while
investigating, then compare/layout/vtable checks for the coherent family after roughly ~100 affected
functions or another substantial model batch. Broad lint/pr-check belongs near completion, not after
individual declaration edits.
