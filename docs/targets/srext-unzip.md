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

The reviewed inventory accounts for 172 addresses: 157 functions discovered by Ghidra plus 15
entries recovered directly from vtables, callback tables, and disassembly.

| Range | Addresses | Interpretation |
| --- | ---: | --- |
| `0x10001000`-`0x100106AF` | 114 | Info-ZIP UnZip 5.4 |
| `0x100106B0`-`0x10011635` | 31 | Sir-Tech SurRender stream adapter and plugin |
| `0x10011640` onward | 27 | import, exception, CRT, and DLL-startup glue |

Source-built FID evidence identifies 100 Info-ZIP functions and three VC6 CRT functions. Every
matching row for each of these 103 targets agrees on exactly one source symbol. The Info-ZIP rows
come from eight builds: RTM, SP5, SP5 Processor Pack, and SP6 compilers, each using both the upstream
`/MT /O2` recipe and the target-derived `/MD /O2` recipe. The target function name is unanimous even
where the producing recipe is not, so the tracked inventory retains the full matching-variant set
instead of promoting one compiler hypothesis.

No competing FID names were found. The remaining 14 functions in the contiguous Info-ZIP region
stay unnamed source-library candidates rather than receiving proximity-based names. The complete
address map and per-function variant evidence live in
`config/analysis/functions/srext-unzip.csv`.

## Missing function entries

The current Ghidra function list omits 15 real entries, including:

- `0x10011200` and `0x10011210`, which return the opener and plugin descriptions;
- the stream-adapter destructor and adjustment thunks at `0x10011310`-`0x100113B0`;
- the Info-ZIP DLL callback table at `0x100115C0`-`0x100115F0`;
- import thunks at `0x10011640` and `0x10011646`.

The opener vtable at `0x10015054` contains the destructor at `0x10011220`, the substantial open
method at `0x100106B0`, and the description method at `0x10011200`. The wrapper vtable at
`0x10015060` contains the destructor at `0x10011280` and description method at `0x10011210`.

The third local table is the owned memory-stream implementation at `0x10015030`. Its seven entries
are the local deleting destructor followed by imported `srBinIMStream` `getSize`, two `seek`
overloads, `tell`, `srBinIStream::vget`, and `srBinIMStream::vread`. Its vbtable at `0x1001504C`
places the virtual `srBinStream` base at `+0x1C`. The complete allocation is `0x2C` bytes, and the
local owner pointer at `+0x14` is passed to `free` by the destructor at `0x10011350`.

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

The root CMake project also exposes `INFOZIP_UNZIP_5_4`, using the exact 19-source VC6 static-library
set from the archive's `windll/visualc/lib` project. `just build INFOZIP_UNZIP_5_4` compiles and
archives that source unchanged with the target-derived `/MD /O2 /Zp4` configuration. The eventual
`srEXT_Unzip.dll` target will link this upstream target with only the owned SurRender adapter and the
small `Wiz_UnzipToMemory` adaptation described above.
