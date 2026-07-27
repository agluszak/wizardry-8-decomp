# The SurRender boundary

SurRender is linked, not authored, so its bodies are not a recovery subject. What the project needs
from it is a *declaration* surface: enough of `sr.dll`'s classes, in real C++, that the first-party
units calling and deriving from them compile in the VC6 target and still match. This document covers
where that surface comes from and how far it can honestly be taken.

## What the export table already settles

`sr.dll` exports 2059 decorated symbols and Wizardry imports 461 of them across 51 classes. Names,
signatures, calling conventions, access and virtuality are all read out of the export table by
`wiz8 surrender-abi`, decoded through `llvm-undname` rather than a local demangler.

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
fifteen vbtables, which is what makes a decoded run a whole table rather than one that ran into its
neighbour.

## The Wizardry side derives from these classes

`evidence/reviewed/wiz8/imported-vftable-sites.csv` records Wizardry installing imported SurRender
vftables in its own constructors: `srMaterial` in a family of builders, `srLight` under
`MonsterLight`, and `srBinIStream` under the virtual-file stream adapter. Some of those inherit
virtually and adjust `this` through a vbtable displacement. So the surface has to support real
derivation, not just calls.

That is now proven possible. `W8VirtualFileBinIStream` derives from `srBinIStream`, with the virtual
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
that thunk, so emitting it is the more faithful shape; it currently matches at 66.67%, and being a
new imperfect row it lowers that target's reported accuracy average while making the class model
closer to the original rather than further from it.

## Sizes are the scarce evidence, and not every site has one

Neither the export table nor the vftable data states a class size, so a size has to come from an
allocation. `evidence/snapshots/polymorphism/vptr-writes.csv` now carries `allocation_size` on every
offset-`0` store, read back from the store rather than from a call to a constructor - which is what
makes an inlined construction, the common shape here, yield one at all.

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

What that does not settle is where `srMaterial` ends and the first-party class begins, and a harder
problem is in the way. `??_7srMaterial@@6B@` has **24** slots, while all three first-party vtables
these builders install - `0x005EBDE0`, `0x005EBE14` and `0x005EBE48`, adjacent and `0x34` apart -
have **13**. A class cannot have fewer virtual slots than the base it derives from, so either those
tables are not `srMaterial` subclasses or one of the two counts is wrong. Each of the three carries
its own constructor writes, so they are three separate tables and not one the census split early.
That contradiction comes before any port of the family.

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
