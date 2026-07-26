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

## Assertion expressions

`SR.DLL`'s `srAssertFail` is called from 1048 sites, 1038 of which push their four arguments as
literals and decode cleanly into `config/analysis/wiz8/assertions.csv`: call site, containing
function, source path, line, and the **expression text**. They span 117 files and 606 distinct
functions.

The expression half is the valuable part, and it is a different kind of evidence from the source
path. A path assigns a function to a translation unit; an expression names identifiers:

| What it yields | Count | Examples |
| --- | ---: | --- |
| Member accesses through `->` or `.` | 195 | `pTrigger->m_pacRecipients`, `pWorld->plsProps`, `pWorld->psrMeshes`, `pSound->pacSoundName`, `pLVL->pProps[i].bNumFrames` |
| Named constants and enumerators | 65 | `BAD_INDEX`, `MAX_MONSTERS_IN_DATABASE`, `HAND_COUNT`, `SPELL_COUNT`, `PHASES_PER_ROUND`, `TRIGGER_REP_PROP`, `BEHAVIOUR_FIRST`/`BEHAVIOUR_LAST` |
| Globals | — | `glsTimedEvents`, `gpGDCamera` |

Two conventions are now established from the original's own text rather than inferred. Members carry
an `m_` prefix, and identifiers are Hungarian-coded:

| Prefix | Uses | Meaning implied by use |
| --- | ---: | --- |
| `p` | 480 | pointer |
| `ui` / `i` | 131 / 108 | `UINT32` / `INT32` (both names appear literally in casts) |
| `f` | 100 | flag |
| `b` / `ub` / `us` | 34 / 14 / 19 | byte, unsigned byte, `UINT16` |
| `g` / `gp` / `gui` | 33 / 16 / 14 | global, global pointer, global `UINT32` |
| `psr` | 24 | pointer to a SurRender object |
| `pls` | — | pointer to a `PList`, matching the already-reviewed `gXStatus.plsMonsterList` |
| `pac` / `pst` / `h` | 17 / 17 / 13 | pointer to char array, pointer to struct, handle |

This directly types fields the disassembly leaves opaque: `pWorld->plsProps` and `pWorld->psrMeshes`
name two `W8World` members and say one is a `PList` and the other a SurRender object.

The paths also extend the tree. All 113 absolute `.cpp` assertion paths already appear in
`source-tree.csv`, which independently confirms that census is complete for `.cpp`. But four
assertions come from headers the absolute-path scan could never have found, because they are
recorded relative:

```text
..\Engine Code\Include\AnimRep.hpp
..\Engine Code\Include\Trigger.hpp
..\Engine Code\Include\stHeap.hpp
..\Engine Code\Include\stLight.hpp
```

That establishes an `Engine Code\Include` directory and an `st*` family alongside the already-known
`stCube.cpp`. Inline code in headers is attributed to the header, not the including unit.

## Turning an assertion into a proven field

Assertions name fields; they do not place them. The matching build closes that gap, because a wrong
offset does not compile to identical bytes.

`Engine Code\3d.cpp:344` asserts `pWorld && pWorld->plsProps`. Its owning function at `0x0046DED0`
tests `[edi+8]` and then passes it to the `PList` count and element accessors, which places the
member at `0x08` and corroborates the `pls` prefix. Porting that function as `WorldUpdateProps`
reproduces all 117 bytes exactly, so `W8World::plsProps` is now a **proven** field rather than an
inferred one. `psrMeshes` is placed at `0x48` by the same method from
`Engine Code\3dapi.cpp:446`, but has no ported consumer yet and is therefore recorded as
unproven.

`config/analysis/wiz8/classes.csv` carries a `layout_proof` column for exactly this distinction.

## `Octree`: a class named and laid out entirely by its own assertions

`Engine Code\Octree.cpp` asserts `m_usNumPropsLoaded<(UINT16)m_ulNumProps` at line 1157 and
`m_usNumParticlesLoaded<(UINT16)m_ulNumParticles` at line 1181. Unusually, both calls also pass a
**message**, and the messages name the class outright: *"Too many props loaded for Octree"* and
*"Too many particles loaded for Octree"*.

So the assertions supply four member names and their types — the `us`/`ul` prefixes and the explicit
`(UINT16)` casts say the loaded counters are 16-bit and the totals 32-bit — while the two asserting
bodies supply the offsets. Porting both as `Octree::AddLoadedProp` and `Octree::AddLoadedParticle`
reproduces 118/118 bytes each on the first build, which proves all seven fields at once:

| Offset | Field | Type |
| --- | --- | --- |
| `0x0C4` | gate flag, name unknown | `unsigned char` |
| `0x0F8` | `m_ulNumParticles` | `UINT32` |
| `0x114` | props array, name unknown | pointer array |
| `0x118` | particles array, name unknown | pointer array |
| `0x11C` | `m_usNumPropsLoaded` | `UINT16` |
| `0x11E` | `m_usNumParticlesLoaded` | `UINT16` |
| `0x188` | `m_ulNumProps` | `UINT32` |

`Octree` is therefore the first class in `classes.csv` whose layout is byte-proven rather than
inferred. It has no identified vtable, constructor or destructor yet, so its recorded minimum size is
only the extent the proven fields require, not a claim about the object's real size.

## `W8TargetSource`: a sentinel and a field order, both proven

`Local Code\Targeting.cpp` asserts `pSource->iChar != BAD_INDEX` (3299),
`pSource->iMonsterID != BAD_INDEX` (3320) and `pSource->fBackfire || pSource->fReflection`
(3307, 3328). The asserting bodies compare against `-1`, which pins **`BAD_INDEX = -1`**.

Porting the pair at `0x0053BEA0` and `0x0053BF10` reproduces 110/110 bytes each on the first build.
That proves `iType`, `iChar` and `iMonsterID` at `0x00`, `0x04` and `0x08` — and, more interestingly,
it settles which flag is which. A short-circuit `||` evaluates its left operand first, so the byte
order of the two flag tests decides the assignment: `fBackfire` is at `0x1C` and `fReflection` at
`0x1B`. Swapping them would emit the tests in the other order and the bodies would not match. This
is a field *ordering* recovered by falsification, which no amount of static reading would settle.

The `iType` discriminant reads 1 for the character, 2 for the monster and 3 for either, which is why
only the type-3 path additionally requires one of the two flags to be set.

## `Monster`: the first vtable-recovered class with a proven layout

The class was originally reviewed from its constructor, vtables and source path, which established
*that* it holds three adjacent `0x1B0`-byte subobject arrays at `0xAC`, `0x25C` and `0x40C` but not
what they contain.

`Engine Code\Monster.cpp:960` closes that. Its assertion expression is `bCycle < CYCLE_NUM_UNIQUE`
and its message is *"GetNumSubsPerCycle() -> Invalid cycle num."* — which names the method outright.
The body bounds-checks against `27`, substitutes a field at `0xA4` for the `-1` sentinel, and reads a
byte at `this + (bCycle + 11) * 16`.

Porting it reproduces 70/70 bytes, proving `CYCLE_NUM_UNIQUE = 27`, the current-cycle field at
`0xA4`, and a 27-entry array of `0x10`-byte elements at `0xAC` whose byte at `+4` is the sub count.
`0xAC + 27 * 0x10 = 0x25C` — exactly where the constructor starts the second array, so the two
independent readings agree.

## `MonsterInfoDialog`: offsets without names

`Dialog Code\MonsterInfoDialog.cpp` contains **no assertions at all**, so the whole
expression-mining route is unavailable and no member name is recoverable from it. Its layout is
still provable, because the matching build does not need names.

Two of its primary vtable slots are small enough to port directly. Slot 12 at `0x005D6E60` is
twelve bytes with no relocations and clears a byte at `0x41` when a byte at `0x50` is set. Slot 2 at
`0x005DBDE0` is twenty bytes and calls a method on the subobject at `0x58` — the first of the three
the reviewed complete destructor tears down. Both reproduce exactly, and both also match in the demo.

The fields therefore keep positional names. This is the honest split: offsets are evidence, names
are not, and a class can have one without the other.

## `GrCycle` is abstract

Eight of `GrCycle`'s sixteen primary slots — 5 through 9, 12, 13 and 15 — point at a single thunk
that jumps to MSVCRT's `_purecall`. They are pure virtual, so `GrCycle` is an abstract base. This
also corroborates `SetBehaviour`, which calls its own slot 9: a non-virtual method of an abstract
base calling a pure virtual its concrete derived class implements.

## Named bases from imported SurRender vftables

`Wiz8.exe` imports 461 decorated C++ symbols from `SR.DLL`, covering 49 classes, 28 of which import
both a constructor and a destructor. Six of those imports are **vftables**, and every one is
referenced from `.text`. That is the strongest class evidence in the executable, because a function
installing an imported base vptr is provably constructing a class derived from a *named* base, and
the mangled name spells out the inheritance:

```text
??_7srLight@@6BsrVertexProcessor@@@                              → srLight, in base srVertexProcessor
??_7srLight@@6B?$srClassSupport@VsrIlluminator@@VsrNode@@$0A@$0BCAA@@@@
??_7srBinIStream@@6B0@@                                          → srBinIStream, in itself
??_7srBinIStream@@6BsrBinStream@@@                               → srBinIStream, in base srBinStream
??_7srBinStream@@6B@
??_7srMaterial@@6B@
```

Thirteen distinct functions install one of these. Most of the `srMaterial` sites are large functions
that construct a stack temporary rather than dedicated constructors; two sites are unambiguous
derived-class constructors and are the first two classes below.

`srClassSupport<srIlluminator, srNode, 0, 0x1200>` looks like SurRender's class-ID registry — the
JPEG extension's `getClassID` returns `0x3110` — which would make `0x1200` `srLight`'s class ID.
That is not yet confirmed against a `getClassID` body.

## `VirtualFileBinIStream`

`0x0047CBD0` is a two-argument `__thiscall` constructor for a **virtually inherited** stream class:

* `[this+4]` is a vbptr selecting the vbtable at `0x005EC6A8`, whose entries `-4` and `12` place the
  virtual `srBinStream` subobject at `this+0x10`. The constructor's `lea ecx, [esi+0x10]` base call
  confirms the same offset independently.
* It installs the two imported `srBinIStream` vftables, then overwrites both with its own
  `0x005EC6A0` (primary) and `0x005EC68C` (virtual base, 7 slots).
* Its sole caller allocates exactly `0x20` bytes.
* It stores an `OpenVirtualFile` (`0x00404C80`) handle at `this+8`.

So this is Wizardry's adapter binding the SurRender stream hierarchy to the SLF virtual file system:
SurRender reads assets through it without knowing archives exist.

## `MonsterLight`

`0x0049D660` is a copy constructor for a `0x250`-byte class derived from `srLight`. It calls the
`srLight` base constructor, installs local vtables `0x005ECD18` (13 slots) at `+0` and `0x005ECD0C`
(16 slots) at `+0x138`, deep-copies state through offset `0x249`, and reinstalls the imported
`srLight` base vftables. The `0x249` high-water mark and the `0x250` allocation agree.

Its only caller is inside `Monster`'s copy constructor at `0x004BEBD0`, copying `Monster+0x624` —
which is exactly the last field `Monster`'s own constructor initializes. So `Monster` owns a pointer
to a light object, and the two independently reviewed classes join up. The name is descriptive; the
base name `srLight` is `original-export`.

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
