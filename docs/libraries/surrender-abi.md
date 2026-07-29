# The SurRender boundary

SurRender is linked, not authored, so its bodies are not a recovery subject. What the project needs
from it is a *declaration* surface: enough of `sr.dll`'s classes, in real C++, that the first-party
units calling and deriving from them compile in the VC6 target and still match. This document covers
where that surface comes from and how far it can honestly be taken.

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

## The Wizardry side derives from these classes

The Ghidra vtable-reference index records Wizardry installing imported SurRender vftables in its own
constructors: `srMaterial` in a family of builders, `srLight` under
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

What that does not settle is where `srMaterial` ends and the first-party class begins - but chasing
it caught a defect in the vftable decoder, which is worth recording because the check that was
supposed to catch it did not.

`??_7srMaterial@@6B@` first decoded to 24 slots while all three first-party vtables the builders
install had 13, and a class cannot have fewer virtual slots than its base. The first-party tables
were right. Slots 13 to 23 were a second table sitting immediately behind srMaterial's: the same
three leading targets repeated, then `srClass::dump` and `srClass::verify` where srMaterial has its
own, then pure stubs. Relocations and executable targets do not end a table that another table
follows, so the run walked straight through the boundary - and the three-build agreement did not
notice, because every build lays the two tables out the same way. A systematic over-read is
systematic.

The fix is the rule the first-party census already uses: a table has to be referred to to be used at
all, so any data address appearing as a relocated operand in code begins one. With that boundary
`srMaterial` decodes to 13 slots and lines up with its subclasses exactly - slots 3, 4, 6 and 8
through 12 reached by import thunk, slots 0, 1, 2, 5 and 7 overridden locally. `srBinStream` and
`srBinIStream` are unchanged at 5 and 2, so the stream pilot's evidence stands.

The reviewed classification was right all along, and the constructor names the class outright. It
registers with `srRegistry` under the literal `stMaterial` and the class id `0x10002`, spelling the
parent chain as it goes - `srMaterialIFace` at `0x2200`, `srMaterial` at `0x2210`, then this - and
`Engine Code\materials.cpp` is the unit, whose own assertions call the pointer `ppstMaterial`. So
`stMaterial` is the original's name, not a descriptive one.

That is enough to declare both classes and port. srMaterial derives from `srClass`, which
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

The destructor is the one override still outstanding, and taking it apart moved two things forward.

Its slot-5 body is freed through the SurRender heap, not the global `operator delete`, and that
routing belongs to `srClass`: the identical 34-byte scalar deleting destructor sits at slot 5 of
first-party classes derived from `srClass` itself, from `srModel`/`srMeshModel`, from
`srTexture`/`srTextureIFace` and from `srNode`, so their common root is the only place it can come
from. Declaring `void operator delete(void*)` there reproduces the tail exactly - `mov ecx, [srHeap]`,
`push`, `call [srHeap::free]`, `mov eax, esi`, `pop`, `ret 4`, instruction for instruction against
`0x00492C40`.

The body itself stays unclaimed, because the complete destructor it calls is not recovered and the
compiler will not emit a deleting destructor for a class nothing constructs. And the complete destructor at `0x00492A30` opens a question the current model does not answer.
Across its 425 bytes it unregisters the instance three times, restoring a first-party vtable before
each - `0x005ECB6C`, then `0x005EBF68`, then `0x005EBF94` - before calling the imported
`srClass::~srClass`. `just wiz8 report class-family` puts every write in that family at `this+0x00`,
so this is single-inheritance vtable churn rather than subobjects, and the slot counts ascend 8, 11,
13, 13 in construction order the way an inheritance ladder does.

What the slots say is that the ladder is not stMaterial's:

| Table | Slots | Slot 0 | Slots 3 and 4 |
| --- | --- | --- | --- |
| `0x005EBF94` | 8 | `srMaterialIFace::sGetClassName` | `srClass::dump`, `srClass::verify` |
| `0x005EBF68` | 11 | `srMaterial::sGetClassName` | `srClass::dump`, `srClass::verify` |
| `0x005ECB6C` | 13 | local, stMaterial's | `srMaterial::dump`, `srMaterial::verify` |
| `0x005ECB38` | 13 | local, stMaterial's | `srMaterial::dump`, `srMaterial::verify` |

The first two are first-party classes that borrow SurRender's names for themselves and leave three
slots on the pure-virtual stub, so they are abstract stand-ins at the `0x2200` and `0x2210` registry
ids. They cannot be bases of stMaterial: their slots 3 and 4 hold `srClass`'s implementations, and a
derived class cannot replace an inherited method with a *different* class's. `0x005ECB38` is a
sibling of stMaterial rather than another level - identical but for slots 5, 6 and 8.

So the four stores are not one four-level chain, and what the constructor and destructor really walk
is still open.

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
