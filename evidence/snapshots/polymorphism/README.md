# Polymorphism-ABI snapshot

Every vtable, vtable slot and constructor vptr write in the first-party Wizardry executables whose
code is readable. Tracked because reproduction needs the proprietary binaries.

The producer is `wiz8decomp.polymorphism`. Normal runs write the same CSVs under
`build/reports/polymorphism/` and fail when they differ from this snapshot:

```sh
uv run wiz8 polymorphism                  # verify against the snapshot
uv run wiz8 polymorphism --update-snapshot
```

The scan is bounded by the image's own relocation table rather than by a byte pattern: a vtable is a
run of consecutive relocated slots pointing into code, and a value that is not a relocated slot
cannot belong to one.

`boundary` records why each table ended, which is the part that is easy to get wrong. Adjacent
tables of one class merge into a single run unless something splits them, so a run is also cut at
any address a constructor writes as a vptr (`vptr-write-boundary`). Without that rule a table
absorbs the next one and reports the sum of both slot counts.

`vptr-writes.csv` is the class-identity channel: `object_offset` is where in the object the pointer
is stored, so offset `0` marks a primary table and any other offset marks the base subobject at that
offset. A table written at several offsets by several functions is normal - a constructor, a copy
constructor and a destructor each write it.

Slot `kind` is `pure-virtual` when the slot holds the `_purecall` thunk, resolved through the import
table by name rather than by a hardcoded address; `import-thunk` when the slot points at a jump
thunk, in which case `import_name` and `import_signature` name the library method the class
inherited; `adjustor-thunk` for the `sub ecx, N; jmp` entries that shift `this` onto a base, with the
adjustment and real target recorded; and `local` otherwise.

`kind` on a table is `vftable` or `vbtable`. A vbtable holds base displacements rather than
addresses, so its entries are never relocated and it is detected by shape.
