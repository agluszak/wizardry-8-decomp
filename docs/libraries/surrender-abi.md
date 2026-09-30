# The SurRender boundary

Recover each executable and DLL under its own reccmp target. Preserve the original DLL import/export
boundaries. Share canonical C++ declarations and genuinely shared source; do not create duplicate
C-compatible models for DLL consumers. Attribute each emitted function, global, and vtable to the
binary containing it. Thus SurRender bodies belong to `SURRENDER`, while Wizardry call sites that
invoke SurRender belong to `WIZ8`. This document covers the shared declaration surface and the
evidence that establishes it.

## Consumer ABI and provider recovery are independent

Consumer import libraries describe the **original provider**, even when that function has a
recovered implementation. Wiz8, JPEG and ZIP must not link against the partial `SURRENDER` target.
The original has 2059 exports; the recovered comparison DLL has only a small subset. Its DEF is not
its complete export list: member `dllexport` declarations contribute exports too.

Use these distinctions without adding another target inventory:

| Family | Recovered boundary | Recovered build product |
| --- | --- | --- |
| Wiz8 → SR / MSS / Bink | Static consumer declarations and retail import libraries | Partial Wiz8 comparison/runtime targets |
| JPEG / ZIP | Extension entrypoints, `srPlugin`, retail SR imports | Both extension DLLs; not a complete reconstructed runtime |
| DirectX7 | Six driver entrypoint types in `srDD.h`, checked against `srGERD` | No recovered driver target |
| Generic VP | Three entrypoint types in `srVectorProcessor.h`, loader and ownership paths | No recovered backend target |
| Other known srDD / srVP / srEXT binaries | Evidence targets; family resemblance is not complete ABI recovery | No recovered products |
| SR provider | Incrementally recovered classes and module loaders | Comparison DLL, not a replacement for retail SR |

Presence in `reccmp-project.yml` establishes a known binary, not a recovered build product,
usable replacement, import library, or tested runtime load.

### Provider recovery workflow

`SURRENDER` is its own comparison target and the canonical Ghidra program is the retail `sr.dll`.
Do not recover provider bodies through WIZ8's default ProgramDB. The normal focused loop is:

```sh
uv run wiz8 build SURRENDER
uv run wiz8 ghidra decompile 0x1003bee0 --program sr.dll
uv run wiz8 compare 0x1003bee0 --program sr.dll
```

The commands infer `SURRENDER` from `--program sr.dll`. The comparison DLL remains non-runnable
and may retain unresolved provider internals while recovery is incomplete.

The recovered extension and VP loaders deliberately retain their calls to unrecovered
`srConfig::get`, the `srConfig` global, `srDebugPrintf`, and `srStreamPrintf`. Consequently
`SURRENDER` uses `/FORCE:UNRESOLVED` explicitly as a comparison image. Those unresolved calls
are not stubs or retail self-imports. Do not deploy this image; recovering the configuration,
heap/index ownership and logging dependency closure remains necessary before runtime use.

## Module ABI spine

`src/surrender/extension.cpp` recovers the eleven authored `srExtension` bodies at
`0x10013840`–`0x10013AF0`; assignment at `0x10013D50` remains a compiler-generated shallow copy,
not an invented ownership operation. The loader rejects empty names, reuses an already-loaded
case-sensitive name, obtains an optional path from `DLL_PATH`, constructs `srEXT_%s`, checks
compatibility through `srDynamicLibrary`, resolves `srInitPlugin`, and retains plugin and module
together. Destruction deletes the plugin **before** freeing its DLL. Failure paths retain retail
logging and unload behavior; there is no invented allocation-failure guard after extension creation.

Every available srEXT binary exports `srGetLibraryVersion` at ordinal 1 and `srInitPlugin` at 2;
their version getters return `0x012A0209`. Both functions take zero arguments. JPEG and ZIP
therefore do not settle an original cdecl/stdcall spelling: both conventions produce plain `RET`
for these x86 calls. `srPlugin.h` records both typed spellings, the loader uses the cdecl form,
and ZIP's existing stdcall definitions are retained rather than relabeled as proven cdecl.
The shared plugin interface remains deleting destructor followed by `getDescription() const`.

DirectX7 exposes `srDDGetDriverApiVersion` (`0x10001680`, value `0x128`),
`srDDGetDeviceCount` (`0x10001690`), `srDDConfigureDriver` (`0x10001700`),
`srDDGetDriverName` (`0x10001900`, `DirectX7`), `srDDGetDeviceName` (`0x10001910`), and
`srDDInitDevice` (`0x10001AA0`). `srGERD::loadDeviceWithFileName` at `0x10018870` resolves all six,
requires API >= `0x128`, supplies the uppercased `DD_<driver>` configuration, checks the device
index, and retains the initialized `srDD` with its module. The argument-bearing calls prove cdecl
cleanup; the indices are scalar words, not Ghidra's initially inferred string pointers.
`srDD.h` does not invent the still-unrecovered device vtable or layout.

Generic VP exports API (`0x10001000`, value `0x119`), ID (`0x10001010`, value 0), and initialization
(`0x10001020`, a `0x440` allocation). This is not an `srPlugin` or `srClassSupport` factory.
The recovered provider-side `srVectorProcessor` loads the named file, resolves API and factory,
requires API >= `0x119`, installs the resulting `srVP`, and retains its module. ID probing loads
and frees a separate reference. Release deletes the owned base/debug processors before unloading.
The full Generic implementation, base initialization, best-backend selection, and debug wrapper
remain unrecovered; no empty replacement backend is built.

## Declaration visibility and client construction

Apply import/export and inline attributes where evidence reaches, not with a global import bypass.
ZIP's `srZipOpener::open` at `0x1001080C` reads the `srStringTable` count directly from object+8;
`getCount()` is header-visible while retaining its exported out-of-line identity at
`0x10003A60`. Its unnecessary ZIP import is gone. `srSystem` uses member-level visibility because
class-wide export generated an assignment export absent from retail. Other class-wide declarations
are not mechanically rewritten without comparable evidence.

All available consumer binaries were checked for retained support-template names, imports and
debug records. AVI (`0x100013EE`), default (`0x1000125E`) and FLIC (`0x100014F2`) call imported
`srColorSurface` constructors, install client-local tables, and expose class ID `0x3110`, matching
the self-support pattern in JPEG and Wiz8. LWO's texture construction at `0x10002E06` likewise
installs a local table with ID `0x2112`. The driver and VP families supply no comparable
`srClassSupport` import evidence; their factories must not be generalized to that template.

None of the retained names or NB10 PDB references establishes an original SDK macro, nested alias,
or declaration macro spelling. `SR_NEW` and its `ClientType` implementation remain explicitly
provisional. Application variables/containers use the canonical class pointer, with `ClientType`
confined to the construction interface. No speculative `SR_CLASS` macro is introduced.

## Import-name verification

The existing build path checks JPEG/ZIP/MSS/Bink DEFs **and generated import-library names** against
the original consumer import table, and checks names against the original provider exports.
Unused imports need not be linked into a partial consumer: MSS has 56 library names but currently
54 linked names; Bink has 12/12, JPEG 69/69, and ZIP 25/25. ZIP's former 27 unused DEF entries were
removed. MSS's retail `_AIL_init_sample@` is explicitly normalized to the provider-exported
`_AIL_init_sample@4`; the malformed spelling is not treated as authoritative callable ABI.
Wiz8's larger SR boundary is checked for provider-supported linked names, not forced to equal the
retail consumer's set while recovery is incomplete.

For `srAssertFail`, the generated long-form COFF import member owns the fixed-arity caller IAT
symbol `__imp_?srAssertFail@@YAXPBD0J0@Z`, while its hint/name section names the provider's variadic
`?srAssertFail@@YAXPBD0J0ZZ`. It has no code section. LIB combines this member with the ordinary
SR descriptor/terminators, preserving DLL grouping; a plain DEF alias did not express this mapping.
The final PE check confirms the real provider spelling without changing the load-bearing caller
declaration or introducing a thunk wrapper.

## What the export table already settles

`sr.dll` exports 2059 decorated symbols and Wizardry imports 461 of them across 51 classes. Names,
signatures, calling conventions, access and virtuality are all read out of the export table by
`wiz8 evidence refresh surrender-abi`, decoded through `llvm-undname` rather than a local demangler.

Sixty-five of those exports are vftables and fifteen are vbtables. A table export names a *data*
address, so the table itself can be read - which is why `evidence/snapshots/surrender-abi/` carries
`vftable-slots.csv` and `vbtable-entries.csv` alongside `exports.csv`. Nothing there disassembles a
SurRender body: a vftable's slots are bounded by the relocation directory and the executable
sections, and a vbtable's entries by the absence of relocations.

That gives virtual slot *order*, which the export table alone never states, for the 24 classes
Wizardry uses that export a vftable. It also distinguishes an implemented virtual from a pure one:
the pure slots of a module share a single internal target, so a class whose slot points elsewhere
implements that method. `srBinStream` is the worked example - five slots, of which 2 to 4 share the
stub while slot 1 holds a real `getSize`, correcting a header that had marked `getSize` pure.

The three `sr.dll` builds in the corpus - the demo and two GOG releases - have different bytes and
nothing forces them to agree on a table's length. They agree on all sixty-five vftables and all
fifteen vbtables, which catches a decode that drifts. It does not catch one that is wrong the same
way in every build, and the srMaterial case below is exactly that, so the agreement is a guard
against instability rather than proof of a boundary.

## Wizardry relevance, not export-table completeness

The criterion for recovering a SurRender type is whether it illuminates Wizardry, not whether
`sr.dll` exports it. IAT xrefs from `evidence/observations/surrender/wiz8-sr-imports.csv` decide:
every corresponding IAT address is queried for xrefs, each xref is resolved to its containing
Wiz8 function, and imported data such as `srVectorProcessor::vp` is followed through
`load vp; CALL [vtable+offset]`. A class with no path into Wiz8 after that census stays
provider-only.

`srARGB` was the important mis-model: the header had only `e_index` and therefore `sizeof` 1, while
palette APIs take `srARGB*` as a color array. The SR bodies settle a 4-byte packed value, not floats:

- `srPalette::getColor` at `0x100048a0` loads `colors[index]` as a dword (`index * 4`) and stores it
  through the hidden return pointer (`RET 8`);
- `setColor` / `setColors` copy the same 4-byte stride;
- `Sampler::shiftDown` at `0x100062a0` shifts all four bytes;
- `Sampler::addColor` forces byte 3 to `0xff` (opaque alpha) and hashes bytes 0-2;
- `Quantizer::quantize` and `Optimizer::setupLUT` consume bytes 2, 1, 0 as R, G, B.

Memory order is therefore B, G, R, A (little-endian `0xAARRGGBB`). `e_index` is the logical ARGB
channel. `getChannelStatistics` at `0x10059240` reads byte `(3 - channel)` of each packed pixel, so
`INDEX_ALPHA` is offset 3 and `INDEX_BLUE` is offset 0. That function also fills `srStat`: sample
count at `+0x00`, unused alignment hole at `+0x04`, mean double at `+0x08`, standard deviation at
`+0x10`, median at `+0x18`, min/max bins at `+0x1c` / `+0x20` (`sizeof` `0x24`; pack 4 so the
trailing longs are not padded to 0x28).

`srCamera` was already recovered (0x188, view plane, FOV, clip and environment ranges) and lives in
`srCamera.h`. There is no evidence for a second generic `srColor` type.

The same census does **not** support adding speculative `srFont`, `srText`, `srViewport`,
`srTransform`, `srImage`, `srSprite` or `srAnimation`. No such export names exist. Viewport is a
`const int*` into `RenderScene`. Text lives in SGP `Font.*` and Wizardry's font catalogue. SR does
export `srWindow::{getWidth,getHeight,isWindow}` as static helpers, which is not a widget/text
system.

Header layout is one header per substantial top-level SurRender type; nested types stay with their
owner (`srHuffman::BitIStream`, `srModeler::Polygon`, `srTextureIFace::Dimensions`). Do not split
four-line nested records into their own files.

Current recovery status is intentionally not tracked in this document. Use the
consumer import observations, the canonical source declarations, and focused
reccmp/report output for what is recovered or still missing; this page owns ABI
rules and durable evidence, not a moving TODO/score table.

`srCore` forward-declares `srHierarchyIOManager`, `srModelIOManager` and `srVideoManager`;
the complete types live in `srImporter.h` / `srVideoManager.h`. Nested `srDD::*` records stay
incomplete; `srDD.h` carries the virtual device interface that `srDebugDD` implements.

## The Wizardry side derives from these classes

The Ghidra vtable-reference index records Wizardry installing imported SurRender vftables in its own
constructors: `srMaterial` in a family of builders, `srLight` under
`MonsterLight`, and `srBinIStream` under the virtual-file stream adapter. Some of those inherit
virtually and adjust `this` through a vbtable displacement. So the surface has to support real
derivation, not just calls.

`W8VirtualFileBinIStream` derives from `srBinIStream`, with the virtual
`srBinStream` base landing at `+0x10`, and its one recovered body stays byte-exact:

| Table | Offset | Slots | Contents |
| --- | --- | --- | --- |
| `0x005EC6A0` | `0x00` | 2 | `srBinIStream::vget` imported; `vread` overridden at `0x0047D5C0` |
| `0x005EC68C` | `0x10` | 5 | destructor and the three seek/tell slots local; `srBinStream::getSize` imported |

Both vtables come out of the declaration rather than being described in a comment, the imported slots
resolve to SR.DLL thunks the way the original's do, and `sizeof` is `0x20` - which holds only if the
base really is a vptr, a vbptr and a virtually-inherited `srBinStream` placed last. The class size
is independently fixed by the allocation its constructor's sole caller makes.

One measurable consequence outside the game image: declaring `srBinIStream`'s second slot pure, which
the exported vftable proves, makes the ZIP extension emit a `vtordisp` adjustor thunk for
`srBinIMStream::getSize` that our source did not emit before. The original `srEXT_Unzip.dll` contains
that thunk, independently supporting the pure-slot declaration.

## srClassSupport is a real base, and srNode proves it

The recovered provider bodies establish the object framework below that template. `srClass` stores
an ordinary signed 32-bit reference count. Objects begin at one, `addReference()` increments the
whole word, `autoRelease()` changes one to zero without destroying the object, and `release()`
decrements and deletes at zero or below. There is no packed autorelease bit in this SurRender build.
`srRuntimeClass` allocates its ID through the registry, owns its optional copied name, refreshes the
name index after `setName()`, and registers and unregisters at its own lifecycle layer. Its exported
copy and assignment bodies shallow-copy the name pointer and ID; higher-level `srClass::operator=`
instead calls `setName()`, preserving the destination object's registered identity during the
framework's instance-then-assign clone path.

`ClassNode` is a 0x2c-byte class-tree node. A nonzero `RegisterInstances` value gives that node its
own name and ID indexes; a zero value retains pointers to the nearest ancestor indexes. Construction
through successive support bases registers an instance at each corresponding class layer, so an
index-owning family root sees its descendants while a concrete child still keeps its own count.
The public normal and exact lookups are byte-exact wrappers around the recovered name, ID and
relative traversal. The count selector is exactness too: zero returns the inclusive count and
nonzero subtracts child-family counts.

The timestamp and update half is likewise active framework behavior. Each constructed `srClass`
touches the global counter. Update records form an intrusive list; a nonpositive interval runs once
for each advancing frame, while a positive interval catches up at each elapsed boundary. The next
record is saved before invoking a callback, allowing that callback to remove its own update safely.

The provider's nonvirtual `srClass::clone` export is a five-byte tail dispatch through slot 7, exactly
as `srClass::instance` (`0x1000E850`) dispatches through slot 6. `srClass` therefore declares slot 7
as a pure virtual beside `vInstance`; its spelling is not exported, so the source names it `vClone`.
Provider callers such as `srNode::cloneHierarchy` dispatch through the slot directly rather than
calling the forwarder. The client-emitted self-support
identity and clone families for material, camera, scene and color surface are byte-exact with the
generic template. No provider or client binary retains an SDK declaration macro, construction macro,
or nested alias name. Consequently the template behavior is recovered, while `SR_NEW` and
`ClientType` remain explicitly provisional source spellings rather than invented macro archaeology.

`srClassSupport<Derived, Base, RegisterInstances, ClassID>` supplies slots 0, 1, 2 and 7 - the
identity trio and clone - plus instance registration in its constructors and unregistration in its
destructor. It contributes no storage. Classes derive from a specialization of it rather than
declaring those members themselves.

srNode is the load-bearing proof, because both sides of its lifecycle are exported and readable:

- the constructor at `0x10050C10` calls `srClass::srClass`, installs an intermediate vtable at
  `0x10077204`, looks up class `0x1000` through the generic `sGetClassNode` shape at `0x100556D0`
  (registering with flag 1 on a miss), calls `registerInstance`, and only then installs srNode's own
  vtable;
- the destructor at `0x10050E20` mirrors it exactly: restore srNode's vtable, tear the children
  down, restore `0x10077204`, repeat the `0x1000` lookup, `unregisterInstance`, then
  `srClass::~srClass`.

A plain `srClass` base cannot produce that distinct support phase.

The intermediate table also settles who owns clone. It has eight slots, and slots 0, 1, 2, 4 and 7
(clone, `0x10055510`) hold the same targets as srNode's own table; srNode overrides only 3 (dump),
5 (destructor) and 6 (vInstance). So the support level supplies the clone body and srNode inherits it.

### The clone slot returns srClass* at every level

`srClass::clone` at `0x1000E860` is not a copy routine. Its whole body tail-calls vtable offset
`0x1c` - slot 7 - and returns `srClass*`, so srClass itself types the slot that way.

That uniform return is what makes the nested chain legal under VC6. Reconstructing the template's
clone as `Base*` instead makes each level return its own base type, so every level below narrows and
the compiler rejects the chain with C2555. That error is an artifact of the wrong reconstruction,
not a property of the hierarchy: retyping clone to `srClass*` removes it everywhere and is
byte-neutral, since a return type cannot change a pointer return in EAX.

## Sizes are the scarce evidence, and not every site has one

Neither the export table nor the vftable data states a class size, so a size has to come from an
allocation. Reviewed Ghidra instructions and P-code bind each allocation to its vtable reference;
focused reports expose that live relationship without preserving a second vptr-write inventory.

Two call forms reach an allocator, and only one was recognised at first. The global `operator new`
is called through a jump thunk; `srHeap::allocate` is called straight through its import slot, which
is how every SurRender-heap construction in the image allocates:

```text
mov  ecx, [0x005EBABC]        the srHeap global
push 0x7c                     the size
call [0x005EBAC0]             srHeap::allocate, through the slot rather than a thunk
mov  ecx, eax
call 0x004925B0               the constructor
```

Recognising the indirect form is what put sizes on the `srMaterial` family: `0x7C` for the pair the
dedicated constructors at `0x004925B0` and `0x00492720` build, and `0x78` for one of the inlined
builders. The other inlined sites still allocate through a register, where no size exists at the
site at all.

A vftable boundary also begins at any data address referenced as a relocated
operand in code. Without that rule, the `srMaterial` table runs directly into
the adjacent table and appears to have 24 slots. Applying the referenced-table
boundary gives `srMaterial` 13 slots, aligned with its subclasses: slots 3, 4,
6 and 8 through 12 reach imported implementations, while slots 0, 1, 2, 5 and
7 are locally overridden.

The constructor registers the literal class name `stMaterial` with class id
`0x10002`, under `srMaterialIFace` (`0x2200`) and `srMaterial`
(`0x2210`). Assertions in `Engine Code\materials.cpp` independently use
`ppstMaterial`, so `stMaterial` is source-backed rather than descriptive.

srMaterial derives from `srClass`, which
`include/surrender/srTypeRegistry.h` already declared: its first seven slots are srClass's, and the
four stMaterial overrides plus the destructor are exactly the five SurRender does not export, in
srClass's own declaration order. So slots 0, 1, 2 and 5 are not positional after all - they are
`getClassName`, `getClassID`, `getClassNode` and the destructor. srMaterial adds slots 7 through 12
over an extent of `0x78`; `srMaterialIFace` is the `0x2200` node the registry tree puts between the
two and carries no slot of its own.

`stMaterial` derives from srMaterial, adds one field at `0x78` for `0x7C`, and four of its five
overrides are recovered relocation-masked exact. Two carry the layout rather than a constant:
`getClassNode` at `0x00492960` walks `srRegistry` down from `0x10002` to whichever ancestor is
already registered and builds the tree back up, and slot 7 at `0x00492A00` calls slot 6 for a fresh
instance, assigns through `srMaterial::operator=`, then copies the field at `0x78`. A wrong slot
index or a wrong base extent would show up in either.

The shared slot-5 scalar deleting destructor frees through the SurRender heap,
not global `operator delete`. The identical 34-byte body appears on classes
rooted in `srClass`, so `srClass::operator delete(void*)` owns that allocation
contract; `0x00492C40` is the `stMaterial` support-template emission.

The support-template destructor at `0x00492A30` unregisters the instance three
times, restoring `0x005ECB6C`, `0x005EBF68`, and `0x005EBF94` before the
imported `srClass::~srClass`. Every write in that family targets `this+0x00`,
so the sequence is single-inheritance construction/destruction vtable churn,
not embedded subobjects.

The slots:

| Table | Slots | Slot 0 | Slots 3 and 4 |
| --- | --- | --- | --- |
| `0x005EBF94` | 8 | `srMaterialIFace::sGetClassName` | `srClass::dump`, `srClass::verify` |
| `0x005EBF68` | 11 | `srMaterial::sGetClassName` | `srClass::dump`, `srClass::verify` |
| `0x005ECB6C` | 13 | local, stMaterial's | `srMaterial::dump`, `srMaterial::verify` |
| `0x005ECB38` | 13 | local, stMaterial's | `srMaterial::dump`, `srMaterial::verify` |

These are construction and destruction tables, not final vtables. During a base's
construction/destruction phase the derived overrides are not installed yet, so slots 3 and 4
legitimately still hold `srClass` implementations while the object is only an
`srMaterialIFace` or an `srMaterial`. The uniform `srClass*` clone return and the
single-inheritance flow make this ordinary MSVC construction dispatch, not evidence of siblings.

Read as nested `srClassSupport` levels the sequence is exactly what the ABI predicts. The ascending
8, 11, 13, 13 slot counts in construction order, one registration and one unregistration per level,
and a vtable restore before each, are the signature of

```text
srClassSupport<srMaterialIFace, srClass, true, 0x2200>
  -> srClassSupport<srMaterial, srMaterialIFace, false, 0x2210>
    -> srClassSupport<stMaterial, srMaterial, false, 0x10002>
      -> final stMaterial
```

which is the same shape `srNode`'s own constructor and destructor prove directly at `0x10050C10`
and `0x10050E20`. The levels borrowing SurRender's names for themselves are support specializations
naming their `Derived`, not first-party stand-ins.

## Shared array and hash emissions

The two-word storage at Wiz8 `0x004701b0` deletes its first word through scalar
`operator delete` and clears both words. SurRender `0x100027d0` has the same
instructions. The typed `srArray<Symbol>` in Huffman's Sampler has this
destructor behavior, and Wiz8's `0x0049e290` array growth allocates scalar
storage, copies the overlapping elements, then deletes the old storage. Other
Wiz8 callers of `0x004701b0` pass different two-word objects, so the address
is a folded emission rather than evidence for a separate `W8OwnedPtr` class or
one particular element type. The template marker records the retail identity
without inventing a separate authored class for that folded helper.

`srArray` is also the heap-backed vector-stream array: its storage goes
through `new T[]`/`delete[]`, and the SurRender vector value classes declare
`operator new[]`/`delete[]` on `srHeap`. `0x004701d0` (unconditional
`srHeap.free`) is `srArray<srVector3T<float> >::release`, and the vector
emissions differ from the scalar ones only by the heap calls and the `new[]`
result select. The raw heap family is `srHeapBuffer`: `srHeap.allocate` on
bytes, no element construction, and a pointer-checked release (`0x004741b0`),
with scratch (`ensure`) and preserving (`setCapacity`) growth.

Allocation syntax follows the recovered lifetime model, not whichever call
happens to match locally. A class-specific `operator new[]`/`delete[]` is a
class-wide claim and is introduced only when the reviewed array-emission census
for that class agrees; one allocation site is not enough. Conversely, direct
`srHeap.allocate`/`free` remains direct only for storage with no evidenced
element construction/destruction. Different `srArray<T>` instantiations
reaching different heaps are not evidence for separate container templates.

The Wiz8 and SurRender hash headers have the same four-word table, chained
entries/free list, initial four buckets, doubling growth, key mixing, lookup,
insertion, removal, and bucket rebuild. Both allocate entry and bucket arrays
with `new[]` and destroy them with `delete[]`. Wiz8's `unsigned int` and
SurRender's `unsigned long` hash-key overloads are both 32-bit in the target
ABI; the only implementation differences are method definition order and
their provisional names. The `TEMPLATE` markers on Wiz8 octree/mesh helpers
and SurRender Huffman `Grow` helpers describe instantiations of this same
algorithm. No accepted source oracle establishes that both binaries used the
same *spelled* header or template name, so the two canonical source headers
retain separate names and their distinct emitted specializations. There is
no second hash implementation hidden in the octree or Huffman bodies.

## What may be written into a header

The export table gives names and signatures. It never gives a class size or a field offset, and the
vftable data never gives one either. So a header here states only what something proves:

- a member's name, signature, convention, access and virtuality: the export table;
- which slot it occupies, and whether it is pure: the exported vftable;
- where a virtual base sits: the exported vbtable, or the vbtable a first-party constructor builds;
- a class size: an allocation at a construction site, or a byte-exact port that depends on it.

Everything else stays `unknown_NN[...]` behind a `sizeof` assertion, so that a later edit which
repacks the class fails to compile instead of silently mismatching. `include/surrender/srBinIStream.h`
is the model: named where the evidence reaches, opaque and asserted where it does not.

## Supported-class copy construction and lifetime

The retail exported copies of `srNode` (`0x10051AA0`), `srMeshModel`
(`0x10041BF0`), `srPalette` (`0x10004EE0`) and the supported scene/material/texture
classes perform the same sequence: default-construct the canonical base, install
and register the support layer, call the derived assignment operator, copy-construct
the derived members, and install the final vtable. Assignment runs before those
members have been constructed. This is the support template's derived-reference
constructor followed by the compiler's implicit copy construction; it is not an
assignment in an authored derived constructor body. The early access to derived
state, subsequent retained pointer copies, and raw pointer aliasing remain retail
behavior. Copies have marker-only `SYNTHETIC` identities.

The support destructor owns unregistration. `srModel` (`0x1003C470`),
`srBounder` (`0x1004B2E0`), `srModelInstance` (`0x1004FA40`), `srIlluminator`
(`0x1004C6E0`) and `srMaterial` (`0x10016880`) must not repeat it in their bodies.
The `srModelInstance` secondary `Client` base is destroyed before support teardown;
material and material-interface support layers each unregister their own identity.

Complete copy bodies also distinguish members from alignment padding: the gaps at
`srIlluminator+0x14C`, `srLight+0x154/+0x224`, `srClipPlane+0x14C` and
`srScene+0x18C` are not copied as fields. Natural alignment preserves the observed
object extents and member offsets without source byte arrays. Retail lifecycle
unwind frames in the core/node/model/material translation units establish `/GX`; their
build settings must preserve member/base cleanup rather than compiling with `/GX-`.

The Huffman sampler copy at `0x100014F0` completes its hash member before
entering the array member's copy construction. The handwritten default-member
construction followed by assignment gave that array a completed lifetime too
early. Its existing primary `srArray` copy constructor supplies default
initialization followed by assignment; the sampler's ordinary implicit copy
uses that constructor in member order. Hash copying remains shallow while the
array copies its storage. Do not repair the resulting shared hash ownership.
Wiz8 imports the sampler's default constructor, destructor and `insert`, but
does not statically import its copy constructor or assignment.

The palette optimizer has only static operations and nested records. VC6 emits
its ordinary empty-class assignment as the byte copy at `0x10004B90`; an empty
handwritten assignment incorrectly omitted that compiler-owned operation.
Quantizer assignment copies the established `0x21918`-byte member payload,
including the raw `lut_row` pointer. The redundant leading-byte copies in
palette sampler/quantizer lifecycle bodies do not establish another field,
union or base. Their original helper/base structure remains unresolved. VC6
class export alone does not emit the quantizer's trivial implicit copy
constructor; retain its recovered constructor until a real source use or
stronger member model explains that emission, rather than inventing a use.

The retail `srVP_generic` table at `0x100776B0` establishes the vector4 indexed
multiply slots: `+0x1C8` is linear-source multiplication and `+0x1CC` is the constant
form. `srDebugVP` (`0x10077960`) forwards these through `0x1006F190` and
`0x1006F100`, respectively. Its statistics indices are reversed (141/140), and the
constant form omits the constant pointer from alignment tracking. Keep these quirks;
source-projected function names must not reverse the interface slots.

The renderer texture-set cache insertion at `0x10024280` hashes the supplied
key by reference and assigns its three words into the allocated entry. It does
not copy-construct a temporary key or call the `srShader` copy constructor.
Keep borrowed-key lookup/insertion/removal in the primary `srHashTable`; scalar
and pointer instantiations otherwise conceal the extra lifetime introduced by
a local key copy. Texture-set equality compares both texture pointers and the
shader value. Its stored index can be zero, so interning tests entry presence
with `FindNextEntry`, rather than testing a `Lookup` result for truth.

The texture-set and scheduler growth bodies (`0x100279E0`, `0x10014750`)
establish unsigned doubling/minimum-four capacity arithmetic and signed
iteration bounds. Growth initializes bucket heads and entry links, rehashes
live entries, then links the unused suffix. Texture-set entry allocation also
initializes each embedded shader before those links. Ordinary array allocation
and primary-template assignment express these lifetimes; no manual constructor
loop or temporary key is needed. Texture-cache initialization/closure use the
same clear-and-grow operation (`0x10028200`, `0x10028460`). Keep the surrounding
default-texture deletion and pool/list reset order.

The texture/material interfaces extend the same support-copy model. Their
copy emissions (`0x1005F660`, `0x10034C70`) perform default `srClass`
construction, support registration and containing-class assignment, with no
remaining interface members. `srTexture` (`0x1005F150`) then copy-constructs
its members after the support layer's assignment. Its embedded palette pointer
is retained and stored without releasing a pre-existing member value at that
stage. A handwritten containing constructor would default-construct the
pointer before its body and incorrectly replace this lifetime with assignment.
Keep implicit copies and preserve the support layer's access to unconstructed
derived state. The retail export bundles cover these classes' lifecycle and
ordinary members; provider class export supplies implicit emissions without
invented source uses. Consumer import spelling remains separately evidenced.
These three abstract classes use `novtable`: their default/copy constructors
and complete destructors install only the support/base vtables, and retail
exports no own-class vtable for them. This avoids inventing three provider
exports and extra construction/destruction vptr writes.

Current VC6 implicit derived-copy builds select `srClassSupport(const Derived&)`
and reproduce the retail support lifecycle reviewed at `0x10051AA0` and
`0x10041BF0`. Do not replace that
converting constructor with a template-self copy constructor merely from
modern overload-resolution expectations. The surface-interface copy
(`0x1005A120`) also has a byte copy at `+0x19` whose member identity remains
unresolved; it does not establish a four-byte array copy or an invented empty
class boundary. Its recovered explicit body remains pending that model.

`srColorSurfaceIFace` also uses `novtable`. Its default constructor
(`0x100571F0`), copy constructor (`0x1005A120`) and complete destructor
(`0x10021260`) install only the support-phase table at `0x10076708`.
Independent retail table bytes identify slot 3 as the exported `srClass::dump`
at `0x1000E620`, rather than the interface override at `0x1005AE10`; this is
not a source-projected table-name inference. Preserve the existing member
import/export declarations and unresolved copy model. The constructor also
zeros the clamp word at `+0x28` before clearing the complete field block;
whether that reflects a flag-wrapper constructor or scalar initialization
remains unresolved.
