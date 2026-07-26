# Wizardry foundation types

## The growable pointer array

Two separately-named structs in this repository — `W8NPCItemListVector` and
`W8MonsterGeneratorVector` — were the same type: one instantiation each of a hand-rolled growable
pointer array. They are now a single `W8PtrVector` in `src/wiz8/gameplay_boundaries.h`.

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
| `0x0C` | `void** data` | `operator new(capacity * 4)` |

Size `0x10`. If the backing allocation fails the constructor stores capacity `0` rather than `5`,
which is why the field is read rather than assumed.

The destructor for that instantiation, `0x0050E510`, is a textbook MSVC scalar deleting destructor:
restore the vptr, `operator delete` the backing store at `+0x0c`, then conditionally `operator
delete` the object itself on the low bit of the hidden flag.

## Scale, and one claim that does not generalise

Scanning `.text` for that destructor shape — a load from `[this+0x0c]`, a vptr store, and a call to
`operator delete` at `0x005E1C10` — finds **75 distinct vtables**. That count is solid and was
reproduced independently.

The per-vtable virtual count is *not* uniform, and it would be wrong to say every instantiation has
exactly one virtual:

| Vtable shape | Count |
| --- | ---: |
| Single virtual (the destructor) | 35 |
| A second code pointer follows | 31 |
| Next slot is another instantiation's vtable, so undeterminable this way | 9 |

So the destructor *shape* is shared by 75 vtables, but only 35 are confirmed to be one-entry. The
other 31 carry more virtuals and may be a richer container or a derived class. The inventory in
`config/analysis/wiz8/ptr-vector-instantiations.csv` records the determination per vtable rather
than asserting a uniform answer.

None of the 75 destructors falls inside current translation-unit interval coverage, so the
`source_unit` column is empty throughout and the `element_type` column is filled only where a
consumer establishes it.

## Why this kept being rediscovered

A per-element-type vtable plus a shared capacity-5 default is what a template instantiated once per
element type looks like. Each instantiation gets its own vtable and its own destructor bodies,
clustered by translation unit exactly as COMDAT emission places them — so each one looks like a
fresh, unrelated struct until the shape is recognised.

The bounds-checked element accessor is inlined into its callers, which explains the in-loop
`cmp index, count` / `jge` guard around the `lea` that
`config/analysis/reccmp/wiz8-gameplay-boundaries.csv` records as odd in both `GetNPCItemListByID`
and `FindMonGenByName`. It is this template's accessor, not a quirk of those two functions.

## Allocator as an ownership discriminator

This container allocates through the global `operator new`, whereas SurRender's own types allocate
through `srHeap` — which `Wiz8.exe` imports as one of the nine plain-C symbols in
`config/analysis/surrender/wiz8-sr-imports.csv`. Which allocator a body calls is therefore a usable
first-party-versus-vendor signal elsewhere in the image.


## `PList`

A second, unrelated container, from `3D Code\PList.cpp`. Its canonical assertions name the
parameters `ppl` and `pEntry`, and its three accessors are now byte-exact:

| Address | Function | Size |
| --- | --- | ---: |
| `0x005E2C70` | `PListGetCount` | 13 |
| `0x005E2870` | `PListGetAt` | 26 |
| `0x005E2890` | `PListIndexOf` | 98 |

| Offset | Field |
| --- | --- |
| `0x00` | `void** data` |
| `0x08` | `int count` |

It is worth stating what `PList` is *not*: it has no vptr, its elements sit at `+0x00` rather than
`+0x0C`, its count is at `+0x08` rather than `+0x04`, and it is reached through free functions
rather than methods. Nothing about it is shared with `W8PtrVector` beyond both being arrays of
pointers, so the two must not be conflated the way `W8NPCItemListVector` and
`W8MonsterGeneratorVector` were.

`PListGetAt` bounds-checks with a signed `jge`, so the index is `int`; both it and `PListGetCount`
return zero for a null list rather than faulting.

### The loop shape that finally matched

`PListIndexOf` is a search loop, and it took four attempts, which is worth recording because five
other functions in `config/analysis/reccmp/wiz8-gameplay-boundaries.csv` are still
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
`+0x08`, reached through free functions — but the elements are `int`.

| Address | Function | Size |
| --- | --- | ---: |
| `0x005E29A0` | `IListInit` | 81 (structurally-strong) |
| `0x005E2A00` | `IListDestroy` | 83 |
| `0x005E2A60` | `IListFreeData` | 56 |
| `0x005E2B50` | `IListClear` | 43 |
| `0x005E2C80` | `IListGetAt` | 55 |
| `0x005E2CC0` | `IListIndexOf` | 66 |

What separates the two types is the failure value: `IListGetAt` returns `-1` where `PListGetAt`
returns null. A sentinel of `-1` only makes sense for a list of integers, so the element type is
established by behaviour rather than assumed from the `I`/`P` naming.

`IListClear` resets only the count and leaves the allocation in place, while `IListFreeData` releases
the element array through the CRT and returns TRUE. Every assertion in the unit names its parameter
`pls`, matching the `pls` Hungarian prefix already seen on `gXStatus.plsMonsterList`.

So the three containers recovered so far are distinct and must not be merged:

| Type | vptr | Elements | Count | Accessors |
| --- | --- | --- | --- | --- |
| `W8PtrVector` | yes, at `+0x00` | `+0x0C` | `+0x04` | methods |
| `W8PList` | no | `+0x00` | `+0x08` | free functions |
| `W8IList` | no | `+0x00` (ints) | `+0x08` | free functions |


`IListInit` establishes the middle field: it allocates ten ints and stores `10` at `+0x04`, so the
layout is `data` / `capacity` / `count`. It is the one near-miss in the unit — VC6 materialises the
success boolean into `cl` early where the original defers a trailing `setne al` past both field
stores, and three source orderings all produce the early form.

`IListDestroy` asserts twice under a single null test because `IListFreeData` is inlined into it and
VC6 merges the two null checks — a useful reminder that two assertion line numbers in one guard
means an inlined callee, not two checks in the source.

`IListIndexOf` matched on the first attempt using the counted-`for`-over-index shape, confirming the
technique found on `PListIndexOf` generalises rather than being a one-off.

## A folded getter

`0x005E2C70` is a 13-byte count getter, and a count getter is byte-identical for both list types. It
sits **inside** `IList.cpp`'s run, flanked by `IList` functions on both sides, while `PList.cpp`'s
own functions occupy `0x005E2780`–`0x005E28F0`. That is identical-COMDAT folding: both units defined
the same getter, the linker kept one body, and callers of either resolve to it. The repository
applies the name `PListGetCount` because its callers use it on `PList` objects, but the retained
COMDAT belongs to `IList.cpp`, and the address should not be read as evidence for either unit alone.
