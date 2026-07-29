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
program, and the cross-build raw harvest behind this table is
`evidence/snapshots/call-sites/assertions.csv`.

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

Type `layout-supported` claims in `evidence/reviewed/wiz8/claims.csv` carry this distinction without duplicating Ghidra's current layout.

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

`Octree` is therefore the first class whose layout claim is byte-proven rather than
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

## The class registry, and `stLight`

SurRender classes carry a registry pair: a virtual `getClassName` returning a literal string and a
virtual `getClassID` returning a constant. Reading those two slots identifies a class without any
RTTI at all.

That settles a question left open earlier. The imported `srLight` vftable mangles as
`srClassSupport<srIlluminator, srNode, 0, 0x1200>`, and `0x1200` was only *suspected* to be a class
ID. The sibling getters confirm it: the class at vtable `0x005ECD18` returns `0x1220`, and a
different class returns `0x10006`. Wizardry-registered classes use IDs from `0x10000` up, while
SurRender's own sit near `0x1200`–`0x3110`.

Two separate classes live in this area, which is worth being careful about:

* vtable `0x005ECD18` — `getClassName` returns **`"srLight"`** and `getClassID` returns `0x1220`. It
  presents itself to the scene graph as an `srLight` variant rather than under a Wizardry name, so
  the repository keeps the descriptive name `MonsterLight` for it. Its scalar deleting destructor is
  `0x0049E0A0` and its complete destructor `0x0049E0D0`.
* vtables `0x005ECC64` and `0x005ECCA4` — `getClassName` returns **`"stLight"`** and `getClassID`
  returns `0x10006`. This is a genuine original class name, and it has *two unrelated origins*: the
  runtime registry string, and the relative assertion path `..\Engine Code\Include\stLight.hpp`
  recovered by the assertion harvest. Its vtables are installed by eight distinct functions,
  including `GrCycle`'s copy constructor at `0x004A5F20`, so `GrCycle` owns or embeds one. The
  source-owned lifecycle now models `stLight` as an `srLight` subclass; the recovered SurRender ABI
  establishes `srLight`'s `srIlluminator` and `srVertexProcessor` bases. That source and lifecycle
  proof—not adjacency or the raw store displacements—licenses the secondary-base interpretation.

## The SurRender ABI surface `Wiz8.exe` consumes

`evidence/observations/surrender/wiz8-sr-imports.csv` records all **461** `SR.DLL` imports with their exact
demangled signatures, calling conventions, IAT addresses and kinds. Every one demangles; the class is
derived from the demangled text rather than the mangling, which is what correctly separates nested
classes from operators and free functions.

Nine of the 461 are **not** C++-decorated: the free functions `srInit`, `srExit`, `srAssertFail` and
`srAssertSetFunc`, and the global objects `srCore`, `srHeap`, `srConfig`, `srBoxFilter` and
`srBSplineFilter`. So 452 are decorated C++ symbols and 9 are plain C.

| Kind | Count |
| --- | ---: |
| method | 371 |
| constructor / destructor | 29 / 28 |
| operator | 16 |
| vftable | 6 |
| global object | 5 |
| free function | 4 |
| vbase destructor | 2 |

Class counts depend on a rule that has to be stated, because nested classes make two answers both
correct:

| Counting rule | Distinct classes | Classes importing both a constructor and a destructor |
| --- | ---: | ---: |
| Full nested name (`srHuffman::Sampler`) | 51 | 24 |
| Outer class only (`srHuffman`) | 43 | 20 |

Nine classes are nested: `srGERD::Renderer`, `srModel::Client`, `srModeler::Polygon`,
`srModeler::Vertex`, and a complete Huffman codec in
`srHuffman::{BitIStream, BitOStream, Compressor, Decompressor, Sampler}`. `srHuffman` itself imports
nothing directly, which is why collapsing nine nested names removes only eight classes.

A ctor/dtor count also depends on whether the two `??_D` vbase destructors
(`srBinIMStream`, `srBinOMStream`) count as destructors; the table above keeps them in their own
`vbase destructor` kind, so they do not.

The largest surfaces, counting by outer class, are `srGERD` (84 members), `srMeshModel` (43),
`srNode` (42), `srColorSurface` (34), `srTexture` (24), `srColorSurfaceIFace` (18) and `srCamera`
(16). `srGERD` is 82 under the full-name rule, the two-member difference being `srGERD::Renderer`.

The free functions and globals are worth naming explicitly, because the repository already depends
on one of them: `srAssertFail` and `srAssertSetFunc`, and the global objects `srCore`, `srHeap`,
`srConfig`, `srBoxFilter` and `srBSplineFilter`. Every assertion mined above is a call into that
first export.

This joins the existing JPEG-side model on `decorated_name`. 62 of that file's 69 symbols also
appear here, and the two disagree on **zero** demangled signatures — the hand-recorded JPEG
signatures and `llvm-undname` produce identical text on every shared symbol, which independently
validates both artifacts. The union is 468 symbols of exact original ABI.

## Named bases from imported SurRender vftables

`Wiz8.exe` imports 461 symbols from `SR.DLL` — 452 decorated C++ and 9 plain C — covering 43 classes
by outer name or 51 counting nested classes separately, as tabulated above. Six of those imports are
**vftables**, and every one is
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

Fifteen reference sites in fourteen distinct functions install one of these; the disposable Ghidra
vtable-reference index reports every site and its classification. An earlier
revision guessed that most `srMaterial` sites were stack temporaries: they are not. Seven are
**inlined heap constructions** — operator new with a null check, the imported vftable,
`srMaterial::reset`, then a local derived vtable (`0x005EBDE0`) — and two are dedicated
constructors of a second derived class that registers itself with `srRegistry` and installs
`0x005ECB6C`/`0x005ECB38`. The one genuine stack temporary is an `srBinStream` in the archive
reader at `0x0043AEC0`, and the once-orphaned site at `0x0047DA2F` turned out to be a 53-byte
vbase-adjusting scalar deleting destructor that restores the imported `srBinStream` vftable. The
remaining four sites are the already-reviewed `VirtualFileBinIStream` and `MonsterLight`
constructors.

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
(3 slots, per the corrected boundary discipline in `vtables.csv`) at `+0x138`, deep-copies state
through offset `0x249`, and reinstalls the imported
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

The reviewed minimum object extent is now `0x144`. The exact constructor at `0x005D14D0` writes
through its own offset `+0x56`; because that member begins at dialog offset `+0xEC`, the previous
`0x130` minimum stopped twelve bytes too early. The exact final allocation size is still not
claimed. Unknown member and base storage remains positional instead of receiving semantic field
names prematurely.

The construction sequence has now been split correctly. `0x005DC7A0` constructs the sole base at
offset zero; its extent is `0x54`, where the derived constructor stores its argument. The calls at
`0x005E0C40`, `0x005DB1B0`, and `0x005D14D0` then construct three **members** at `+0x58`, `+0xA4`,
and `+0xEC`. The destructor tears those members down in reverse before calling the base destructor
at `0x005DC860`. No checked source supplies their names, so `include/wiz8/monster_info_dialog.h`
uses constructor-address-qualified positional types. The first member constructor is ported with
all established field values; its remaining code-generation difference is the register-heavy
initialization of the nested `0x10`-byte region.

The other two member lifetime pairs are now byte-exact. `0x005DB1B0`/`0x005DB260` prove the
`0x48`-byte polymorphic member at `+0xA4`, including separately released resource handles at
member offsets `+0x18` and `+0x1C`. `0x005D14D0`/`0x005D1590` prove an aligned minimum of `0x58`
bytes for the final member at `+0xEC`, including two embedded `0x10`-byte pointer-vector
specializations at member offsets
`+0x1C` and `+0x2C`. The destructor removes and virtually deletes the first vector's entries before
both backing arrays are released.

That typed composition also emits three adjacent pointer-vector lifetime bodies byte-exactly:
the complete destructor at `0x005D2540`, the suffix-sharing base scalar deleting destructor at
`0x005D2560`, and the derived scalar deleting destructor at `0x005D2590`. Vtables `0x005EF898`
and `0x005EF89C` are independently installed at the same subobject address, so each is a one-slot
table; treating them as one two-entry run would repeat the adjacent-vtable counting error already
seen elsewhere. The base subobject is not a container of its own: it is
`W8GrowableVector<W8DialogOwned005D14D0*>`, one instantiation of the template in
`include/wiz8/vector.h`, and `W8DialogPtrVector005EF898` is the derived layer that adds a second
vtable and no storage.

The vtable has 14 slots. Seven slot targets missed by initial auto-analysis were created at
`0x005D5F90`, `0x005D6FA0`, `0x005DCCE0`, `0x005B1BE0`, `0x005AD270`, `0x005D6E60`, and
`0x005D6E70`. Only the constructor, destructor, and compiler-generated deleting destructor are
named so far; the other virtual methods remain unnamed until their behavior is reviewed.

## `W8Monster` and `W8MonsterRep`

The object constructed at `0x004BEA20` is `W8MonsterRep`, the animation object stored in
`W8Monster::m_pRep`:

* `0x004BEA20` constructs three adjacent `0x1B0`-byte subobject arrays at offsets `0xAC`,
  `0x25C`, and `0x40C`, initializes fields through offset `0x624`, and finally writes vtable
  `0x005ED200`;
* `0x004BEBD0` is a copy constructor that rebuilds the same three arrays, copies state from its
  source object, and installs the same vtable;
* vtable slot 0 at `0x004BEBA0` is a scalar deleting destructor that calls the complete destructor
  at `0x004BEE50` before conditionally invoking operator delete;
* virtual slots 5, 12, and 26 directly reference
  `C:\Projects\Wizardry 8\Engine Code\Monster.cpp`.

Its reviewed minimum extent is `0x628`, based on constructor accesses through offset `0x624`.
The three repeated regions contain 27 adjacent `0x10`-byte elements each, but their element type
and the remaining fields are intentionally unnamed pending use-site review. The vtable has 31
slots; the reviewed Ghidra project defines the slot targets, but no behavioral method names are
assigned from slot position alone.

The concrete world object is the `0x348`-byte `W8Monster : W8GrCycle` constructed at
`0x004BFB00` and destroyed at `0x004C0170`. Its `m_pRep` member points to the separately allocated
`W8MonsterRep`. The five-slot vftable `0x005ED218` is the inherited secondary GrCycle base at
`W8Monster+0x18`; it is not part of `W8MonsterRep`. The positional
`W8MonsterPolymorphicSubobject18` view remains useful for forwarded field accesses inside
`W8MonsterRep`, but it is neither a base nor an embedded polymorphic object.

## `GrCycle`

`GrCycle` is a `0x1D8`-byte graphics-cycle object with two polymorphic bases. Its
default constructor at `0x004A5E50` constructs the primary base at offset zero and a secondary
subobject at offset `0x18`, then installs vtables `0x005ECE78` and `0x005ECEB8`. The copy
constructor at `0x004A5F20` repeats that layout while rebuilding owned graphics state. The
complete destructor at `0x004A6610` restores both vtables before releasing the owned cycle data
and destroying both bases; primary slot zero at `0x004A5F00` is its scalar deleting destructor.
That complete constructor/copy/destructor family is what proves multiple inheritance; adjacency
and the raw `+0x18` store displacement alone would not.

The primary table ends at the independently installed secondary table, giving 16 slots. The
secondary table has **five**, not thirteen: the copy constructor writes `0x005ECECC`, the next
pointer-sized address, into a separate embedded container at `0x004A63FC` and `0x004A6407`.
Treating the following eight one-entry/container vtables as secondary slots repeated the same
maximal-pointer-run error that originally joined the primary and secondary tables.

The `+0x18` table is a base vtable, not a polymorphic member. Its first slot, `0x004A9100`,
subtracts `0x18` from `this` and jumps to the complete object's scalar-deleting destructor at
`0x004A5F00`. The constructor first calls `0x00451EC0` on `this+0x18`, then replaces that
subobject's `0x005EC2D0` vptr with `0x005ECEB8`; the complete destructor calls `0x00452120` on
the same `this+0x18` subobject after the GrCycle fields are released. Those are the VC6 multiple-
inheritance constructor, adjustor-thunk, and reverse-destruction patterns. The concrete
`W8Monster` table repeats the proof: `0x004CAE30` subtracts `0x18` and jumps to `0x004BFDE0`.

The constructor also settles the storage boundary. The base constructed by `0x004B6900` occupies
`0x18` bytes. The base constructed by `0x00451EC0` at `+0x18` occupies `0x190` bytes, because the
first derived initialization is at `+0x1A8`; `GrCycle` then owns the final `0x30` bytes. No checked
source supplies either base name, so the public header uses constructor-address-qualified names
rather than semantic guesses. Primary slots 4 (`0x004A7470`) and 11 (`0x004A7E10`) directly
reference `C:\Projects\Wizardry 8\Engine Code\GrCycle.cpp`.

The concrete `W8Monster` GrCycle is now modelled directly. The GrCycle factory allocates `0x348`
bytes and calls its default constructor at `0x004BFB00`; the copy constructor at `0x004BFE00`
and complete destructor at `0x004C0170` install primary vtable `0x005ED22C` and the inherited
secondary vtable `0x005ED218`. Its members are a `W8MonsterRep*`, byte and integer growable
vectors, three timer objects, and the SurRender-heap vector. Its destructor releases those and
then runs the `GrCycle` base destructor. Primary slot zero at `0x004BFDE0` is compiler-generated
and is represented only by a `SYNTHETIC` marker.

Inspect the reviewed model and export its disposable index with:

```sh
uv run wiz8 ghidra restore
uv run wiz8 ghidra index
```

Authoritative class relationships and owned lifecycle identities come from the C++ declarations;
the Ghidra index retains original-only analysis until it is promoted.
