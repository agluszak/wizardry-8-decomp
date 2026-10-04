# Wizardry 8 Standard Gaming Platform

This is our reconstruction of the Wizardry fork of Sir-Tech's source-available
SGP project. It is the only buildable SGP implementation. `WIZ8_SGP` is one
static library shared by the comparison, runtime, and runtime-test executables.

`src/sgp` is editable recovered source, like `src/wiz8`, with unusually strong
ancestry evidence. It is **not** the source oracle.

## Released baseline (the oracle)

The immutable ancestor oracle is the `sgp/` directory of
`https://github.com/ja2-stracciatella/ja2-stracciatella.git`, commit
`5ac0a9d56d27e8a7e2c4a7b48ed8932ae7f64033` (Initial Import, 2004-09-06), tree
`52766c4237e63d7a3d619796784947ed1681f24e`, `sgp/` subtree
`5cf5916502ee719fe4966b7f15ec643379ecb390`.

A byte-identical copy (every blob matches the upstream subtree) is in this
repository's history at commit `72697ddaac1c`, path `third_party/sfi-sgp/sgp/`:

```sh
jj file show -r 72697ddaac1c 'third_party/sfi-sgp/sgp/<file>'
```

The released prebuilt `SMACKW32.LIB`, `ddraw.lib`, and `mss32.lib` were never
imported. The later move commit `cb593aff` already carried a Wizardry
`english.h`; it is not a pristine reference. Claims that cite
`released-baseline:fdff790b` refer to that import tree (pristine apart from
`english.h`).

Released SGP is shared JA2/Wizardry/utility source. It is an excellent ancestor
for names, types and bodies, but Wizardry's SGP was a fork. Released bodies
are evidence only where binary/object evidence shows Wizardry retained them.

## Ownership versus provenance

- A `FUNCTION`/`GLOBAL` marker in `src/sgp` establishes current **ownership**:
  the address belongs to the SGP component and must not be recovered in
  `src/wiz8`. It is not evidence that the name or body is original.
- `sgp-source` provenance comes only from an explicit reviewed claim in
  `evidence/reviewed/wiz8/claims.csv`, backed by the released baseline and an
  object/binary comparison. The `source-oracle` gate reports SGP owner markers
  that lack such a claim as unproven ownership
  (`build/reports/source-oracle.json`, `"proven": false`).
- Names introduced here without a released counterpart (for example
  `OpenLibraryStream` at `0x00412f10`) are our names until evidence says
  otherwise.

## Licence

The accompanying `SFI Source Code license agreement.txt` is retained verbatim
(upstream blob `b66aabc6f7affb4fca0b6cc2d6288f3225ecd0b1`). Preserve it and all
upstream notices. Changed files carry dated modification notices. This component
is not offered under commercial-use or broader terms.

## Project boundary

`CMakeLists.txt` records the reconstructed object membership directly. Only
source that could plausibly have constituted Wizardry's SGP library is kept:

- Released units with no Wizardry contribution (`DbMan.c`,
  `ExceptionHandling.cpp`, `Flic.c`, `Install.c`, `Ja2 Libs.c`,
  `Mutex Manager.c`, `WinFont.c`, `video.c`), their orphaned headers, the
  JA2 Visual Studio project, the `JA2 SGP ALL.H` umbrella and the Smacker/RAD,
  `dsound.h`, `trle.h` and `Bitmap.h` headers were removed; the baseline above
  retains them. `DbMan.h` stays for the `HDBFILE` handle in `FileMan.cpp`.
  Wizardry's renderer is `Video2.cpp`, not released `video.c`.
- `Mutex Manager.h` stays although no retained code calls the mutex API:
  removing its declarations from `Video2.h`/`sgp.h` changes VC6 code
  generation in product TUs and breaks the exact `W8OptionsPanel` constructor
  at `0x005a81e0`. Wizardry's headers therefore still declared that API, as
  released `video.h` does.
- The released JA2, utility (`UTIL`/`UTILS`) and precompiled-header
  configuration branches are collapsed to the Wizardry build. Their removal
  changes no generated code except `__LINE__` immediates, and every retained
  `DirectDraw Calls.cpp` `ATTEMPT` line number moves closer to retail's value.

## Retained-function audit (2026-10-03)

Every retail function start in the SGP contribution range
(`0x004011e0..0x00415910`, found from all call, immediate and data references)
carries a marker. An emitted SGP body without a marker is therefore inlined,
folded into a marked body, or not retained by the retail link. Each emitted
body of the 30 units was classified against that image and the released
baseline:

- retained released body (299) and recovered Wizardry delta/addition (41);
- retail fold (`DeleteList`, `ListSize`, the `0x004023a0` no-op family) and
  inline/fold dependencies referenced by retained code (94);
- not retained and not referenced by any retained or product code (335).

The unreferenced, unretained bodies and their prototypes were removed, except
in `DirectDraw Calls.cpp`. There, retail `ATTEMPT` `__LINE__` immediates keep the
released spacing between retained functions (for example `DDGetSurfaceDescription`
120 → `DDRestoreSurface` 224 in retail). So the eleven unretained bodies between
them were present in Wizardry's file, and they stay. Elsewhere, absence from the
link proves only that a body was not retained; the original fork may still have
contained it.

SGP comparison findings after the delta recovery are classified:

- `DirectDraw Calls.cpp` `ATTEMPT` line numbers exceed retail by exactly one per
  `// FUNCTION:` marker above the call (plus the modification notice): the
  drift is our annotation, not authored source. The released macro still uses
  `__LINE__`/`__FILE__`; the unit now compiles from its ordinary checkout path.
  reccmp normalizes diagnostic source coordinates as build context, without
  hard-coded historical coordinates or a simulated retail source tree.
- Past-the-end loop bounds (`pSoundList`, `pSampleList`, `gFileDataBase`) and
  calls into the `0x004023a0`/`0x005a1140` folds are relocation/fold noise.
  reccmp names identical reference-free stubs symmetrically across both images;
  this does not establish their original source names or template arguments.
- `GetRuntimeSettings` is inlined into `InitializeStandardGamingPlatform` by
  the recomp but called by retail; both standalone bodies match. The remaining
  `LibraryDataBase.cpp`, `RedirectToString` and `AddSubdirectoryToPath` residuals
  are block-layout, CSE and stack-slot lowering.

Retail list callers reach the folded size/delete bodies through `ListSize` and
`DeleteList`; product code uses the list API for `HLIST` values.

SGP's product calls use the real `GameData.h`, `Video2.h`, `game_init.h`, and
`local_code/Gameloop.h` declarations. `MoveTimer` and its action constants belong
to `GameData.h`;
product font handles belong to `fonts.h`. The C++ source has no parallel
`sgp_bridge.h` or game-loop umbrella interface. Product timer, video, game-loop,
and octree calls use ordinary C++ linkage. Historical SGP API linkage remains
in the SGP headers; `gap.c` retains its real C boundary.

The empty `builddefines.h` and unused `local.h` configuration shells and their
lint filename adapters are removed. The VC6 SDK supplies `zmouse.h`, including
wheel constants hidden by the old `WINUSER.H` target-version guards. No
`include/wiz8/sgp-compat` tree or include path remains. Other `tools/lint/include`
filename-capitalization adapters
still resolve the released SGP names on the host filesystem.

Cleanup verification (`run-8qc2gk6v`, 2026-10-04) selects 5,688 addresses:
2,797 `no-differences`, 2,687 `differences`, and 204 explicitly classified
compiler non-emissions, with zero unpaired or analysis failures. The same 340
SGP functions now have 290 `no-differences` and 50 `differences`, three gains
and zero regressions against the migration report. Comparing common functions
with PR #861's saved CI report adds one residual: VC6 emits exception-unwind
setup around the allocation in `MoveTimer` after its declaration gains C++
linkage. Its authored body is unchanged; no exception or optimizer controls are
added to reproduce the former C-linkage output.

## Known Wizardry deltas

The recovered deltas live in their original units, including the 252-character
font table, startup and input behavior, sound-cache revision, and SLF mapping
and patch precedence. The retail English input table is **512 words**, not the
released 1,024: its two character banks occupy `0x005ffc3c..0x0060003b`.
Library initialization records are `0x103` bytes and library records are `0x28`
bytes; the latter include the patch flag and mapping fields. `WizLibs.cpp` owns the
product library configuration: 50 records, six initially populated. Retail
allocates 56 open-library records separately; the two capacities are not the
same. The patch loop is preserved, including this original capacity discrepancy.
`UnlockMouseBuffer`, `VideoCaptureToggle` and SGP's two-argument
`PlayButtonSound` share the retail no-op at `0x004023a0`; `DeleteList`/
`DeleteStack` and `ListSize`/`StackSize` are folded pairs. The
The retail `DirectDraw Calls.c` `__FILE__` string records the historical
`C:\Projects\SGP` build path; our compiler uses the ordinary checkout path.

JA2 Utils `Text_Input` is not an SGP unit. The Wizardry derivative remains
product code; its released ancestor is `ja2-stracciatella/ja2-stracciatella`
commit `5ac0a9d56d27e8a7e2c4a7b48ed8932ae7f64033`,
`ja2/Build/Utils/Text_Input.{c,h}`.
Miles startup/exit support belongs to the Miles import boundary, not to SGP.

## Comparison

### Mechanical C++ migration (2026-10-04)

The last-C source checkpoint is commit
`6410dae65aa6bd910c40bfae39fbb059d7a2c403`, immediately before the first
C++ migration commit `39d5d8fdc1a170c63bc510561908fe98972f7cfa`.
Git preserves that implementation; there is one maintained SGP source tree.

All 30 retained translation units now compile as C++. Changes are limited to
the source extensions/build membership, explicit pointer conversions, character
buffer types, missing declarations/includes, and removal of duplicate tentative
definitions. The historical public API keeps its `extern "C"` linkage.
`CINTERFACE` and `COBJMACROS` preserve the existing DirectDraw call expressions;
the C++ SDK's `REFIID` parameter takes a GUID reference. No allocator, ownership,
container, `BOOLEAN`, or class modernization is included.

The initial conversion omitted `impTGA.h` from its own implementation. Its
`LoadTGAFileToImage` definition acquired C++ linkage while callers retained C
linkage, leaving an unresolved symbol and preventing archive extraction of both
marked TGA functions. Including the owning header restores the historical API
linkage; private importer helpers may use C++ linkage.

Saved pre-migration comparison evidence remains under `build/`: the whole-SGP
report `run-qmn7a37x` selected 340 functions (282 `no-differences`, 58
`differences`), before the generic comparison normalization fixes. The later C
report `run-57v1n5d9` covers 235 SGP functions (208 `no-differences`, 27
`differences`). These reports have different selections and tool/build inputs;
their counts alone do not measure the effect of the language switch.

The final whole-SGP C++ comparison (`run-gq0ykkzl`) selects all 340 marked
functions: 287 `no-differences`, 53 `differences`, zero unpaired and zero analysis
failures. Both TGA functions are `no-differences`. Its rebuilt PE SHA-256 is
`1d90b377e9218dbea73c09b88814d22cc0c129728b74e6d17ac90b52f146d311`,
using reccmp revision `4902aabd3f5d6b9f6b1ac686b686803479207e59`, Ghidra
12.1.4 and Ghidriff 1.0.0. The VC6 build has no unresolved symbols; `pr-check`
and merge-preservation pass. Generated reports and build products stay untracked.

The C++ residuals include changed helper inlining (`DequeueEvent`,
`DeleteVideoObject`), string-copy lowering (`InitializeButtonImageManager`),
and C++ local-static symbol spelling (`WindowProcedure`, `RenderFastHelp`).
Keep natural calls and expressions rather than shaping them to reproduce the
C compiler's output. Build-context and decompiler-label differences are not
behavioral-equivalence proofs. The subsequent source-ownership cleanup removes
the bridge and compatibility shells as described above.

Compile and compare whole translation units so `/Ob2` sees the actual helpers,
globals and headers. reccmp's COFF object view retains functions, static symbols,
data, common/BSS storage, section attributes and relocation targets. An exact
selected-contribution comparison masks relocatable operands at an independently
known original extent; it does not establish relocation-target identity or
authored source syntax. It requires no retained function in a recompiled PE.

`/OPT:NOREF` remains the comparison-image mode and `/OPT:REF` the runtime mode;
both link this same archive. Extra retained comparison-image functions are a
linker diagnostic, not a reason to build a different platform implementation.

Ghidra owns live identities and types; current reccmp comparisons own current
comparison results.
