# Wizardry foundation types

## The growable vector template

Two separately-named structs in this repository — `W8NPCItemListVector` and
`W8MonsterGeneratorVector` — were not separate container designs. They are uses of one hand-rolled
`W8GrowableVector<T>` template, now defined once in `include/wiz8/vector.h`. Every owner spells its
element type through that template: `g_npc_item_lists` is a
`W8GrowableVector<W8NPCItemList*>*`, `W8World::monster_generators` is a
`W8GrowableVector<W8MonsterGenerator*>*`, and the dialog member at `0x005D14D0` embeds two
`W8GrowableVector<W8DialogOwned005D14D0*>` subobjects. An owner whose element type is not yet
recovered names it positionally rather than erasing it to `void*`, because one erased spelling
shared by unrelated owners would merge template identities the image keeps apart — each element
type has its own vtable and its own destructor COMDATs.

The vtable a constructor installs is what separates them, and
`evidence/snapshots/polymorphism/vptr-writes.csv` records it per site. Both `MonsterManager.cpp`
vectors — `W8MonsterManagerEntry` at `+0xd8` and `W8MonsterManagerState` at `+0x9b7` — install
`0x005EBFE0`, so they are one instantiation, shared with nineteen further owner bodies and named
`W8GrowableVector<W8VectorElement005EBFE0*>` until the element type itself is proven. The vector
`GenerateItemsFromTable` builds for candidate indices installs `0x005EC0E0` instead, which is why
`W8GrowableVector<int>` is a different specialization rather than the same one spelled two ways.

`GetAt` returns the address of the element, bounds-checked against `count`, and `RemoveAt` returns
the element it unlinked. Both are inlined at every call site, and `RemoveAt`'s return is why one
method serves two inlinings: the dialog destructor at `0x005D1590` deletes what it returns while
`GenerateItemsFromTable` discards the same value. Calling `GetAt` is not always the right port
even so — `GetNPCItemListByID` reads the count once and keeps the storage pointer it already
loaded, and spelling that as the accessor re-reads both.

The layout is read off a constructor such as `0x005098B0`:

```text
push 0x10 ; call operator new     the container itself
mov  [esi], 0x005ED810            vptr
push 0x14 ; call operator new     backing store, capacity * 4
mov  [esi+0x0c], eax              data
mov  [esi+0x04], 0                count
mov  [esi+0x08], 5                capacity
```

| Offset | Field | Notes |
| --- | --- | --- |
| `0x00` | `vptr` | **not padding** — these are polymorphic C++ objects |
| `0x04` | `int count` | |
| `0x08` | `int capacity` | 5 at every inlined construction |
| `0x0C` | `T* data` | `operator new(capacity * sizeof(T))` |

Size `0x10`. If the backing allocation fails the constructor stores capacity `0` rather than `5`,
which is why the field is read rather than assumed.

The 5 is an argument, not a constant. Every construction that uses it is inlined, so the clamp and
the multiply fold away and only `operator new(20)` survives. Out-of-line template constructor
emissions retain the parameter and clamp the requested capacity up to one. Whether the original
spelled a default argument or a separate default constructor cannot be read off the image.

The destructor for that instantiation, `0x0050E510`, is a textbook MSVC scalar deleting destructor:
restore the vptr, `operator delete` the backing store at `+0x0c`, then conditionally `operator
delete` the object itself on the low bit of the hidden flag.

## Scale, and one claim that does not generalise

Scanning `.text` for that destructor shape finds a broad family of candidate template
instantiations. The scan does not establish every element type, nor does a following code pointer
by itself prove another virtual slot: tightly packed specialization vtables can be adjacent. The
canonical per-vtable observations live in
`evidence/observations/wiz8/ptr-vector-instantiations.csv`; this document deliberately does not
promote that mechanical inventory into named source types.

## Grow is shared, and the hierarchy that allows that is unresolved

`Grow` is not instantiated per element type. The image contains exactly one body, at `0x004ADDF0`.
Its callers span roughly thirty translation units, and joining them against
`evidence/snapshots/polymorphism/vptr-writes.csv` shows sixteen *different* vector vtables being
constructed inside those same caller bodies — including the `W8GrowableVector<int>` at `0x005EC0E0`
and the `0x005EBFE0` instantiation `MonsterManager.cpp` embeds. One body cannot be a template
member of sixteen specializations.

It is reached through `ecx`, so it is a member function rather than a free helper, and it reads
count, capacity and data at `+0x04`, `+0x08` and `+0x0c` off its own `this` — so the object it
receives already carries a vptr at offset zero.

The obvious model, a polymorphic `W8GrowableVectorBase` holding the storage and `Grow` with
`W8GrowableVector<T>` derived from it, reproduces `Grow` byte-exactly and pulls
`GenerateItemsFromTable` from 139 bytes over the original to 24. It also costs four reviewed-exact
lifetime bodies — `0x005D14D0`, `0x005D1590`, `0x005D2540` and `0x005D2560` — an extra vptr store,
because the extra polymorphic level is one more than the original constructs. So that hierarchy is
wrong too, and the header keeps `Grow` as a template member: a spelling that is knowingly not the
original's, chosen because it costs no reviewed body. Resolving it is tracked in Beads.

### Specialization identities are recorded directly

The source index follows the same convention used by ISLE: a standalone vtable annotation names the
template specialization directly, without inventing an empty address-qualified derived class:

```cpp
// VTABLE: WIZ8 0x005ec294
// class W8GrowableVector<W8VectorElement005EC294*>
```

Template constructor and complete-destructor emissions use `TEMPLATE`; compiler-generated deleting
destructors use `SYNTHETIC`. These annotations account for emitted code without claiming that the
original source contained wrapper lifecycle bodies. The specialization annotations are consolidated
in `src/wiz8/vector.cpp`.

Some constructors write an adjacent vtable before the specialization table. That observation remains
real, but it does not justify manufacturing a second source class. Until independent evidence names
the adjacent owner, it remains unresolved rather than being projected as a base or derived wrapper.
This also leaves the shared `Grow` source hierarchy open.

## Everything else the image repeats is not a first-party template

VC6's linker does not fold identical COMDATs, so two byte-identical bodies at different addresses
were emitted twice from one source. Clustering the accepted function starts that way — masking
relocated operands, `call`/`jmp` displacements and trailing padding, all of which a second emission
changes without the source differing — puts the growable vector at the top of the first-party
results and finds no second class template under it. What the other large clusters are:

| Shape | Example | What it is |
| --- | --- | --- |
| initialiser and `atexit` thunk pair | `0x004217A0` | MSVC's dynamic initialiser for a file-scope object |
| scalar deleting destructor that calls a separate destructor | `0x004218B0` | compiler-generated, one per polymorphic class |
| the same, freeing through `srHeap::free` | `0x0042A170` | compiler-generated for the SurRender-allocated classes |
| `srRegistry::registerClass` chains | `0x0042A030`, `0x0047EAC0` | SurRender's class-registration macro expanded in first-party units |
| `operator new` under an EH state, then a constructor call | `0x0044EDF0` | a typed heap factory, one per type |

Only one first-party duplicate in that survey is a shared *function*: the quicksort at `0x00467640`
and `0x0048A3D0`, which sorts an unsigned key array while permuting a parallel array through the
same index. Two emissions of one header-defined helper, in units the assertion anchors bracket to
`..\Engine Code\Include\stHeap.hpp` and `Engine Code\ReadMesh.cpp`.

The other templates in the image are SurRender's, `srVector3T<float>` and `srVector3T<double>` among
them, and they are declarations here rather than recovery subjects.

## Why this kept being rediscovered

A per-element-type vtable plus a shared capacity-5 default is what a template instantiated once per
element type looks like. Each specialization gets its own vtable and destructor COMDATs, clustered
by translation unit exactly as VC6 emits them — so one source template can look like many unrelated
binary classes until the shared shape is recognised. `GenerateItemsFromTable` makes the distinction
concrete: it uses `W8GrowableVector<int>` for candidate indices and
`W8GrowableVector<W8WorldItem*>` for its output, both from the same definition.

The bounds-checked element accessor is inlined into its callers, which explains the in-loop
`cmp index, count` / `jge` guard around the `lea` that
the compiler emits in both `GetNPCItemListByID` and `FindMonGenByName`. It is this template's
accessor, not a quirk of those two functions.

## Allocator as an ownership discriminator

Allocator choice is a class-recovery signal because three independently named allocation families
coexist in the same image:

| Allocation family | Typical ownership signal | Proven example |
| --- | --- | --- |
| global `operator new` / `operator delete` | first-party C++ object or template | this polymorphic vector template |
| CRT `malloc` / `free` / `realloc` | C-style record, cache, or resizable buffer | `MonsterDBFromSpecies`'s `0x297`-byte record cache |
| `srHeap::allocate` / `srHeap::free` | SurRender-facing or header-inline SR type | the inline string recovered in `srEXT_Unzip.dll` |

The reviewed address-to-family relationship lives in
`evidence/reviewed/wiz8/allocator-layers.csv`. The exact SurRender decorated imports remain in
`evidence/observations/surrender/wiz8-sr-imports.csv`. This discriminator is supporting ownership
evidence, not proof by itself: a boundary adapter can deliberately allocate through a vendor heap,
so call sites and layout still have to agree.

The global `operator new` returns null in this executable's VC6 ABI. The constructor above therefore
has two semantic failure paths: a failed object allocation leaves the owner null, while a failed
backing allocation retains the object but stores capacity zero. See
`docs/libraries/msvc6-runtime.md` for the import shape and porting contract. The much denser delete
surface is not anomalous: template instantiations emit ordinary and scalar-deleting destructor
forms, both of which eventually reach the shared delete wrapper.


## `PList`

A second, unrelated container comes from `3D Code\PList.cpp`. Its canonical assertions name the
parameters `ppl` and `pEntry`; source markers own its functions and reccmp reports current matching.

| Offset | Field |
| --- | --- |
| `0x00` | `void** data` |
| `0x04` | `int capacity` |
| `0x08` | `int count` |

It is worth stating what `PList` is *not*: it has no vptr, its elements sit at `+0x00` rather than
`+0x0C`, its count is at `+0x08` rather than `+0x04`, and it is reached through free functions
rather than methods. Nothing about it is shared with `W8GrowableVector` beyond both being arrays of
pointers, so the two must not be conflated the way `W8NPCItemListVector` and
`W8MonsterGeneratorVector` were.

`PListInit` allocates ten entries and stores `10` at `+0x04`; `PListAdd` and `PListInsert` grow that
capacity by five. `PListGetAt` bounds-checks with a signed `jge`, so the index is `int`; both it and
`PListGetCount` return zero for a null list rather than faulting. All twelve PList bodies and all
nine IList bodies are now source-owned and relocation-masked exact.

### The loop shape that finally matched

`PListIndexOf` is a search loop, and it took four attempts, which is worth recording because five
other functions still show the same loop-peeling difference in live comparison:

| Source shape | Result |
| --- | ---: |
| `do`/`while` over a cursor, early `return` | 106 bytes, first compare peeled, two epilogues |
| same, with `goto` to a shared exit | 106 bytes, still peeled |
| `for` over a cursor | 98 bytes, only the `data` load misplaced |
| `for` indexing `ppl->data[index]` | **exact** |

VC6 lowers a counted `for` over `base[i]` into a guard plus a rotated `do`/`while` with one backward
branch, and sinks the `base` load past the guard because it is only needed inside the loop. Rolling
the cursor by hand defeats both. Any near-miss that is a few bytes long with a duplicated comparison
should try the counted-`for`-over-index form first.


## `IList`

`3D Code\IList.cpp` is `PList`'s integer sibling: the same shape — elements at `+0x00`, count at
`+0x08`, reached through free functions — but the elements are `int`. Current function coverage and
matching state are generated by `just wiz8 report status`.

What separates the two types is the failure value: `IListGetAt` returns `-1` where `PListGetAt`
returns null. A sentinel of `-1` only makes sense for a list of integers, so the element type is
established by behaviour rather than assumed from the `I`/`P` naming.

`IListClear` resets only the count and leaves the allocation in place, while `IListFreeData` releases
the element array through the CRT and returns TRUE. Every assertion in the unit names its parameter
`pls`, matching the `pls` Hungarian prefix already seen on `gXStatus.plsMonsterList`.

So the three containers recovered so far are distinct and must not be merged:

| Type | vptr | Elements | Count | Accessors |
| --- | --- | --- | --- | --- |
| `W8GrowableVector<T>` | yes, at `+0x00` | `+0x0C` | `+0x04` | methods |
| `W8PList` | no | `+0x00` | `+0x08` | free functions |
| `W8IList` | no | `+0x00` (ints) | `+0x08` | free functions |


`IListCreate` allocates a `0x0C`-byte object, which fixes `sizeof(W8IList)` at exactly three ints,
then inlines `IListInit`. `IListAdd` grows the array by five when `count` reaches `capacity`, copies
the old elements across, and returns the index it stored at; its growth assertion names the
temporary `pTemp`.

`IListInit` establishes the middle field: it allocates ten ints and stores `10` at `+0x04`, so the
layout is `data` / `capacity` / `count`. Assigning its byte-sized success result immediately after
the allocation store makes VC6 defer the trailing `setne al` until after both field stores, matching
the original standalone body as well as its inlined copy inside `IListCreate`.

`IListDestroy` asserts twice under a single null test because `IListFreeData` is inlined into it and
VC6 merges the two null checks — a useful reminder that two assertion line numbers in one guard
means an inlined callee, not two checks in the source.

`IListIndexOf` matched on the first attempt using the counted-`for`-over-index shape. `IListRemove`
needed a separate shift cursor even though it begins equal to the search index; that preserves the
original register allocation and completes the nine-function unit exactly.

## A folded getter

`0x005E2C70` is a 13-byte count getter, and a count getter is byte-identical for both list types. It
sits **inside** `IList.cpp`'s run, flanked by `IList` functions on both sides, while the other
`PList.cpp` bodies occupy `0x005E22C0`–`0x005E2890`. That is identical-COMDAT folding: both units
defined the same getter, the linker kept one body, and callers of either resolve to it. The repository
applies the name `PListGetCount` because its callers use it on `PList` objects, but the retained
COMDAT belongs to `IList.cpp`, and the address should not be read as evidence for either unit alone.

## SurRender math templates

The vector and matrix types passed across the SR.DLL boundary are SurRender types, not Wizardry
types. Decorated exports establish the names `srVector2T`, `srVector3T`, `srVector4T`,
`srMatrix3T`, `srMatrix4T`, `srVector2i`, and `srVector3i`; their reviewed identities and layouts
live in `evidence/reviewed/surrender/math-types.csv`. The exported SR.DLL stream operators establish
the storage without requiring guessed member functions:

| Type | Proven instantiations | Proven layout |
| --- | --- | --- |
| `srVector2T` | `float` | two adjacent scalars |
| `srVector3T` | `float`, `double` | three adjacent scalars |
| `srVector4T` | `float` | four adjacent scalars |
| `srMatrix3T` | `float`, `double` | three adjacent `srVector3T` elements |
| `srMatrix4T` | `float`, `double` | four adjacent `srVector4T` elements |
| `srVector2i` | `int` | two adjacent signed integers |
| `srVector3i` | `int` | three adjacent signed integers |

MSVC's decorated scalar codes make the template convention explicit: `M` is `float` and `N` is
`double`. The imported API then shows how SurRender uses that split. Mesh vertices, texture
coordinates, bounds, polygon equations, fog, gamma, ambient light, and renderer matrices use the
float forms. Persistent scene-node state uses the double forms: `srNode::getLocation` returns
`srVector3T<double>`, while `setLocation`, `setScale`, rotations, and world-space setters accept
double vectors or matrices. Float output overloads exist where scene state crosses into rendering.

The renderer transform surface is likewise visible directly in imports: `srGERD::matrixMode`,
`pushMatrix`, `popMatrix`, `rotate`, `scale`, `translate`, `getMatrix`, `getNormalMatrix`, and
`getInverseModelViewMatrix`. These names constrain later world-transform recovery; they do not
justify inventing a separate Wizardry math layer. Scalar math comes from the CRT or inline x87
instructions, so current evidence provides no third math library.

Two exact bodies compiled into Wiz8.exe belong to `srVector3T<float>`. `0x00421680` converts three
double arguments into the three float fields and returns `this`; `0x00446110` converts an
`srVector3T<double>` into an `srVector3T<float>` and also returns `this`. SR.DLL does not export the
inline member names, so the source uses the explicit positional placeholders `method_00421680` and
`method_00446110`. The old CFAgent descriptions `VectorFromThreeFloats` and `Copy3DVector` remain
aliases only. Translation-unit reporting classifies both bodies as external SurRender templates,
not recovered first-party Wizardry functions.

## The SurRender inline string, and where it is allowed to appear

There is no first-party string class and no `std::string` anywhere in `Wiz8.exe`. VC6's
`std::basic_string` emits "string too long" and "invalid string position" through `_Xlen`/`_Xran`,
`std::vector` emits "vector too long", and none of those strings exists in the image; MSVCP60.DLL
is imported with exactly four symbols, all `<iostream>` initialization pulled in because fourteen
SurRender virtuals take a `std::basic_ostream&`. Wide text is fixed-size `W8WideChar` arrays inline
in records, manipulated CRT-direct (`wcschr`, `wcscpy`, `swprintf`, ...), and narrow text is raw
`char*` — the original's own `pac` Hungarian prefix, visible on plain pointers throughout the
assertion harvest, says exactly that.

The one string object in the image is SurRender's header-inline string class, and its reach is a
single translation unit. Layout, read off the assignment operator at `0x0047CE00`:

| Offset | Field | Meaning |
| --- | --- | --- |
| `+0x00` | `char inline_nul[4]` | `data == this` exactly when the string is empty |
| `+0x04` | `int length` | includes the terminator, so 1 when empty |
| `+0x08` | `char* data` | heap storage |

It is SurRender's rather than Wizardry's because it allocates through `srHeap::allocate` /
`srHeap::free` (`0x005EBAC0` / `0x005EBAB8`) rather than global `operator new` — the same allocator
split that separates the vendor from the first-party growable vector above. `0x0047CE90` is its
`Find`, delegating to CRT `strstr`. The empty-string initialization sequence occurs exactly three
times in the whole image, all between `0x0047CDEA` and `0x0047D293` — the `VirtualFileBinIStream`
SLF-to-SurRender stream adapter, which builds string temporaries to hand filenames to SurRender.
`srHeap::allocate` has 108 call sites, so the vendor heap is used widely, but its string class is
not. When those adapter bodies are ported, the type belongs in that unit as a vendor-marked model;
nothing else should acquire a string class, and `srStringTable` is unrelated — it is a
device-description table consumed by `srGERD::loadDevice`.
