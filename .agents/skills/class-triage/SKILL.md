---
name: class-triage
description: Evaluate evidence for a new Wizardry 8 class boundary or a change to an existing hierarchy; not for implementing another method of an established class.
---

# Class triage

Use this skill when proposing a new class boundary or revising an existing hierarchy. Do not rerun
identity triage merely to implement another method of an established class.

Inspect and edit native Ghidra objects through [direct PyGhidra](../matching-decomp/references/pyghidra.md).
Search the actual data-type manager before declaring a type; missing fields in a wrapper report do
not establish that the type is absent. Existing lifecycle and field-flow algorithms are optional
helpers, not required command paths or a substitute for binary evidence.

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
when the model changes. Record unresolved identity concisely instead of inventing a wrapper.
