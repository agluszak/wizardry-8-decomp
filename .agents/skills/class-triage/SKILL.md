---
name: class-triage
description: Evaluate new Wizardry 8 class boundaries, inheritance/subobject changes, or whether lifecycle/vtable families are distinct authored classes; not ordinary method recovery.
---

# Class triage

Use this skill for a proposed class boundary, inheritance/subobject change, or uncertain identity
across lifecycle/vtable families. Do not rerun identity triage for ordinary methods of an established class.

Inspect and edit native Ghidra objects through [direct PyGhidra](../matching-decomp/references/pyghidra.md).
For field widths, prototypes, receiver normalization, or Clang consistency checks, read
[type and layout evidence](../matching-decomp/references/type-and-layout-evidence.md) as needed.

First test whether the canonical base or template explains the evidence. A distinct vtable,
lifecycle body, deleting destructor, or registry family does not alone prove authored source.
Declare only the additional boundary independently supported by storage, behavior, or source identity.

Scalar and vector deleting destructors are MSVC ABI glue, never authored functions or evidence for a
new class boundary. Keep the wrapper address as a marker-only `SYNTHETIC` identity. Recover a separate
ordinary destructor only when retail emits it independently; otherwise use the declaration, inline
form, or inherited virtual destructor required by the evidenced hierarchy.

Read [template emission](references/template-emission.md) for `srClassSupport`, registry, clone, or
deleting-destructor evidence. Read [inheritance evidence](references/inheritance-evidence.md) when
receivers, subobject placement, or a hierarchy changes. Validate the affected lifecycle/vtable bundle
when the model changes, using `uv run wiz8 compare ADDRESS...` and `uv run wiz8 vtable CLASS`.
Record unresolved identity concisely; prefer an address-qualified name to unsupported semantics.
