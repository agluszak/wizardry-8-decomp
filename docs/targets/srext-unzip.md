# `srEXT_Unzip.dll` target

The ZIP extension is byte-identical in all five materialized variants. Its SHA-256 is
`dba6cdc5741ae57574e019ab8f37935362bc288bee738bc711460f34d7647f62`; the file is 102,400
bytes, uses image base `0x10000000`, and has PE timestamp `2000-09-17T13:57:44Z`.

The version resources identify `UNZIP32.DLL`, file and product version 5.4, with the descriptions
`Info-ZIP's UnZip DLL for Win32` and `Info-ZIP's UnZip Windows DLL`. LINK 6.0, MSVCRT imports, and
the dominant Rich records for products 10 and 11 at build 8447 identify the surrounding VC6-era
toolchain. Unlike the JPEG extension, this binary has no CodeView/PDB record.

## Binary interface

The DLL exports only:

| Address | Ordinal | Name |
| --- | ---: | --- |
| `0x10011630` | 1 | `srGetLibraryVersion` |
| `0x10011190` | 2 | `srInitPlugin` |

`srGetLibraryVersion` is recovered in owned source and returns `0x012A0209`. `srInitPlugin`
allocates an eight-byte wrapper and a `0x24`-byte ZIP opener, then registers the `.zip` stream type
through `srIStreamOpener::addStreamType`.

The module imports 114 functions: 41 from `MSVCRT.DLL`, 32 from `KERNEL32.DLL`, 25 from `SR.DLL`,
14 from `ADVAPI32.DLL`, and two from `USER32.DLL`.

## Function ownership

The reviewed inventory accounts for 176 addresses: 157 functions discovered by Ghidra plus 19
entries recovered directly from vtables, callback tables, and disassembly.

| Range | Addresses | Interpretation |
| --- | ---: | --- |
| `0x10001000`-`0x100106AF` | 118 | Info-ZIP UnZip 5.4 |
| `0x100106B0`-`0x10011635` | 31 | Sir-Tech SurRender stream adapter and plugin |
| `0x10011640` onward | 27 | import, exception, CRT, and DLL-startup glue |

Source-built FID evidence identifies 100 base Info-ZIP functions and three VC6 CRT functions. Every
matching row for each of these 103 targets agrees on exactly one source symbol. The Info-ZIP rows
come from eight builds: RTM, SP5, SP5 Processor Pack, and SP6 compilers, each using both the upstream
`/MT /O2` recipe and the target-derived `/MD /O2` recipe. The target function name is unanimous even
where the producing recipe is not, so the tracked inventory retains the full matching-variant set
instead of promoting one compiler hypothesis.

No competing FID names were found. Source order, call graph, callback references, and linked-object
evidence resolve the remaining bodies, including the hidden WinDLL callbacks, `DllMain`,
`Wiz_NoPrinting`, the exact Sir-Tech `srWizUnzipToMemory` adaptation, and six encryption functions
from the public-domain zcrypt 2.8 overlay. The complete address map and per-function evidence live in
`config/analysis/functions/srext-unzip.csv`; the accepted public-library identities used by
reccmp live in `config/analysis/reccmp/srext-unzip.csv`.

## Repaired Ghidra model

The imported Ghidra program originally omitted 19 real entries, including:

- `0x10011200` and `0x10011210`, which return the opener and plugin descriptions;
- the stream-adapter destructor and adjustment thunks at `0x10011310`-`0x100113B0`;
- the Info-ZIP DLL callback table at `0x100115C0`-`0x100115F0`;
- `DllMessagePrint`, `UzpPassword`, `DummySound`, and `Wiz_StatReportCB` at
  `0x1000D910`-`0x1000DA01`;
- import thunks at `0x10011640` and `0x10011646`.

`uv run wiz8 ghidra apply-functions ...` now creates those entries and applies all 176 reviewed
exact, high-confidence, and structurally strong identities. The program contains the complete
176-address census with no candidate-only rows. `uv run wiz8 ghidra
apply-unzip-model` installs the object and callback structures, applies the callback prototypes,
and types the local tables.

The opener vtable at `0x10015054` contains the destructor at `0x10011220`, the substantial open
method at `0x100106B0`, and the description method at `0x10011200`. The wrapper vtable at
`0x10015060` contains the destructor at `0x10011280` and description method at `0x10011210`.

The owned memory-stream implementation has two local vtables. The five-slot `srBinStream` table at
`0x10015030` contains the deleting destructor, `getSize`, the one-argument `seek`, the directional
`seek`, and `tell`. The two-slot `srBinIStream` table at `0x10015044` contains `vget` and `vread`.
Its vbtable at `0x1001504C` places the virtual `srBinStream` base at `+0x1C`. The complete allocation
is `0x2C` bytes, and the local owner pointer at `+0x14` is passed to `free` by the destructor at
`0x10011350`.

All 22 imported SR functions already carry their exact demangled parameter types and calling
conventions. The other three SR imports are the global objects `srCore`, `srHeap`, and `srConfig`.

## Recovered adapter behavior

The first reviewed functions establish a narrow adapter rather than a second ZIP implementation:

- `0x10011060` reads `ZIP_CASE_INSENSITIVE`, calls the five-argument Info-ZIP memory-extraction
  wrapper at `0x1000DA10`, rejects multiple callback matches, and wraps the returned buffer in the
  owned `srBinIMStream` subclass;
- `0x1000DA10` is the target's lightly adapted `Wiz_UnzipToMemory` wrapper: it constructs Info-ZIP
  globals, initializes the `0x30`-byte callback record, sets redirect-to-memory and case-sensitivity
  state, delegates to the FID-confirmed upstream `unzipToMemory` at `0x10001010`, destroys globals,
  and frees a failed result buffer;
- `0x10010EB0` initializes the opener state and its six callback entries; `0x10011020` destroys the
  callback record and its small-string allocation;
- `0x100106B0` implements `ZIP_PATH` search, while `0x10010A60` splits the virtual `.zip` path and
  archive member before entering the memory-extraction path.

The recovered SurRender declarations now include `srConfig`, `srStringTable`,
`srIStreamOpener::Opener`, and the virtual-base `srBinIMStream` hierarchy. These declarations come
from exact decorated `SR.DLL` imports and the three local tables, not from guessed SDK headers.

The owned `src/srext_unzip/infozip_adapter.c` now implements the five-argument wrapper. VC6 SP5
produces the same 142-byte body as `0x1000DA10`, instruction for instruction, including the exact
`ret 0x14`; the linked comparison is now 100% after resolving the upstream call target.

## Reconstructed DLL status

The `SREXT_UNZIP` CMake target now links the pinned 19-file Info-ZIP source set, the exact
five-argument wrapper, and the first typed SurRender plug-in slice into `srEXT_Unzip.dll`. It emits
a PDB and exports the original interface (`srGetLibraryVersion` ordinal 1 and `srInitPlugin`
ordinal 2). `just build SREXT_UNZIP` is the canonical build and `just compare SREXT_UNZIP` selects
the corresponding reccmp target.

The comparison covers 153 code identities and all four local vtables. Every compared identity is
implemented, with 96.02% aggregate accuracy; 103 code functions already occupy their original
addresses. The remaining 23 entries in the 176-address census are explicitly classified rather
than omitted: five linker import thunks are consumed by reccmp's import resolution, and eighteen
source-generated EH cleanup funclets have no public PDB symbols. Their parent functions, cleanup
objects, stack offsets, and conditional-state bits are recorded in the function map.

The six zcrypt bodies, both descriptions,
`srGetLibraryVersion`, the adapter constructor, callback table, small-string constructors and
assignment, and many upstream Info-ZIP functions are exact. The source-defined Info-ZIP `DllMain` is now present;
that requires omitting `UNZIPLIB` on the real DLL target while retaining it on diagnostic library
targets. The separately compiled `srWizUnzipToMemory` body at `0x1000DA10` remains an
instruction-for-instruction match.

The recovered stream hierarchy is now enforced by compile-time sizes: `srBinStream` is `0x10`,
the adapter state is `0x20`, the complete opener is `0x24`, and the owned memory stream is `0x2c`.
The concrete subclass adds the `malloc` owner at `+0x14`; VC6 naturally emits the vbtable,
deleting destructor, adjustment thunk, and virtual `srBinStream` subobject at `+0x1c`. The
intermediate `srBinIStream` and `srBinIMStream` declarations are `novtable` client interfaces:
`SR.DLL` proves that both destructor bodies are empty, while `srBinStream::~srBinStream` only
restores the imported base vtable. The vtable symbols are now paired, so the opener, wrapper, and
owned-stream destructors, deleting destructors, and virtual-base adjustors all compare at 100%.
Dedicated vtable comparison reports all four local vtables as exact, and raw-data comparison reports
the owned-stream vbtable as exact. This also exposed and corrected the ABI order of the two `seek`
overloads. Decorated imports further correct `srBinIStream::vget` to return `unsigned short` and
establish `srBinIMStream::vread` as a private virtual.

The ZIP-path parser is now recovered in owned source. `srZipOpener::open` accepts direct paths that
contain `.zip`; otherwise it reads `ZIP_PATH`, splits it on semicolons, trims spaces from each
prefix, and tries the requested member beneath each prefix. `openArchivePath` records an optional
prefix before `@`, separates the `.zip` archive name from the member after its path separator, and
passes both to the typed adapter. This is substantive behavior rather than a placeholder, although
its temporary-object shape still needs matching refinement.

The callback ABI is exact. The password callback appends the `srZipAdapter*` after Info-ZIP's four
standard arguments (it is the fifth parameter), while the print/service, replace, and message
callbacks are instruction-exact no-op bridges.

The target uses narrowed source equivalents of Info-ZIP's `api.c` and `windll.c`: the original DLL
retains only the five memory-extraction API helpers and eight WinDLL setup/callback functions, while
the stock archive also contains unused version, validation, extraction, grep, and general-purpose
entry points. This source partition removes the stock-only code and its imports without changing the
upstream helper bodies.

The generated image reproduces the original exports by name and ordinal and every imported symbol:
25 from `SR.DLL`, 32 from KERNEL32, 41 from MSVCRT, 14 from ADVAPI32, and two from USER32. Its
`.rsrc` section has the same 840-byte virtual size, 4096-byte raw size, and complete version-resource
dictionary as the original. Both images are `0x19000` bytes in memory and have identical raw section
sizes. Remaining section drift is small: `.text` is 928 virtual bytes larger, `.rdata` 48 bytes
larger, `.data` 32 bytes smaller, and `.reloc` 50 bytes smaller. The 39-byte file-size excess is the
recompiled image's CodeView/PDB record.

## Runtime replacement proof

The rebuilt DLL was installed only in a reflinked copy of the materialized GOG tree and loaded with
Wine 9.0 in a dedicated 32-bit prefix. A VC6 harness initializes the original `sr.dll`, loads either
the original or rebuilt extension by explicit path, calls `srInitPlugin`, obtains the opener from the
recovered eight-byte plugin layout, and invokes the typed opener and stream interfaces. It destroys
every returned stream through its virtual destructor, deletes the plugin, unloads the DLL, calls
`srExit`, and unloads `sr.dll`.

The test archive contains a 29-byte `folder/member.txt` and a 22-byte `Case.TXT`. Original and
rebuilt extensions produce the same results:

| Scenario | Result |
| --- | --- |
| direct `tmp\\runtime.zip/folder/member.txt` | 29 bytes, FNV-1a `368f826b` |
| `ZIP_PATH=tmp\\runtime.zip/` plus `folder/member.txt` | same 29-byte member |
| exact `Case.TXT` | 22 bytes, FNV-1a `0b3e85e4` |
| lowercase `case.txt`, case-insensitive disabled | null stream |
| lowercase `case.txt`, case-insensitive enabled | exact 22-byte member |
| `evidence@tmp\\runtime.zip/folder/member.txt` | exact 29-byte member |
| missing archive | null stream |
| corrupt `.zip` file | null stream |
| wildcard matching both members | null stream, confirming multi-match rejection |

Each successful path was opened, read, and destroyed three times in one plug-in lifetime. No
exception, heap diagnostic, or Wine error occurred in either original or rebuilt runs; Wine emitted
only its generic read-access warning for the host `Z:` drive. This exercises registration, direct
and configured lookup, case policy, member splitting, callback-count rejection, extracted-buffer
ownership, repeated use, and clean unload against the original engine.

Normal game startup does not request `srEXT_Unzip.dll` in this installation, so the executable's
20-second smoke run is not claimed as extension coverage. The explicit harness is the relevant
drop-in test: it uses the unmodified game `sr.dll` and the extension's real exported factory and
SurRender vtables.

## Source archive provenance

The pinned archive has SHA-256
`b8c5a17798cd44050042fc1538df4102509d932dcab49eff3061f3065d00b45b`, contains 311 files, and
expands to 3,393,134 bytes. Its own `README` calls it the 28 November 1998 public Info-ZIP release
and names `unzip540.zip` as the portable 5.4 source distribution; `version.h` independently defines
major 5 and minor 4.

The exact archive currently comes from the configured historical Infania mirror. The official
[Info-ZIP SourceForge archive](https://sourceforge.net/projects/infozip/files/UnZip%205.x%20and%20earlier/)
currently exposes only 5.51 and 5.52 in that category, so there is no official-hosted 5.4 byte hash
available there for an independent equality check. This is a provenance limitation on the archive
container, not a version-identification gap: the target version resources, archive contents, and 100
unanimous target function matches all independently support UnZip 5.4.

The base UnZip 5.4 archive intentionally ships a dummy `crypt.c`. The target's six non-dummy
encryption bodies come from the public-domain zcrypt 2.8 overlay. The pinned overlay archive has
SHA-256 `fb02bc8d818da7c27ca02bf0d51e9c51e49ddce5262356b87d15f32295c4a6b3`; only `crypt.c` and
`crypt.h` are overlaid, and all six resulting encryption functions compare exactly. The source
fetch manifest records both the base archive and overlay hashes.

The root CMake project also exposes `INFOZIP_UNZIP_5_4`, using the exact 19-source VC6 static-library
set from the archive's `windll/visualc/lib` project. `just build INFOZIP_UNZIP_5_4` compiles and
archives that source unchanged with the target-derived `/MD /O2 /Zp4` configuration. The complete
`srEXT_Unzip.dll` target links the same ordered computational objects, replacing only the two broad
front-end objects with their evidenced retained subsets, the owned SurRender adapter, and the small
`Wiz_UnzipToMemory` adaptation described above.
