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
2. **Null and sentinel returns, checked defensively at the container boundary.** `PListGetCount`
   (13 bytes, 609 call sites) maps a null list to 0; `PListGetAt` (26 bytes, 178 sites) maps null
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

## Live recovery state

This document does not inventory current classes, layouts, match counts, or unresolved
lifecycle work. Those facts change with ordinary recovery and become harmful when copied into
prose.

Use the authoritative surfaces instead:

- C++ declarations, inheritance, `static_assert` layout checks, and function markers for the
  source-owned model;
- `just wiz8 report context 0x<address> --program <program>` for joined identity, ownership,
  assertion, and current Ghidra evidence;
- `just wiz8 analyze source-layouts` for the current PDB-to-Ghidra layout audit;
- `just verify-boundaries` for relocation-masked body proof;
- reviewed claims under `evidence/` for why an accepted identity or layout is trusted.

Historical recovery examples belong in commit and Bead history, not in a manually maintained
snapshot here.
