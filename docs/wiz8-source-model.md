# Wizardry executable source and class model

## Translation-unit tree

Raw strings in the main executables preserve 149 distinct absolute Wizardry source paths. The
canonical GOG program contributes 136 paths rooted at `C:\Projects\Wizardry 8`; the demo uses
`E:\Wizardry 8` and adds 13 units not present in the canonical release strings. The reviewed tree
is tracked in `config/analysis/wiz8/source-tree.csv` with exact absolute spellings and per-build
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

## First reviewed class: `MonsterInfoDialog`

The first local class model is grounded by four independent observations:

* `0x005D5E30` constructs three embedded members, retains its second argument at offset `0x54`,
  writes vtable `0x005EF910`, and loads `Data\Dialogs\popup_monsterinfo.sti`;
* vtable slot 3 at `0x005D6080` directly references
  `C:\Projects\Wizardry 8\Dialog Code\MonsterInfoDialog.cpp`;
* `0x005D5EE0` is the scalar deleting destructor: it calls `0x005D5F00` and conditionally invokes
  operator delete;
* `0x005D5F00` restores the same vtable and destroys the subobjects beginning at `0x58`, `0xA4`,
  and `0xEC` in reverse order.

The reviewed minimum object extent is `0x130`: methods access byte `0x129`, while the exact final
size is not yet claimed. Unknown member and base storage remains explicitly opaque in the Ghidra
type instead of receiving semantic field names prematurely.

The vtable has 14 slots. Seven slot targets missed by initial auto-analysis were created at
`0x005D5F90`, `0x005D6FA0`, `0x005DCCE0`, `0x005B1BE0`, `0x005AD270`, `0x005D6E60`, and
`0x005D6E70`. Only the constructor, destructor, and compiler-generated deleting destructor are
named so far; the other virtual methods remain unnamed until their behavior is reviewed.

## `Monster`

The second reviewed class is the engine-level `Monster` object:

* `0x004BEA20` constructs three adjacent `0x1B0`-byte subobject arrays at offsets `0xAC`,
  `0x25C`, and `0x40C`, initializes fields through offset `0x624`, and finally writes vtable
  `0x005ED200`;
* `0x004BEBD0` is a copy constructor that rebuilds the same three arrays, copies state from its
  source object, and installs the same vtable;
* vtable slot 0 at `0x004BEBA0` is a scalar deleting destructor that calls the complete destructor
  at `0x004BEE50` before conditionally invoking operator delete;
* virtual slots 5, 12, and 26 directly reference
  `C:\Projects\Wizardry 8\Engine Code\Monster.cpp`.

The reviewed minimum extent is `0x628`, based on constructor accesses through offset `0x624`.
The three repeated regions contain 27 adjacent `0x10`-byte elements each, but their element type
and the remaining fields are intentionally unnamed pending use-site review. The vtable has 31
slots; missing slot targets are created during model replay, but no behavioral method names are
assigned from slot position alone.

## `GrCycle`

`GrCycle` is a `0x1D8`-byte graphics-cycle object with two polymorphic bases or interfaces. Its
default constructor at `0x004A5E50` constructs the primary base at offset zero and a secondary
subobject at offset `0x18`, then installs vtables `0x005ECE78` and `0x005ECEB8`. The copy
constructor at `0x004A5F20` repeats that layout while rebuilding owned graphics state. The
complete destructor at `0x004A6610` restores both vtables before releasing the owned cycle data
and destroying both subobjects; primary slot zero at `0x004A5F00` is its scalar deleting
destructor.

The primary table ends at the independently installed secondary table, giving 16 and 13 slots
respectively. This corrects the misleading 29-entry maximal pointer run produced by treating both
adjacent tables as one. Primary slots 4 (`0x004A7470`) and 11 (`0x004A7E10`) directly reference
`C:\Projects\Wizardry 8\Engine Code\GrCycle.cpp`. Unknown base and member storage remains opaque.

Replay the tracked model with:

```sh
just ghidra apply-functions wiz8--gog-base--wiz8--18a74ff61c65 \
  --map config/analysis/functions/wiz8-classes.csv
just ghidra apply-wiz8-class-model wiz8--gog-base--wiz8--18a74ff61c65
```

The authoritative evidence rows are `config/analysis/wiz8/classes.csv` and
`config/analysis/functions/wiz8-classes.csv`.
