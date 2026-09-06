---
name: class-triage
description: Decide whether an unnamed Wizardry 8 constructor, destructor, vtable, or registry family is a real class before declaring it.
---

# Class triage

Use this skill when proposing a new class boundary or revising an existing hierarchy. Do not rerun
identity triage merely to implement another method of an established class.

First test whether the canonical base or template explains the evidence. A distinct vtable,
lifecycle body, deleting destructor, or registry family does not alone prove authored source.
Declare only the additional boundary independently supported by storage, behavior, or source identity.

Read [template emission](references/template-emission.md) for `srClassSupport`, registry, clone, or
deleting-destructor evidence. Read [inheritance evidence](references/inheritance-evidence.md) when
receivers, subobject placement, or a hierarchy changes. Validate the affected lifecycle/vtable bundle
when the model changes. Record unresolved identity concisely instead of inventing a wrapper.
