# Wizardry foundation types

## The growable vector template

Two separately-named structs in this repository — `W8NPCItemListVector` and
`W8MonsterGeneratorVector` — were not separate container designs. They are uses of one hand-rolled
`W8GrowableVector<T>` template, now defined once in `include/wiz8/vector.h`. `W8PtrVector` is only
an erased `void*` specialization retained for consumers whose element type has not yet been
recovered; new source-owned code uses the known specialization directly.

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
| `0x08` | `int capacity` | 5 in every decoded instantiation |
| `0x0C` | `T* data` | `operator new(capacity * sizeof(T))` |

Size `0x10`. If the backing allocation fails the constructor stores capacity `0` rather than `5`,
which is why the field is read rather than assumed.

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

## Why this kept being rediscovered

A per-element-type vtable plus a shared capacity-5 default is what a template instantiated once per
element type looks like. Each specialization gets its own vtable and destructor COMDATs, clustered
by translation unit exactly as VC6 emits them — so one source template can look like many unrelated
binary classes until the shared shape is recognised. `GenerateItemsFromTable` makes the distinction
concrete: it uses `W8GrowableVector<int>` for candidate indices and
`W8GrowableVector<W8WorldItem*>` for its output, both from the same definition.

The bounds-checked element accessor is inlined into its callers, which explains the in-loop
`cmp index, count` / `jge` guard around the `lea` that
`config/reccmp/wiz8-gameplay-boundaries.csv` records as odd in both `GetNPCItemListByID`
and `FindMonGenByName`. It is this template's accessor, not a quirk of those two functions.

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
parameters `ppl` and `pEntry`; the reviewed function inventory and matching state live in
`config/reccmp/wiz8-gameplay-boundaries.csv`.

| Offset | Field |
| --- | --- |
| `0x00` | `void** data` |
| `0x04` | `int capacity` |
| `0x08` | `int count` |

It is worth stating what `PList` is *not*: it has no vptr, its elements sit at `+0x00` rather than
`+0x0C`, its count is at `+0x08` rather than `+0x04`, and it is reached through free functions
rather than methods. Nothing about it is shared with `W8PtrVector` beyond both being arrays of
pointers, so the two must not be conflated the way `W8NPCItemListVector` and
`W8MonsterGeneratorVector` were.

`PListInit` allocates ten entries and stores `10` at `+0x04`; `PListAdd` and `PListInsert` grow that
capacity by five. `PListGetAt` bounds-checks with a signed `jge`, so the index is `int`; both it and
`PListGetCount` return zero for a null list rather than faulting. All twelve PList bodies and all
nine IList bodies are now source-owned and relocation-masked exact.

### The loop shape that finally matched

`PListIndexOf` is a search loop, and it took four attempts, which is worth recording because five
other functions in `config/reccmp/wiz8-gameplay-boundaries.csv` are still
`structurally-strong` with "loop peeling" named as the remaining difference:

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
