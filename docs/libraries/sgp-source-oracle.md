# Standard Gaming Platform source oracle

Wizardry 8 contains code from Sir-Tech's shared Standard Gaming Platform (SGP), and the released
Jagged Alliance 2 history preserves explicit Wizardry build branches. The authoritative oracle is
the `sgp/` tree at ja2-stracciatella commit
[`5ac0a9d56d27e8a7e2c4a7b48ed8932ae7f64033`](https://github.com/ja2-stracciatella/ja2-stracciatella/tree/5ac0a9d56d27e8a7e2c4a7b48ed8932ae7f64033/sgp),
the SGP-side `Initial Import` dated 2004-09-06. This is separate from the repository's JA2-side
initial-import root.

The source is covered by the Strategy First Source Code License Agreement. Its terms are not a
normal permissive open-source grant: they restrict use to non-commercial purposes and impose
conditions on redistribution and derivative works. We therefore pin and compile the source only as
a local oracle. No released SGP source is copied into this repository. The exact license artifact is
`sgp/SFI Source Code license agreement.txt`, Git blob
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

The prerequisite Just recipe clones the oracle into `WIZ8_WORK_DIR`, creates a detached worktree at
the pinned commit, and verifies `HEAD` before CMake sees it. The licensed source and COFF object stay
outside git.

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
them from this byte matcher. The complete address matrix is
`config/analysis/sgp/directdraw-cross-build.csv`.

The oracle also resolves the independently reconstructed zlib boundary. The five exact functions
at `0x00415820` through `0x004158F0` are the retained decompression half of `sgp/Compression.c`,
with original names `ZAlloc`, `ZFree`, `DecompressInit`, `Decompress`, and `DecompressFini`.

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

Two per-unit compile flags are part of the finding, not incidental:

* `/G5` is required. Under `/G6` only `InitializeRandom` — which contains no arithmetic — still
  matches; both `Random` and `Chance` diverge.
* `/Ob2` is required. Wizardry inlined `Random` into `Chance`, constant-folding the range to 100.
  Plain `/O2` implies `/Ob1` and will not inline a function that is not marked `inline`, so under
  `/O2` alone `Chance` emits an out-of-line call and never matches.

`Chance` is a good illustration of why per-unit flag sweeps matter: at `/O2 /G5` the unit looks like
two exact functions and one absent one, which invites the wrong conclusion that Wizardry modified or
dropped `Chance`. It did neither.

The complete address matrix is `config/analysis/sgp/random-cross-build.csv`. The whole unit sits in
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
or `/G6`, and `/MD` or `/MT` with VC6 SP5. It selects one flag combination for the complete
translation unit by scoring every emitted function against every reviewed executable; flags are
never selected independently per function. COFF relocation fields are masked before comparison.

Each candidate is classified as `exact`, `relocation-equivalent`,
`near-source-with-wiz8-modifications`, `absent-or-stripped`, or `ambiguous-generic`. Executables
that cannot be compared statically remain in the report with an orthogonal `unavailable` state and
a reason, rather than being mislabeled absent. The tracked reports are
`config/analysis/sgp/directdraw-harness.csv` and `config/analysis/sgp/random-harness.csv`.

The sweep selects `/O2 /Ob1 /G5 /MD` for `DirectDraw Calls.c` and reproduces all 13 established
functions in each of the five comparable builds. It selects `/O2 /Ob2 /G5 /MD` for `Random.c` and
reproduces all three functions in the same five builds. These results preserve the legacy
cross-build address matrices while making the compiler search and negative results reproducible.

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
