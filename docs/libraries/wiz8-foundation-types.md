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
