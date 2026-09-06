# Wizardry 8 class recovery

Ghidra is the operational owner of class layouts. Structures, fields, inheritance observations,
vtables, constructors, destructors, function signatures, labels, and comments are reviewed and
edited in the canonical project. The evidence ledger records why an identity or layout claim is
accepted; it is not a second Data Type Manager.

## Start from one product transition

Choose a handler reached by the current menu flow, identify its screen class and original
translation unit, and recover only the immediate call graph needed to make that transition work.

Use [direct PyGhidra](../.agents/skills/matching-decomp/references/pyghidra.md) for native function,
listing, reference, data-type, and vtable inspection and edits. The existing project opener handles
startup and restores the seed when needed; do not run restore as a routine prerequisite.
`just context ADDRESS...` is optional when its joined source/provenance view answers the question.
Do not add a custom query or edit command to expose an existing Ghidra operation.

## Establish a class

1. Find constructor/destructor writes of a shared vtable address.
2. Inspect all vtable slots and their receiver adjustments.
3. Group field accesses by offset and width. Preserve unknown spans rather than inventing fields.
4. Check exception metadata, source-oracle declarations, assertion paths, and cross-build evidence.
5. Create or refine the structure, pointers, signatures, namespaces, and applied types in Ghidra.
6. Record only the supporting claim in the provenance ledger when the conclusion needs an
   explanation outside Ghidra.
7. Recover the declaration or body in its proven source owner and validate it.

An address-qualified or positional name is preferable to an attractive but unsupported semantic
name. A vtable shape proves callable slots and receiver structure; it does not by itself prove
method names or inheritance semantics.

## Experiments and checkpoints

Use native transactions in the existing project; roll back speculative edits and save accepted
changes once per coherent batch. Do not clone a project, create overlay plans, or maintain candidate
databases for ordinary recovery. Resolve canonical data types by their actual paths and preserve
unresolved fields/types. Correct demonstrated ABI errors directly, then regenerate affected
caller/callee output instead of interpreting candidates built from a known-bad prototype.
Refresh the tracked checkpoint when sharing reviewed analysis, not after every edit:

```sh
uv run wiz8 ghidra seed refresh wiz8
```

Focused reports under `build/` are disposable. The tracked GZF is the canonical analysis checkpoint,
while provenance remains the explanation of accepted claims.

## Acceptance

Follow `AGENTS.md` for change-specific verification and `matching-decomp` for the focused comparison
loop.

Reccmp's current-binary relocation-masked result is the exact-body authority. Runtime behavior is
the product authority.
