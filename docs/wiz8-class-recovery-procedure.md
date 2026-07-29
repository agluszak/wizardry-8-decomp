# Wizardry 8 class recovery

Ghidra is the operational owner of class layouts. Structures, fields, inheritance observations,
vtables, constructors, destructors, function signatures, labels, and comments are reviewed and
edited in the canonical project. The evidence ledger records why an identity or layout claim is
accepted; it is not a second Data Type Manager.

## Start from one product transition

Choose a handler reached by the current menu flow, identify its screen class and original
translation unit, and recover only the immediate call graph needed to make that transition work.

```sh
uv run wiz8 ghidra restore
just context 0x<handler-address>
```

The joined context includes source ownership, current match results, provenance, strings and
assertions, callers/callees, decompilation, and relevant fields. Use Ghidra itself for ordinary
listing, cross-reference, data-type, and vtable inspection.

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

Use Ghidra undo/versioning, a cloned project, or a temporary GZF for speculative changes. Do not
create overlay plans or candidate databases. Refresh the tracked checkpoint only after review:

```sh
uv run wiz8 ghidra index
uv run wiz8 ghidra seed refresh wiz8
```

The normalized export under `build/ghidra-index/` is disposable and reviewable. The tracked GZF is
the canonical analysis checkpoint, while provenance remains the explanation of accepted claims.

## Acceptance

For a recovered function, require the narrowest applicable proof:

```sh
just build WIZ8
just wiz8 verify-boundaries
just compare
just test
```

Relocation-masked boundary verification is the exact-body authority. Runtime behavior is the
product authority. Linked-image reccmp output is a diagnostic and pairing surface.
