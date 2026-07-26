# Global-variable snapshot

Every global the code refers to in the first-party Wizardry executables whose code is readable,
found through the relocation table rather than by pattern. Tracked because reproduction needs the
proprietary binaries.

The producer is `wiz8decomp.data_globals`. Normal runs write the same CSV under
`build/reports/globals/` and fail when it differs from this snapshot:

```sh
uv run wiz8 globals                  # verify against the snapshot
uv run wiz8 globals --update-snapshot
```

An absolute address appears in code only because the loader fixes it up, so the relocation table
enumerates global references exactly. A reference the table does not list is not a global reference.

`storage` separates `initialized` from `bss`. Most of the game's mutable state lives in the
uninitialised tail of `.data`, past the bytes the file actually stores; those are ordinary globals
with no initialiser, not unmapped addresses.

`widths` lists every operand size observed at the address, so a single consistent width is the
variable's size and several widths mean either a union, a structure addressed at its first field, or
an array indexed by different types. `access_kinds` separates `read` and `write` from
`address-taken`; a global that is only ever address-taken is an array or a structure rather than a
scalar.

`extent_upper` is the next referenced global in the same section and `extent_bytes` the distance to
it. That bounds the variable's size from above - nothing more, since an unreferenced neighbour leaves
the bound loose.

`kind` is `import-slot` for an import-table entry, whose reference count measures calls to a library
function rather than use of a game global; `string` for a literal starting exactly at the address,
which requires the preceding byte to terminate a string so that printable bytes inside a float
constant are not mistaken for text; `code-pointer` when the first word points into code; and `data`
otherwise. `preview` carries the first characters of a string, or the imported symbol name.

The full per-reference list is a generated report under `build/reports/globals/references.csv`
rather than a tracked artifact: it is an order of magnitude larger, and it is derived from this same
producer run.
