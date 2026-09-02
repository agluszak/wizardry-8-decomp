---
name: class-triage
description: Decide whether an unnamed Wizardry 8 constructor, destructor, vtable, or registry family is a real class before declaring it.
---

# Class triage

1. Start with `just context <selector>` and identify source ownership, storage, and lifecycle evidence.
2. Check ordinary base/template emission before proposing a new class or override.
3. Require independent evidence for a class boundary; vtables, deleting destructors, and template emissions alone are insufficient.
4. Recover normal C++ ownership and ABI shapes, then validate the complete constructor/destructor/vtable bundle.
5. Record unresolved identity and negative experiments in the active Bead instead of inventing wrappers.

Open `references/full-guide.md` only for the matching evidence pattern at hand (template emission,
inheritance, layout, or deleting-destructor ABI).
