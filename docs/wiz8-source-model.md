# Wizardry executable source and class model

## Translation-unit tree

Raw strings in the main executables preserve absolute Wizardry source paths. The
canonical GOG program uses `C:\Projects\Wizardry 8`; the demo uses
`E:\Wizardry 8` and preserves additional unit paths. The reviewed tree
is tracked in `evidence/observations/wiz8/source-tree.csv` with exact absolute spellings and per-build
presence.

“Demo-only” means that a retained absolute source string is demo-only; it does not prove that corresponding code is absent from retail.

## Translation-unit layout

The reviewed assertion paths in `evidence/observations/wiz8/assertions.csv` anchor their
containing functions to translation units (`wiz8decomp.unit_intervals`). Header spellings such as
`..\Engine Code\Include\AnimRep.hpp` are header-origin inline evidence, never unit anchors, and a
function naming two distinct `.cpp` paths resolves to `inlined-or-conflicting` rather than one
owner.

Ordinary non-COMDAT functions emitted by one translation unit occupy one contiguous `.text`
contribution, so the convex hull of a unit's direct anchors is hard-owned while everything outside
every hull remains an explicit gap. Hulls of distinct units must not overlap; an overlap is a model
contradiction, surfaced rather than papered over. A gap may contain a unit's unanchored tail, an
invisible TU, or the next unit's head, and is never assigned heuristically. The placement validator
in `uv run wiz8 check` compares this layout against the current source-index placement and enforces
every anchored function.

## Linker folding is not source ownership

Retail identical-code folding can retain one machine body for several independently authored
functions. That is a property of the linked executable, not a source-level alias or ownership
relationship. Recovered C++ therefore does not use reccmp `FOLDED` markers: the retained retail
emission carries the address marker, while independently evidenced sibling source functions remain
ordinary unmarked definitions with their own types and translation-unit placement.

The comparison image deliberately links with `/OPT:NOICF`. A type-correct sibling can therefore
compile to a separate body and leave an expected call-target or vtable-target mismatch against the
single retail emission. Once independent evidence establishes the ICF relationship, keep that fact in
reviewed evidence or comparison diagnostics rather than changing the source model to improve a score.

## Header architecture

TU ranges place out-of-line functions. They do not by themselves prove original header
filenames. Only `AnimRep.hpp`, `Trigger.hpp`, `stHeap.hpp` and `stLight.hpp` occur as
actual source paths in assertion evidence; other header names and splits are recovered
roles, not original spellings.

Headers under `include/wiz8/layouts/` hold packed records, enums and the globals that are
that storage. Interface headers declare functions implemented in one original TU unless the
declarations genuinely span several (for example `float_constants.h`). Inline members and
template implementations belong to the header that defines them.

SGP compiles as C++ and consumes owning product headers directly. `MoveTimer`
and its action constants belong to `GameData.h`; game font handles belong to
`fonts.h`.
Game initialization belongs to `game_init.h`; game-loop entries belong to
`local_code/Gameloop.h`. Video and octree product calls have ordinary C++ linkage.
Historical SGP API declarations retain their C linkage in the SGP headers.

## RTTI result

The canonical executable contains no MSVC Type Descriptor strings beginning with `.?AV` or `.?AU`.
This is consistent with the `/GR-` configuration already required by the matching extension build.
Local class recovery
must use vtable writes, object construction/destruction, source paths, and behavior. Imported
SurRender decorated names remain external ABI evidence, not local Wizardry RTTI.

## Assertion expressions

`SR.DLL`'s `srAssertFail` is reached through the import slot directly
(`FF 15`) or through a register that VC6 hoisted the slot into (`mov edi, [slot]`
… `call edi`), which neither a byte scan for the direct encoding nor Ghidra's xref list can see.
`evidence/observations/wiz8/assertions.csv` records reviewed sites for the canonical retail
program: call site, call kind, containing function, source path, line, the **expression text**,
and the optional fourth-argument **message**. Sites outside a function currently
defined in Ghidra record an empty containing function. Containment is resolved through the reviewed canonical
program. The reviewed table is the durable interpretation; focused Ghidra
queries provide current call and function facts without a parallel raw harvest.

Messages are a different naming
channel from expressions: expressions name members, parameters and constants, while messages tend
to name the enclosing routine or class — `"Too many props loaded for Octree"` named the `Octree`
class and `"GetNumSubsPerCycle() -> Invalid cycle num."` named the method, and both claims now
cite the `message` column rather than prose.

The expression half is the valuable part, and it is a different kind of evidence from the source
path. A path assigns a function to a translation unit; an expression names identifiers:

| What it yields | Examples |
| --- | --- |
| Member accesses through `->` or `.` | `pTrigger->m_pacRecipients`, `pWorld->plsProps`, `pWorld->psrMeshes`, `pSound->pacSoundName`, `pLVL->pProps[i].bNumFrames` |
| Named game constants and enumerators | `BAD_INDEX`, `MAX_MONSTERS_IN_DATABASE`, `HAND_COUNT`, `SPELL_COUNT`, `PHASES_PER_ROUND`, `TRIGGER_REP_PROP`, `BEHAVIOUR_FIRST`/`BEHAVIOUR_LAST` |
| Globals (`g`/`gp`/`gui` prefixes) | `glsTimedEvents`, `gpGDCamera` |

ALL-CAPS tokens also include null/boolean constants and typedef names such as
`NULL`, `FALSE`, `INT32`, `UINT16` and `UINT32`; spelling alone does not establish
a game enumerator.

Identifiers are Hungarian-coded, and that coding is established from the original's own text rather
than inferred:

| Prefix | Meaning implied by use |
| --- | --- |
| `p` | pointer |
| `ui` / `i` | `UINT32` / `INT32` (both names appear literally in casts) |
| `f` | flag |
| `b` / `ub` / `us` | byte, unsigned byte, `UINT16` |
| `g` / `gp` / `gui` | global, global pointer, global `UINT32` |
| `psr` | pointer to a SurRender object |
| `pls` | list-bearing pointer; both `PList.cpp` and `IList.cpp` use it, so the concrete list type needs consumer evidence |
| `pac` / `pst` / `h` | pointer to char array, pointer to struct, handle |

This narrows fields the disassembly leaves opaque, but it does not replace consumer evidence.
`pWorld->plsProps` is a `PList` because its users call the reviewed PList accessors; `psrMeshes`
identifies a SurRender-facing pointer independently.

An `m_` member prefix is **not** a project-wide convention. `m_` is
used by some classes, notably `Trigger`, the `Oct*` family, `GDFileIO`'s trigger arrays and `Item`'s
representation object, while most member accesses are plain Hungarian names. Treat `m_` as a
per-class habit to be checked, not as a rule to apply when naming a recovered field.

The paths also extend the tree. Register-indirect sites preserve paths such as
`Local Code\Gameloop.cpp` and `Engine Code\Cursor3d.cpp`. Header assertions can
preserve relative paths that an absolute-path scan cannot find:

```text
..\Engine Code\Include\AnimRep.hpp
..\Engine Code\Include\Trigger.hpp
..\Engine Code\Include\stHeap.hpp
..\Engine Code\Include\stLight.hpp
```

That establishes an `Engine Code\Include` directory and an `st*` family alongside the already-known
`stCube.cpp`. Inline code in headers is attributed to the header, not the including unit.

## How the original signals failure

Retail has C++ unwind frames but no `_CxxThrowException` import. Cleanup frames
do not prove an authored `try`/`catch` or literal compiler flags, and an absent
throw import does not rule out exceptions from called code. The ordinary failure
mechanisms below are established independently.

1. **Assertions, shipped enabled in retail.** Sites call SR.DLL's
   `srAssertFail`, and `InitializeVideoDevice` (`0x00422240`) installs `AssertFailureHandler`
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
   maps a null list to 0; `PLGet` maps null
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
   release build's log call retains no sink — while `WriteGameLog` (`0x0058AAD0`)
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
  modal branch: W8MessageDialogBase -> W8NotificationDialog
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
[modal base](../include/wiz8/dialog_code/MessageDialogBase.h) is only one inheritance branch.
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

## Receiver and storage evidence

A stack-cleaning return does not distinguish a free helper from a member. Establish where the
callee obtains its arguments and whether it consumes an incoming receiver; callback/export
contracts can independently establish the calling convention. Identical bodies likewise do not
establish common source ownership.

`W8Navigator::UpdateLinkedNavigator` (`0x00454d70`) operates on the navigator secondary
subobject at complete-object offset `+0x18`. Retail makes null-preserving `-0x18` conversions
before the monster queries. Its monster-link path has that concrete precondition; it does not
establish that arbitrary navigators are monsters or justify moving the method to Monster.

Packed file storage can also be runtime storage. NPC scripts decode presence slots into pointers
in the same allocation. Mesh loading expands compressed faces into `0x29`-byte faces and reads
uncompressed faces directly into that layout. A separate runtime record or removal of packing
requires independent allocation, stride and copy evidence.

Unknown blocks require consumer evidence for their complete extent before they become named
subrecords. Layout padding is valid storage; accesses that give it semantics require recovery at
the owning type. Neither a shared offset nor compiler reuse of a local slot establishes a union.

## Compiler-backed type gate

`uv run wiz8 lint` is the authoritative compiler-backed source-model gate. It
uses the same recovered source lists, includes, definitions, layouts and
per-source properties as the product build rather than maintaining a parallel
approximation. The concrete warning set and clang-tidy profile are configuration
owned by `cmake/Lint.cmake`, `cmake/CompileSettings.cmake`, and
`.clang-tidy`; do not copy those lists into documentation.

Intentional retail behavior that conflicts with a modern diagnostic is handled
at the narrow source site with evidence, not by weakening the whole lane or
inventing source constructs to satisfy the analyzer. Vendor source keeps its
separate warning policy.

The same compiler projection produces `build/source-index.json`, which is the
canonical machine-readable view of declarations, definitions, linkage and
header ownership used by repository gates.
Cross-target namespace separation belongs to that index/tooling layer; this
document owns only the source-model rules, not compile-database plumbing.

C++ declaration disagreements are source-model defects. The cross-TU gate
therefore compares compiler-derived declarations rather than maintaining a
second hand-parsed declaration model.

## Live recovery state

This document does not inventory current classes, layouts, match counts, or unresolved
lifecycle work. Those facts change with ordinary recovery and become harmful when copied into
prose.

Use the authoritative surfaces instead:

- C++ declarations, inheritance, `static_assert` layout checks, and function markers for the
  source-owned model;
- `uv run wiz8 ghidra decompile 0x<address>` / `ghidra sym 0x<address>` for native identity,
  ownership attachment, and current Ghidra evidence;
- `uv run wiz8 compare <addresses>` for relocation-masked body proof;
- reviewed claims under `evidence/` for why an accepted identity or layout is trusted.

Historical recovery examples belong in commit and Bead history, not in a manually maintained
snapshot here.
