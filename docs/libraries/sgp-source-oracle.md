# Standard Gaming Platform source oracle

Wizardry 8 contains code from Sir-Tech's shared Standard Gaming Platform (SGP), and the released
Jagged Alliance 2 history preserves explicit Wizardry build branches. The authoritative oracle is
the `sgp/` tree at ja2-stracciatella commit
[`5ac0a9d56d27e8a7e2c4a7b48ed8932ae7f64033`](https://github.com/ja2-stracciatella/ja2-stracciatella/tree/5ac0a9d56d27e8a7e2c4a7b48ed8932ae7f64033/sgp),
the SGP-side `Initial Import` dated 2004-09-06. This is separate from the repository's JA2-side
initial-import root.

The source is covered by the Strategy First Source Code License Agreement. Its terms are not a
normal permissive open-source grant: they restrict use to non-commercial purposes and impose
conditions on redistribution and derivative works. This project is non-commercial and accepts
those conditions, so the pinned tree is vendored at `third_party/sfi-sgp/sgp` with the verbatim
license. Modified licensed files must carry the notices and dates required by the agreement. The
exact license artifact is `sgp/SFI Source Code license agreement.txt`, Git blob
`b66aabc6f7affb4fca0b6cc2d6288f3225ecd0b1`.

## Wizardry branches

Thirty-three translation units actively select `WIZ8 SGP ALL.H` when
`WIZ8_PRECOMPILED_HEADERS` is defined. `MemMan.c` retains the same branch only as commented-out
text. Eleven additional headers/project files contain Wizardry-specific types, feature exclusions,
or project entries. The complete reviewed census is tracked in
`config/analysis/sgp/source-oracle.yml`.

The missing product PCH cannot be reconstructed by renaming the JA2 PCH: the JA2 header pulls in
game-specific screens and utilities which are not SGP ABI. Each probed unit therefore gets its own
overlay directory under `config/sgp-overlays/`, containing only the includes and declarations its
own source proves necessary, plus the shared empty `common/builddefines.h`. For
`DirectDraw Calls.c` in particular:

* `WIZ8_PRECOMPILED_HEADERS` is defined;
* `JA2`, `JA2_PRECOMPILED_HEADERS`, `UTIL`, and `UTILS` are absent;
* the release build has no `_DEBUG`, making `MemAlloc` and `MemFree` reduce to CRT allocation;
* the product PCH exposes `gfDontUseDDBlits`;
* `builddefines.h` contributes no definition consumed by this translation unit.

This is deliberately a translation-unit model, not a claim that the full `WIZ8 SGP ALL.H` has
been recovered. `WizLibs.h` and other product headers remain missing.

## Binary path evidence

The GOG executable embeds `C:\Projects\SGP\DirectDraw Calls.c` at VA `0x0060003C`. The same exact
path occurs in the demo, base executable, 1.261 executable, and both 1.28 executable copies. The
2001-12-23 retail executable does not retain it. Per-file hashes and offsets are tracked in
`config/analysis/sgp/wiz8-source-paths.csv`; absence in retail is recorded rather than inferred as
absence of SGP code.

The path has 25 references in the canonical executable, all clustered in the assertion-bearing
DirectDraw wrapper region. It is therefore direct translation-unit ownership evidence, not a loose
library-family string.

## First compiled matches

The pristine `DirectDraw Calls.c` blob
`2bdc93c4baf998c914eca8fce8322c2f314e24fa` compiles with VC6 SP5 using
`/O2 /MD /DNDEBUG /DWIZ8_PRECOMPILED_HEADERS` and the narrow overlay. Masking only COFF relocation
fields produces 13 exact functions in the canonical executable:

```sh
just build WIZ8_SGP_DIRECTDRAW
```

The tracked provenance pins the original revision and tree. CMake compiles the vendored source
directly; generated COFF objects remain outside git under `WIZ8_WORK_DIR`.

| Address | Source identity | Size |
| --- | --- | ---: |
| `0x0040F0B0` | `DDCreateSurface` | 74 |
| `0x0040F100` | `DDLockSurface` | 79 |
| `0x0040F150` | `DDUnlockSurface` | 35 |
| `0x0040F180` | `DDGetSurfaceDescription` | 51 |
| `0x0040F1C0` | `DDReleaseSurface` | 79 |
| `0x0040F210` | `DDRestoreSurface` | 30 |
| `0x0040F230` | `DDBltFastSurface` | 84 |
| `0x0040F290` | `DDBltSurface` | 103 |
| `0x0040F300` | `DDCreatePalette` | 50 |
| `0x0040F340` | `DDSetPaletteEntries` | 50 |
| `0x0040F380` | `DDGetPaletteEntries` | 50 |
| `0x0040F3C0` | `DDReleasePalette` | 30 |
| `0x0040F3E0` | `DDSetSurfaceColorKey` | 40 |

The exact hashes are authoritative in `config/analysis/functions/wiz8-sgp.csv`. Other source
functions are not accepted merely because they share the address neighborhood: some were removed
as unreferenced COMDATs, and several larger Wizardry bodies differ from the released JA2-era
implementation.

All 13 bodies are also exact in the playable demo at addresses shifted by `+0x360`. They remain at
the canonical addresses in the base executable and both files materialized for 1.261; the 1.28
tree's base executable is likewise exact. The packed/rewritten `Wiz8_v128.exe` and protected retail
executable do not contain the same relocation-masked byte sequences, so no names are transferred to
them from this byte matcher. The complete per-build observations are the `directdraw` rows in
`config/analysis/sgp/harness.csv`.

The oracle also resolves the independently reconstructed zlib boundary. The five exact functions
at `0x00415820` through `0x004158F0` are the retained decompression half of `sgp/Compression.c`,
with original names `ZAlloc`, `ZFree`, `DecompressInit`, `Decompress`, and `DecompressFini`.

### `Compression.c`: retained decompression half

The per-unit sweep compiles all nine functions emitted by `Compression.c` under the common
`/O2 /Ob2 /G5 /MD` project profile. `ZAlloc`, `ZFree`, `DecompressInit`, and `Decompress` each have one unique
relocation-masked candidate in all five comparable executables. `CompressedBufferSize`,
`CompressInit`, and `Compress` have no exact, relocation-equivalent, or near candidate in any of
those builds.

`CompressFini` and `DecompressFini` require one more piece of evidence: after relocation masking
their 23-byte bodies are identical, so the generic harness correctly reports both as
`ambiguous-generic`. The sole canonical candidate at `0x004158F0` calls `0x00415960`, the separately
source-identified `inflateEnd`, not `deflateEnd`. It is therefore `DecompressFini`; with no second
candidate, `CompressFini` was stripped along with the other three compression-only COMDATs.

The retained functions preserve source order from lines 14, 19, 24, 55, and 80 at addresses
`0x00415820` through `0x004158F0`. The next function, zlib's `inflateReset`, starts at `0x00415910`.
This supports the linker-order hypothesis that `Compression.obj` immediately preceded the selected
zlib objects, with the four later source COMDATs eliminated rather than moved elsewhere. The full
machine observations are the `compression` rows in `config/analysis/sgp/harness.csv`; the reviewed
source-line and object-order resolution is in `config/analysis/sgp/reviewed-findings.csv`.

## `Random.c`: a complete unit, and a name correction

`Random.c` is the first translation unit recovered in full. Its own header settles the build
configuration: `PRERANDOM_GENERATOR` is gated behind `JA2`, which the Wizardry branch does not
define, so the unit compiles to exactly three functions. The header also states outright that
Wizardry uses the subsystem — *"Wizardry can use it too, but I'm saving them a K in the meantime"*.

```sh
just build WIZ8_SGP_RANDOM
```

All three bodies are relocation-masked exact, and each matches at exactly one address per build:

| Address | Source identity | Size | Source line |
| --- | --- | ---: | ---: |
| `0x0040EF80` | `InitializeRandom` | 19 | 23 |
| `0x0040EFA0` | `Random` | 50 | 39 |
| `0x0040EFE0` | `Chance` | 51 | 54 |

Two project-profile flags are proven by this unit, not incidental:

* `/G5` is required. Under `/G6` only `InitializeRandom` — which contains no arithmetic — still
  matches; both `Random` and `Chance` diverge.
* `/Ob2` is required. Wizardry inlined `Random` into `Chance`, constant-folding the range to 100.
  Plain `/O2` implies `/Ob1` and will not inline a function that is not marked `inline`, so under
  `/O2` alone `Chance` emits an out-of-line call and never matches.

`Chance` is a good illustration of why per-unit flag sweeps matter: at `/O2 /G5` the unit looks like
two exact functions and one absent one, which invites the wrong conclusion that Wizardry modified or
dropped `Chance`. It did neither.

The complete address matrix is represented by the `random` rows in
`config/analysis/sgp/harness.csv`. The whole unit sits in
the demo at the same `+0x360` shift as the DirectDraw block, is identical in the 1.261 and 1.28 base
executables, and is absent from the packed `Wiz8_v128.exe` and the protected retail executable —
recorded as unavailable rather than as absent.

## Reusable per-unit harness

The DirectDraw prototype is generalized as a declarative, per-translation-unit sweep in
`config/analysis/sgp/harness.yml`. Run all configured units, or select one, with:

```sh
uv run wiz8 sgp sweep
uv run wiz8 sgp sweep --unit random
```

For each unit the harness compiles all 16 combinations of `/O1` or `/O2`, `/Ob1` or `/Ob2`, `/G5`
or `/G6`, and `/MD` or `/MT` with VC6 SP5. All tracked reports use the single project hypothesis
`/O2 /Ob2 /G5 /MD`; a sparse unit is not allowed to invent its own historical flags. The profile
can be reconsidered later against the aggregate recovered-unit corpus. COFF relocation fields are
masked before comparison.

Each candidate is classified as `exact`, `relocation-equivalent`,
`near-source-with-wiz8-modifications`, `absent-or-stripped`, or `ambiguous-generic`. Executables
that cannot be compared statically remain in the report with an orthogonal `unavailable` state and
a reason, rather than being mislabeled absent. All units share the generated
`config/analysis/sgp/harness.csv`; conclusions that require human review live in
`config/analysis/sgp/reviewed-findings.csv`. Accepted identities remain in
`config/analysis/functions/wiz8-sgp.csv`, and source-path evidence remains in
`config/analysis/sgp/wiz8-source-paths.csv`.

The common profile reproduces all 13 established `DirectDraw Calls.c` functions in each of the
five comparable builds and all three `Random.c` functions in the same builds. These results preserve the legacy
cross-build address matrices while making the compiler search and negative results reproducible.

The released VC6 `Standard Gaming Platform.dsp` supports project-wide treatment: release compiler
options live on the project configuration, and the four currently probed units have no release
per-file overrides. This is evidence for flag scope, not proof that Wizardry used the same project
file; the profile remains a testable working hypothesis.

## `FileMan.c`: exact file-system boundary and two reviewed forks

`FileMan.c` emits 43 external functions under the common project profile. Fifteen have exact
relocation-masked identities in the canonical executable: `FileExists`, `FileExistsNoDB`,
`FileOpen`, `FileWrite`, `FileSeek`, `DirectoryExists`, `MakeFileManDirectory`,
`GetExecutableDirectory`, `GetFileFirst`, `GetFileNext`, `GetFileClose`, `W32toSGPFileFind`,
`FileCopy`, `FileGetAttributes`, and `FileClearAttributes`. Except for the one patch-specific case
below, the same identities recur in all five comparable modules. They are source-backed in
`config/analysis/functions/wiz8-sgp.csv`; the earlier descriptive names `OpenVirtualFile` and
`SeekVirtualFile` remain aliases of `FileOpen` and `FileSeek`.

The SGP model installs `HWFILE`, the one-byte `BOOLEAN`, `SGP_FILE_OPEN_FLAGS`,
`SGP_FILE_SEEK_ORIGIN`, `SGP_FILE_ATTRIBUTES`, the `0x110`-byte `GETFILESTRUCT`, the `0x08`-byte
`SGP_FILETIME`, the `0x140`-byte `WIN32_FIND_DATAA`, and all fifteen exact prototypes.

Two near matches are deliberately not promoted as exact:

* Every comparable build's `FileCheckEndOfFile` differs at four stable bytes. The released source
  indexes `LibraryHeaderStruct` with a `0x20` stride; Wizardry multiplies by `0x28`. Its nested
  `FileOpenStruct` indexing remains `0x10`, isolating the change to the Wizardry library header.
* `GetFileFirst` is exact in the demo, base executable, 1.261 base, and 1.28 base. Only
  `Wiz8New.exe` changes the immediate stored to `fFindInfoInUse[iWhich]` after `FindFirstFile` from
  `TRUE` to `FALSE`. The identity remains exact for the canonical executable, while the patched
  module is recorded as a Wizardry modification.

The byte offsets and behavioral deltas are tracked in the `fileman` rows of
`config/analysis/sgp/reviewed-findings.csv`; the generated similarities remain in the matching
rows of `config/analysis/sgp/harness.csv`.

## `Container.c`: retained stack and list APIs

The unchanged vendored unit emits 32 functions under the common profile. Wizardry retains ten
physical bodies in one source-ordered block from `0x00405970` through `0x00405E58`. They represent
twelve source identities: `CreateStack`, `CreateList`, `Push`, `Pop`, `PeekStack`, `DeleteStack`,
`DeleteList`, `PeekList`, `StoreListNode`, `StackSize`, `ListSize`, and `AddtoList`.

Six bodies are unique relocation-masked matches. Four short bodies collide with other source
functions and were resolved from the block and its callers. Calls to `0x004059B0` pass list element
sizes and are followed by `PeekList`/`AddtoList`; no queue operation survives. The sole caller of
`0x00405B90` uses the operation that the released source explicitly says was added for Wizardry,
`StoreListNode`, while `SwapListNode` is labelled as a JA2 addition. Both stack and list callers reach
the folded delete body at `0x00405B00` and the folded size body at `0x00405C00`, so both source names
are preserved as aliases. The other twenty identities are stripped.

The source defines a `0x0C`-byte `StackHeader`, `0x14`-byte `QueueHeader` and `ListHeader`, and
`0x18`-byte `OrdListHeader`. These layouts and the ten physical function prototypes are installed in
Ghidra under `/wiz8/sgp`. This API is unrelated to the first-party `3D Code/PList.cpp` family used by
the monster code; no `PList` field was inferred from `Container.c`.

## `LibraryDataBase.c` and `DbMan.c`: vendored SLF subsystem

These units are compiled directly from `third_party/sfi-sgp/sgp`; they are no longer treated as
implementation that needs independent reconstruction. `LibraryDataBase.c` emits 23 functions and
`DbMan.c` emits 20 under the common project profile.

Four `LibraryDataBase.c` identities are exact in all five comparable executables:

| Address | Source identity | Size |
| --- | --- | ---: |
| `0x00412B10` | `ShutDownFileDatabase` | 145 |
| `0x00413680` | `CreateRealFileHandle` | 175 |
| `0x00413730` | `GetLibraryAndFileIDFromLibraryFileHandle` | 31 |
| `0x00413D00` | `CompareDirEntryFileNames` | 83 |

`CheckIfFileExistInLibrary` and `CompareFileNames` retain the source behavior but index Wizardry's
`0x28`-byte `LibraryHeaderStruct`, which extends the released `0x20`-byte structure with the file
mapping handle and view already observed in the binary. The disk structures are unchanged:
`LIBHEADER` is `0x214`, `DIRENTRY` is `0x118`, `FileHeaderStruct` is `0x0C`, and `FileOpenStruct` is
`0x10`. Their source names and fields are installed under `/wiz8/sgp`, together with the extended
runtime structure and `DatabaseManagerHeaderStruct` global at `0x006EB720`.

`DbExists` is not promoted. Its 14-byte source wrapper occurs at `0x00519BEE`, fourteen bytes into
an existing 31-byte function after a range guard; it is an interior sequence rather than a function
start. The rejection is retained in `reviewed-findings.csv`.

### `Random`, not `GetRandomNumber`

`0x0040EFA0` is CFAgent's `pW8FUNC_GetRandomNumber` seed. Its relocation-masked body hash
`438ef441…` is exactly the hash the repository already had from its own hand-written port, and it is
now also exactly what released `Random.c` emits. The original shared-source name is therefore
`Random`; `GetRandomNumber` is retained as a fan-patch alias, since no Wizardry-side evidence shows
Sir-Tech renamed it.

This is the first name promoted out of `external-semantic` under the rules in
[docs/wiz8-evidence-model.md](../wiz8-evidence-model.md): the row now carries
`name_origin=sgp-source|fan-patch-signature` and `authority=source-backed`, and both `Random` and
`GetRandomNumber` are applied to the address. The neighbouring `InitializeRandom` and `Chance` had
no name from any source before this compile.

## Applied analysis model

The 13 exact DirectDraw identities are applied from `config/analysis/functions/wiz8-sgp.csv` and
remain classified as `sgp-shared`, not Wizardry gameplay code. The source-derived ABI can be replayed
with:

```sh
just ghidra apply-functions wiz8--gog-base--wiz8--18a74ff61c65 \
  --map config/analysis/functions/wiz8-sgp.csv
just ghidra apply-sgp-model wiz8--gog-base--wiz8--18a74ff61c65
```

The second command installs the 32-bit DirectDraw `RECT`, `DDCOLORKEY`, `DDPIXELFORMAT`, `DDSCAPS`,
`DDSURFACEDESC`, `PALETTEENTRY`, and `DDBLTFX` layouts, opaque COM interface types, and all 13 exact
C prototypes. For example, Ghidra now renders `DDLockSurface(IDirectDrawSurface2 *, RECT *,
DDSURFACEDESC *, dword, HANDLE)` and recognizes the descriptor's exact `0x6C` size.
