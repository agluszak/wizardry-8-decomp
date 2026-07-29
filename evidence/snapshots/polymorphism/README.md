# Polymorphism-ABI snapshot

Every vtable, vtable slot and decoded vptr write in the first-party Wizardry executables whose
code is readable. Tracked because reproduction needs the proprietary binaries.

The producer is `wiz8decomp.polymorphism`. Normal runs write the same CSVs under
`build/reports/polymorphism/` and fail when they differ from this snapshot:

```sh
uv run wiz8 evidence refresh polymorphism                  # verify against the snapshot
uv run wiz8 evidence refresh polymorphism --update-snapshot
```

The scan is bounded by the image's own relocation table rather than by a byte pattern: a vtable is a
run of consecutive relocated slots pointing into code, and a value that is not a relocated slot
cannot belong to one.

`boundary` records why each table ended, which is the part that is easy to get wrong. Adjacent
tables merge into a single run unless something splits them, so a run is cut at any table address
referenced from code (`code-reference-boundary`). This remains valid even when the corresponding
constructor store cannot be decoded. Without that rule a table absorbs the next one and reports
the sum of both slot counts.

`vptr-writes.csv` records `store_displacement`, the raw displacement in the decoded memory operand.
It is not a root-relative object offset unless a separate receiver-provenance analysis establishes
the register's relation to the complete object. Zero and nonzero displacements therefore do not by
themselves classify primary tables, embedded polymorphic members, or secondary bases. A table
written at several displacements by several functions is normal.

`allocation_size` on a zero-displacement write is the size pushed to `operator new` just before it.
It is an allocation hint until receiver provenance associates the write with that allocation. It is
read back from the store rather than
from a call to a constructor, so an inlined construction has one too. It is empty when the object is
embedded, stack-placed, or - as the srMaterial builders do - allocated through a register holding the
allocator, where no size is visible at the site at all.

Slot `kind` is `pure-virtual` when the slot holds the `_purecall` thunk, resolved through the import
table by name rather than by a hardcoded address; `import-thunk` when the slot points at a jump
thunk, in which case `import_name` and `import_signature` name the library method the class
inherited; `adjustor-thunk` for the `sub ecx, N; jmp` entries that shift `this` onto a base, with the
adjustment and real target recorded; and `local` otherwise.

`kind` on a table is `vftable` or `vbtable`. A vbtable holds base displacements rather than
addresses, so its entries are never relocated and it is detected by shape.
