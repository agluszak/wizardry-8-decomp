# MSVC exception-metadata snapshot

Generated observations of the C++ exception tables in every first-party Wizardry executable.
They are tracked because reproduction needs the proprietary binaries, which are never committed.
Each row records the program identity that produced it, so an unavailable build is visible as
missing rows rather than as a silent gap.

The producer is `wiz8decomp.eh_metadata`. Normal runs write the same CSVs under
`build/reports/eh-metadata/` and fail when they differ from this snapshot:

```sh
uv run wiz8 eh-metadata                  # verify against the snapshot
uv run wiz8 eh-metadata --update-snapshot
```

`functions.csv` has one row per `FuncInfo` record. `frame_setup` is the exact instruction that
pushes the record's handler thunk, and `eh_setup_start` is the preceding `push -1`. Both lie inside
the owning function, but neither is claimed to be its entry point: VC6 may install the EH frame
after an FS load, argument work, or an early-return region. A Ghidra consumer can resolve the
containing function from either address without turning the setup into a false boundary.
`unwind_signature` excludes every address, so the same source compiled into another build hashes
the same and the column can be joined across programs.

`unwind.csv` has one row per unwind state. `frame_offset` is the `ebp`-relative slot the cleanup
funclet addresses and `target` is the destructor it branches to, so a row places a typed local
object at a known offset. `kind` records how the object was reached: `object` for a direct `lea`,
`pointer` for an indirect load, `pushed-pointer`/`pushed-literal` for the `__cdecl` shapes.

`catch.csv` has one row per catch handler, including the `TypeDescriptor` address and its decorated
name where the build kept one. These are the only surviving MSVC type descriptors in the corpus.
