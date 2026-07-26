# SurRender export-ABI snapshot

Every exported symbol of every SurRender module in the corpus, with its decorated name decoded
into structural facts. Tracked because reproduction needs the proprietary binaries.

The producer is `wiz8decomp.surrender_abi`. Normal runs write the same CSV under
`build/reports/surrender-abi/` and fail when it differs from this snapshot:

```sh
uv run wiz8 surrender-abi                  # verify against the snapshot
uv run wiz8 surrender-abi --update-snapshot
```

This is declaration evidence about linked library code, not recovery of it: the rows come from
export tables, never from disassembling a SurRender body.

`kind` distinguishes constructors, destructors, vftables, vbtables and ordinary members.
`virtuality` separates `virtual` from `non-virtual` and `static`, and `adjustor_thunk` marks the
compiler-generated entries that exist only to shift `this` onto a secondary base - so the columns
together describe the polymorphism ABI without reading a single instruction.

`vftable_base` is the base subobject an exported vftable belongs to, taken from the demangled
signature rather than the decorated name, which uses back-references and unexpanded templates.
An empty value on a `vftable` row means the primary vftable; a non-empty value names the base, which
makes the inheritance edges of the library explicit.

`parse_status` is `ok` only when the whole structural prefix decoded. `template-scope` marks names
whose owning class is itself a template, where the scope chain is reported best-effort.

Modules with byte-identical payloads across variants are recorded once, under the canonical
variant; `wiz8 surrender-abi` reports the aliases it collapsed.
