# Wizardry executable source and class model

## Translation-unit tree

Raw strings in the main executables preserve 149 distinct absolute Wizardry source paths. The
canonical GOG program contributes 136 paths rooted at `C:\Projects\Wizardry 8`; the demo uses
`E:\Wizardry 8` and adds 13 units not present in the canonical release strings. The reviewed tree
is tracked in `evidence/observations/wiz8/source-tree.csv` with exact absolute spellings and per-build
presence.

| Original directory | Units |
| --- | ---: |
| `Engine Code` | 50 |
| `Local Code` | 48 |
| `Local Screens` | 27 |
| `Level Specific Code` | 13 |
| `Dialog Code` | 9 |
| `3D Code` | 2 |

The demo-only units are `GDCamera.cpp`, `gap.c`, `PolyPick.cpp`, `Game Difficulty.cpp`,
`Gameplay Init.cpp`, `QuoteManager.cpp`, `Test.cpp`, `ThingEditorShared.cpp`, `MGSFormation.cpp`,
`MGSPortraitCombat.cpp`, `MGSRadarMap.cpp`, `NPCInteractionSubscreen.cpp`, and
`RCSStatsPage.cpp`. “Demo-only” here means only that the retained absolute string is demo-only; it
does not by itself prove that no corresponding code survived in retail.

The older generated `build/evidence/source-paths.csv` is not authoritative for this model because
its extractor truncates `.cpp` paths to `.c`. The tracked tree was rebuilt from raw NUL-terminated
binary strings and retains the original extensions.

## Translation-unit layout

The reviewed assertion paths remain the strongest anchors, but the same absolute source strings also
appear in allocation macros, diagnostics and error routines. `wiz8decomp.ghidra.unit_intervals`
scans the live Ghidra program for references to Wizardry source paths, resolves the containing
function, and treats each as a direct anchor; reviewed assertion rows stay a separate anchor kind
that also carries their line. Header spellings such as `..\Engine Code\Include\AnimRep.hpp` are
header-origin inline evidence, never unit anchors, and a function naming two distinct `.cpp` paths
resolves to `inlined-or-conflicting` rather than one owner.

Ordinary non-COMDAT functions emitted by one translation unit occupy one contiguous `.text`
contribution, so the convex hull of a unit's direct anchors is hard-owned while everything outside
every hull remains an explicit gap. Hulls of distinct units must not overlap; an overlap is a model
contradiction, surfaced rather than papered over. A gap may contain a unit's unanchored tail, an
invisible TU, or the next unit's head, and is never assigned heuristically. Other official builds
(demo, 1.2.6, 1.2.8) contribute `cross-build` anchors through unique relocation-insensitive body
matches, which can establish a retail hull for a unit whose retail path string is gone; ambiguous or
non-unique matches stay unknown. The same layout drives `wiz8 report context`, `wiz8 recover`, and
`wiz8 report translation-units`; the placement validator in `uv run wiz8 check` compares it against
the current source-index placement and enforces every anchored function. The earlier provisional Video2
exemptions are gone: the cursor, window and dirty-tile bodies were consolidated into
`src/wiz8/engine_code/Video2.cpp` rather than kept in invented semantic units.

## RTTI result

The canonical executable contains no MSVC Type Descriptor strings beginning with `.?AV` or `.?AU`.
This is consistent with the `/GR-` configuration already required by the matching extension build.
Consequently, there are zero exact RTTI class names to export from `Wiz8.exe`; local class recovery
must use vtable writes, object construction/destruction, source paths, and behavior. Imported
SurRender decorated names remain external ABI evidence, not local Wizardry RTTI.

## Assertion expressions

`SR.DLL`'s `srAssertFail` is reached two ways: 1048 sites call through the import slot directly
(`FF 15`), and 729 more call through a register that VC6 hoisted the slot into (`mov edi, [slot]`
… `call edi`), which neither a byte scan for the direct encoding nor Ghidra's xref list can see.
`evidence/observations/wiz8/assertions.csv` records all 1777 sites for the canonical retail
program: call site, call kind, containing function, source path, line, the **expression text**,
and the optional fourth-argument **message**. All but one direct site decode their literal
arguments. They span 128 files; 789 distinct functions contain at least one site, and 84 sites
fall outside any function the canonical Ghidra program currently defines and record an empty
containing function. The containing function is resolved through the reviewed canonical
program. The reviewed table is the durable interpretation; focused Ghidra
queries provide current call and function facts without a parallel raw harvest.

The message argument is usually null, but 349 sites pass one, and messages are a different naming
channel from expressions: expressions name members, parameters and constants, while messages tend
to name the enclosing routine or class — `"Too many props loaded for Octree"` named the `Octree`
class and `"GetNumSubsPerCycle() -> Invalid cycle num."` named the method, and both claims now
cite the `message` column rather than prose.

The expression half is the valuable part, and it is a different kind of evidence from the source
path. A path assigns a function to a translation unit; an expression names identifiers:

| What it yields | Count | Examples |
| --- | ---: | --- |
| Member accesses through `->` or `.` | 366 | `pTrigger->m_pacRecipients`, `pWorld->plsProps`, `pWorld->psrMeshes`, `pSound->pacSoundName`, `pLVL->pProps[i].bNumFrames` |
| Named game constants and enumerators | 88 | `BAD_INDEX`, `MAX_MONSTERS_IN_DATABASE`, `HAND_COUNT`, `SPELL_COUNT`, `PHASES_PER_ROUND`, `TRIGGER_REP_PROP`, `BEHAVIOUR_FIRST`/`BEHAVIOUR_LAST` |
| Globals (`g`/`gp`/`gui` prefixes) | 138 | `glsTimedEvents`, `gpGDCamera` |

Ninety-three distinct ALL-CAPS tokens appear, but five of them — `NULL`, `FALSE`, `INT32`, `UINT16`
and `UINT32` — are a null pointer constant, a boolean and three typedef names, which the prefix
table below already treats as type evidence. The game-side count is therefore 88.

Identifiers are Hungarian-coded, and that coding is established from the original's own text rather
than inferred:

| Prefix | Uses | Meaning implied by use |
| --- | ---: | --- |
| `p` | 885 | pointer |
| `ui` / `i` | 227 / 195 | `UINT32` / `INT32` (both names appear literally in casts) |
| `f` | 172 | flag |
| `b` / `ub` / `us` | 60 / 17 / 20 | byte, unsigned byte, `UINT16` |
| `g` / `gp` / `gui` | 82 / 35 / 21 | global, global pointer, global `UINT32` |
| `psr` | 28 | pointer to a SurRender object |
| `pls` | 49 | list-bearing pointer; both `PList.cpp` and `IList.cpp` use it, so the concrete list type needs consumer evidence |
| `pac` / `pst` / `h` | 17 / 24 / 23 | pointer to char array, pointer to struct, handle |

This narrows fields the disassembly leaves opaque, but it does not replace consumer evidence.
`pWorld->plsProps` is a `PList` because its users call the reviewed PList accessors; `psrMeshes`
identifies a SurRender-facing pointer independently.

An `m_` member prefix is **not** a project-wide convention, and an earlier revision of this document
wrongly said it was. Only 90 distinct `m_` identifiers appear, and 266 of the 277 `->` member-access
assertions contain no `m_` at all — including four of the five examples in the table above. `m_` is
used by some classes, notably `Trigger`, the `Oct*` family, `GDFileIO`'s trigger arrays and `Item`'s
representation object, while most member accesses are plain Hungarian names. Treat `m_` as a
per-class habit to be checked, not as a rule to apply when naming a recovered field.

The paths also extend the tree. All 124 absolute `.cpp` assertion paths — including eleven files
such as `Local Code\Gameloop.cpp` and `Engine Code\Cursor3d.cpp` that only register-indirect sites
reach — already appear in `source-tree.csv`, which independently confirms that census is complete
for `.cpp`. But four assertions come from headers the absolute-path scan could never have found,
because they are recorded relative:

```text
..\Engine Code\Include\AnimRep.hpp
..\Engine Code\Include\Trigger.hpp
..\Engine Code\Include\stHeap.hpp
..\Engine Code\Include\stLight.hpp
```

That establishes an `Engine Code\Include` directory and an `st*` family alongside the already-known
`stCube.cpp`. Inline code in headers is attributed to the header, not the including unit.

## How the original signals failure

Wizardry 8 ships four failure mechanisms and none of them is a C++ exception — `/GX` is on and 479
functions carry unwind frames, but `_CxxThrowException` is not imported, so nothing throws. A
recovered function that appears to need a `try`/`catch` has been misread.

1. **Assertions, shipped enabled in retail.** Every one of the 1777 sites calls SR.DLL's
   `srAssertFail`, and Wizardry installs its own handler: `srAssertSetFunc` has exactly one
   reference, inside `InitializeVideoDevice` (`0x00422240`), installing `AssertFailureHandler`
   (`0x00428AB0`). The handler copies the developer-notice preamble at `0x006042F4` into a stack
   buffer, appends *"Debug assertion in module %s line %d failed: Expression [ %s ] evaluates to
   false"* plus the optional message, and hands the text to SGP's `ShutdownWithErrorBox`
   (`0x00401920`) — which stashes it in `gzErrorMsg` and calls `exit(0)`, so the report surfaces
   through the SGP shutdown path. Two contracts follow, and they are different: at **runtime** a
   failed assert terminates the process; in the **emitted code** `srAssertFail` is an ordinary
   returning call and every site falls through into the guarded code, which is load-bearing for
   byte-exact ports. `GetMonsterDataByID` asserts its index and then indexes anyway; port the
   fall-through, never an abort.
2. **Null and sentinel returns, checked defensively at the container boundary.** `PLLength`
   (13 bytes, 572 direct callers) maps a null list to 0; `PLGet` (26 bytes, 178 sites) maps null
   or out-of-range to 0; `PListIndexOf` returns `BAD_INDEX`, which the byte-proven Targeting pair
   pins to `-1`. Callers routinely pass unvalidated indices and test the result — that is the
   idiom, not a bug, and the ported PList accessors reproduce it.
3. **Boolean status returns** — `unsigned char` success/failure on loaders and accessors
   (`LoadMonsterDatabaseRecord`, `LevelGetLocationCodeByID`, `LevelBuildInfoByID`).
4. **Formatted diagnostics through shared static buffers.** `FormatString` (`0x00517A70`)
   vsprintf's into the 200-byte narrow buffer at `0x0068BFD0` and returns it; `FormatWideString`
   (`0x00517A90`) uses the 8-KB wide buffer at `0x00689FD0`. Neither is reentrant, and two calls in
   one expression alias each other — the recorded original bug where
   `MonsterGetIndexByLocationID` reuses one diagnostic argument across both paths is exactly that
   shape. `FormatDebugMessage` (`0x005182E0`) formats into a stack buffer and **discards it** — the
   release build's log call retains no sink — while `WriteGameLog` (`0x0058AAD0`, 541 call sites)
   is the live wide-character channel feeding the on-screen text sink at `0x0058AC00`.

## UI ownership

The recovered UI has two largely separate control systems, not one universal widget
hierarchy. They share lower-level rendering services; this distinction does not imply
disjoint dependencies. The source declarations remain authoritative:

```text
Controls panel
  manages W8Widget objects
    W8TextControl derives from W8Widget
      contains W8TextBuffer

W8DialogBase
  modal branch: W8ModalDialogBase -> W8NotificationDialog
  other branches: monster, spell, and other dialog families
    contain button / scrollbar / text-area helpers as needed

W8DialogTextArea
  owns W8DialogTextEntry objects
  keeps a separate non-owning visible-entry list

W8DialogTextEntry derives from W8TextBuffer
```

The panel in [Controls.h](../include/wiz8/local_code/Controls.h) holds pointers to
[widgets](../include/wiz8/local_code/Widget.h), and each widget holds its panel pointer.
[TextControl](../include/wiz8/local_code/TextControl.h) adds interaction state while its
embedded [TextBuffer](../include/wiz8/local_code/TextBuffer.h) handles text layout and rendering.
Range controls and selection listeners also have dedicated declaration headers.
Their implementations remain together in the original `Local Code/Controls.cpp` unit;
the header split does not claim original header names. Panel and widget are not bases of
one another; TextControl and TextBuffer are not duplicate identities.

[DialogBase.h](../include/wiz8/dialog_code/DialogBase.h) describes only the separate SGP
Button-System shell. Button, scrollbar and text-area helpers have their own headers
and provisional implementation units; concrete dialogs include the helpers they contain.
Factory-dialog and spell-dialog declarations likewise live outside the base header. The
[modal base](../include/wiz8/dialog_code/ModalDialogBase.h) is only one inheritance branch.
[TextArea](../include/wiz8/dialog_code/DialogTextArea.h) has its own provisional
implementation in `dialog_code/DialogTextArea.cpp`, separate from its dialog consumers. It is
nonpolymorphic; its two vectors own their pointer storage, but its destructor deletes
entries only through the owning collection. The
[dialog text entry](../include/wiz8/dialog_code/DialogTextEntry.h) extends TextBuffer
with palette, prefix, filtering and state data plus its own rendering behavior. An empty
destructor does not imply an empty derived object.

Names describe recovered roles unless independently tied to original identifiers.
In particular, the filename `Controls.cpp` does not prove that the panel's original
class name was `Controls`. Likewise, matching four-integer layouts do not establish
that `W8ControlsRect` and `W8ScreenRect` were one source type; that identity remains
unresolved. Similar UI responsibilities alone do not justify merging classes.

## Compiler-backed type gate

`wiz8 lint` is the authoritative source-model/type gate. It configures the
same CMake source lists as the product build, compiles every manually owned
translation unit with clang-cl at `/W4 -Werror` plus the recovery diagnostics
(`-Wsometimes-uninitialized -Wswitch -Warray-bounds -Wsign-compare
-Wmissing-field-initializers -Woverloaded-virtual
-Winconsistent-missing-override -Wshadow-field`), and then runs the narrow clang-tidy profile
from `.clang-tidy`. `WIZ8_CLANG_LINT` is an umbrella over the Wizardry game
sources, SurRender, `WIZ8_SGP`, and the recovered/adapted JPEG and UnZip
plugin code; the pristine IJG and Info-ZIP trees keep their upstream warnings.
`wiz8 diagnostics` uses the same recovery diagnostics report-only and runs
the broader `.clang-tidy-diagnostics` profile for trial checks.

The product VC6 build and the clang-cl lint lane share one interface target
(`cmake/CompileSettings.cmake`) for includes, forced compatibility header and
product definitions, so the lint lane cannot drift into a parallel
approximation of the product build. `/G6` is the only setting that stays
VC6-only. Each component additionally compiles its recovered sources once as
an object target (`wiz8_recovered_objects`, `wiz8_surrender_objects`, the
`wiz8_jpeg_*_objects` and `wiz8_unzip_*_objects` groups) that both the product
link and the lint lane consume; components register those targets with
`wiz8_lint_target()` in `cmake/Lint.cmake`, which applies the modern
diagnostics to the exact same sources, headers, defines and per-source
properties. `/Zp4` for UnZip is layout and rides along in both lanes, while
`/GX` is VC6 codegen only and stays behind a compiler-id guard (Clang rejects
it as unused and never reproduces EH bytes).

An intentional original behavior that trips a recovery diagnostic gets a
function-local `#pragma clang diagnostic` with the binary/source evidence in
the comment. The retained SGP C library is the one target-level exception: its
upstream C style warnings stay report-only because fixing them would mean
rewriting vendor source. `wiz8 diagnostics` is fully non-gating: SGP gets its
four recovery warnings report-only there and promotes them to errors only in
the gating lane.

The same Clang projection feeds `build/source-index.json`. Index targets
derive from every reccmp target with a `source-root` that has compile-database
coverage, so the first-party JPEG and UnZip sources are indexed alongside
`WIZ8` and `SURRENDER`. Each target is its own link namespace, so collection
is partitioned by source root with a separate cache: the same unmangled
symbol may legitimately be defined in several binaries (both extension DLLs
define `DllMain` as `_DllMain@12`), and one shared collector would keep only
one of those definitions and leave the other target's marker unbound.
External vendor translation units (`/zlib`, `/infozip`) are not collected
standalone: their headers are already parsed through the first-party units
that include them. Each namespace fingerprints only the include directories
its own compile commands reference, so a `src/wiz8` edit does not invalidate
the SURRENDER or extension caches, and the compiled Clang collector is built
once and reused across the per-namespace caches.

C++ mangling already encodes the complete type, so divergent C++ declarations
cannot share a symbol. The reccmp indexer records variable declarations with
canonical type, linkage, and definition kind alongside function linkage, and
retains every distinct spelling it saw per identity. The cross-TU consistency
gate over those records (`validate_cross_tu_declarations`) is parked for B:
its remaining hits are the legal extern-array completion idiom (`extern T g[]`
completed by `T g[N]`), which needs an array-aware compatibility rule before
it can gate. It stays tested but uncalled in the meantime.

The lint lane itself runs on the trixie image with LLVM 19, and the
clang-tidy profile includes `readability-redundant-casting`,
`bugprone-misplaced-widening-cast`, tuned `bugprone-sizeof-expression`, and
`bugprone-swapped-arguments`.

## Live recovery state

This document does not inventory current classes, layouts, match counts, or unresolved
lifecycle work. Those facts change with ordinary recovery and become harmful when copied into
prose.

Use the authoritative surfaces instead:

- C++ declarations, inheritance, `static_assert` layout checks, and function markers for the
  source-owned model;
- `uv run wiz8 report context 0x<address> --program <program>` for joined identity, ownership,
  assertion, and current Ghidra evidence;
- `uv run wiz8 analyze source-layouts` for the current PDB-to-Ghidra layout audit;
- `uv run wiz8 compare <addresses>` for relocation-masked body proof;
- reviewed claims under `evidence/` for why an accepted identity or layout is trusted.

Historical recovery examples belong in commit and Bead history, not in a manually maintained
snapshot here.
