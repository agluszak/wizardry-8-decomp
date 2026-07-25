# Function ID corpus

The project-owned FID databases are reproducibly rebuildable through the shared `wiz8` CLI. Ghidra's
packed database bytes include internal identity, so the semantic library/function counts in the
adjacent JSON manifest are the reproducibility check rather than byte-identical `.fidb` output. Proprietary
compiler files and extracted COFF objects remain under `WIZ8_WORK_DIR`; only pinned metadata and
the resulting FID database are tracked.

## Rebuild

```sh
uv run wiz8 ghidra fid inventory
just ghidra fid build-image
just ghidra fid build-seeds
just ghidra fid extract-libraries
just ghidra fid build
uv run wiz8 ghidra fid match --program wiz8--gog-base--wiz8--18a74ff61c65
```

`build-image` clones only the exact archaic-msvc commits declared in
`config/static-libraries.yml`. The compiler-capable RTM, SP5, Processor Pack, and SP6 images build
the pinned zlib 1.0.4 and IJG JPEG 6 source seeds. SP3 and SP4 are deliberately marked
`precompiled-libraries` because their repositories contain headers and libraries but no runnable
compiler. `extract-libraries` copies the pinned `libcmt.lib` and `libcpmt.lib` archives from the
candidate images, verifies their SHA-256 values, and deterministically extracts ordinary i386 COFF
members. Byte-identical archives are recorded but seeded only once.

## Current evidence

- Wizardry first-party modules report LINK 6.0 and import `MSVCRT.dll` and `MSVCP60.dll`.
- The canonical executable's Rich header contains product/build records 10/8447, 11/8447,
  11/8168, and 48/9044. This confirms a mixed VC6-era link, not one exact compiler version.
- The SP3 snapshot identifies itself as product 6.00.8447 with `c1xx.dll` 12.00.8168. The SP4
  snapshot reports compiler 12.00.8804 and linker 6.00.8447. Neither Git tree contains the
  corresponding compiler executable, so source-built SP3/SP4 optimizer claims remain blocked.
- zlib 1.0.4 is confirmed in the canonical executable by its embedded version strings. The tested
  compiler/flag matrix does not reproduce its large functions, including `inflate`; that miss is
  consistent with the unavailable 8447-era optimizer and is not evidence against the library ID.
- IJG JPEG release 6 is confirmed in `srEXT_JPEGImporter.dll`. Source-built `/O2 /MD` seeds match
  160 target functions across the tested complete VC6 compiler candidates.
- Info-ZIP UnZip 5.4 is confirmed by the file and product versions in `srEXT_Unzip.dll`. The
  pinned 5.4 source tree builds 19 project compilation units in both its upstream `/MT /O2` and
  target-runtime `/MD /O2` variants. Those seeds identify 100 target functions. All 100 have
  multiple matching build rows, but every row for a target agrees on the same source symbol; the
  remaining ambiguity is the compiler/flag provenance, not competing function names.
- The UnZip target imports `MSVCRT.dll` and has dominant Rich product/build records 10/8447 and
  11/8447. The source's Visual C++ project predates that target toolchain, so `/MD /O2` is retained
  as a target-derived variant alongside the unmodified project recipe instead of replacing it.
- Exact SP3/SP4 CRT archives add high-confidence names including `__alldiv`, `__alloca_probe`,
  `__ArrayUnwind`, vector construction/destruction helpers, and `__DllMainCRTStartup@12`.
  Corresponding SP3 and SP4 function bodies are identical for these matches, so they do not
  distinguish the service pack.
- `libcpmt.lib` has two byte-identical groups: RTM/SP3/SP4 and SP5/Processor Pack/SP6. Each observed
  `libcmt.lib` service-pack snapshot is distinct; the Processor Pack copy is identical to SP5.
- COFF-local names such as `$L264` vary between snapshots and are excluded from query evidence;
  they remain in the FID graph so parent/child scoring is not weakened.

### Rejected match: `Wiz8.exe` `0x004146E0`

FID offers `jzero_far` for `0x004146E0` at score 14.68. The 27-byte body is byte-identical to the
IJG function, but it is only a generic zero-fill loop. Its eight callers belong to Sir-Tech code:
the dominant callers reference `C:\Projects\SGP\DirectDraw Calls.c`, and another references
`C:\Projects\Wizardry 8\Engine Code\Video2.cpp`. The main executable has no neighboring IJG
corpus, while its `JPEGImporter` string refers to the separate plugin architecture. This match is
therefore rejected as library-ownership evidence and must not transfer the IJG name into the main
program.

The MSVC static archives are support-code oracles. Their presence in the FID corpus does not imply
that Wizardry was linked with `/MT`; the observed first-party binaries use the DLL runtimes.
