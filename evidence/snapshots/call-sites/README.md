# Diagnostic call-site snapshot

Literal arguments recovered from `srAssertFail` and `srRuntimeClass::setName` call sites in every
first-party Wizardry executable whose code section is readable. Tracked because reproduction needs
the proprietary binaries.

The producer is `wiz8decomp.call_sites`. Normal runs write the same CSVs under
`build/reports/call-sites/` and fail when they differ from this snapshot:

```sh
uv run wiz8 call-sites                  # verify against the snapshot
uv run wiz8 call-sites --update-snapshot
```

Recovery is static. Arguments are pushed immediately before the call, so a short instruction window
that decodes to end exactly on the call yields them; no decompiler and no Ghidra project is
involved, which is why every build is covered rather than only the canonical one.

`call_kind` records how the import was reached: `direct` for `call dword ptr [slot]`, `thunk` for a
relative call into a linker jump thunk, and `register-indirect` for the sites that load the slot
into a register first - the last of which a byte search for the call encoding cannot see.

`function_start` is derived, not read: it is the first byte after the nearest preceding inter-
function padding run, accepted only when that byte begins a recognised prologue. It is blank when no
candidate qualified. It is not a substitute for a reviewed function identity.

`assertions.csv` keeps the message argument as well as the expression. Messages frequently name the
owning class in prose where the expression only names a member.

These rows are observations across all builds and do not replace
`evidence/observations/wiz8/assertions.csv`, which is the reviewed canonical-retail table carrying
`containing_function` from Ghidra.
