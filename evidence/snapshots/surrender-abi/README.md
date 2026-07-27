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

`vftable-slots.csv` and `vbtable-entries.csv` read the tables `exports.csv` names. A vftable export
is a data address, so its slots are read out of the module rather than disassembled: each slot holds
an absolute address the loader fixes up, which puts it in the relocation directory, and each points
into an executable section. The first address failing either test, or the start of another exported
symbol, ends the run. `resolution` is `exported` when the slot's target is itself an exported
symbol, whose name and signature then fill `target_name` and `target_signature`, and `internal`
when it is a method the library does not export - recorded as unresolved rather than guessed.

A vbtable holds displacements rather than addresses, so none of its entries is relocated and the
first relocated slot ends that run instead. Entry zero is the offset from the vbptr back to the
vbtable; the rest locate each virtual base within the object, which is what a derived declaration
needs in order to inherit virtually and still match.

`subobject` on either table is the base a secondary table belongs to, empty for a primary.

Modules with byte-identical payloads across variants are recorded once, under the canonical
variant; `wiz8 surrender-abi` reports the aliases it collapsed.
