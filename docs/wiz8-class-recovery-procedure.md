# Recovering a class end to end

The complete procedure for taking a class from an unnamed vtable address to a reviewed,
byte-proven entry in the model. It is written from the first full run, which recovered
`W8Dialog005A80A0` — a modal popup dialog whose constructor, complete destructor, scalar
deleting destructor and one virtual method are all relocation-masked exact against the original.

Two other documents sit either side of this one: `docs/wiz8-class-candidates.md` explains what the
candidate layer *is*, and the `class-triage` skill is the short operational checklist. This is the
long form, with the reasoning and the failure modes.

## 0. Orientation

Three sources of truth, in decreasing authority:

1. **The original image.** `just verify-boundaries` compares our compiled bytes against it with
   relocations masked. Nothing outranks this.
2. **The materialized Ghidra program.** Authoritative for *structure* — which function contains an
   address, what a body decompiles to — because it carries the reviewed model plus the whole
   observation replay.
3. **The census snapshots.** Exhaustive and cheap, but heuristic in places. They generate leads,
   never conclusions.

The candidate layer is category 3 presented through category 2. Its job is to make category 1
worth attempting on a specific class.

## 1. Pick a target

```sh
just wiz8 report class-candidates
```

Read `build/reports/class-candidates/candidates.csv`. The tractable shape is: a `slot0_target`
present, one or two `constructor_or_destructor` writers, an `allocation_size_hints` value, and few
`import_slots`. That combination means a dedicated constructor whose object size is already known.

Filtering for it is a five-line script; the run that produced `W8Dialog005A80A0` found five such
candidates out of 306 and took the one with the largest slot count, on the theory that a 15-slot
class is a real domain object rather than a container.

Cross-check the pick against `evidence/observations/wiz8/ptr-vector-instantiations.csv` — if the
vtable is in there, it is an instantiation of the growable-vector template and belongs to that
model instead.

## 2. Review the writers

```sh
just ghidra query wiz8--gog-base--wiz8--18a74ff61c65 decompile 0x<writer>
```

Settle these, in this order. Each one narrows the next.

**Which writer is which.** Construction order is base constructor, own fields, own vtable last.
Destruction order is own vtable restored first, then members, then the base destructor last. A
body that *only* restores a vtable and jumps to another function is a complete destructor; a body
that calls it and then conditionally calls `operator delete` is the scalar deleting destructor.

**The base class comes free.** The constructor's first call is the base constructor. If that
address is another candidate's writer, that candidate *is* the base, and its own allocation hint
is the base's extent. Confirm it against the offset where the derived class's first own field
starts — in the worked example both readings said `0x98`, and that agreement is what made the base
extent trustworthy before any port existed.

**Allocation size.** Confirm the hint at its call sites: `push N; call operator new; ...; call
ctor`. Not every site allocates — one of the three here reused an existing object — and that is
fine. Beware that the pushed size belongs to the *most-derived* class under construction, so a
hint attached to an abstract base is really some derived class's size.

**When a destructor restores the wrong vtable, read the whole family.** This is the single most
confusing thing the census can show you, and one command answers it:

```sh
just wiz8 report class-family <any-vtable-in-the-family>
```

It prints every table the family contains and, grouped by function, which table each writer
installs at which object offset. Construction and destruction order read straight off it:

- a function writing two tables **at the same offset** is a constructor installing a base and then
  the derived table, so the second one is the class's own;
- a function writing one table at a **non-zero** offset and another at zero is a destructor tearing
  down a subobject before its owner;
- a destructor that restores a table which is *not* its class's own is the giveaway that the
  derived vptr store was dropped as dead and what survives is an **inlined base destructor**.

That last case is what makes two bodies look contradictory. In the widget family, `0x004F3D90` and
`0x004F6640` both restore `0x005ED5BC` while touching different members, which reads as impossible
until the map shows `0x004F6640` also restoring a subobject at `+0x60` and the constructors
installing `0x005ED604` at zero: one is the base's own destructor, the other is a derived
destructor with the base's inlined into it.

**The slot boundary.** The census ends a vtable where a code reference points into it. Verify by
reading the raw table and checking xrefs to the address one slot past the end:

```sh
just ghidra query <program> --query "read-data 0x<vtable> 0x44" --query "xrefs-to 0x<end>"
```

Eleven code references to `0x005EEFA8` proved the 15-slot count in the worked example.

**A name, only if the program supplies one.** Class-registry `getClassName` strings, assertion
messages, source paths embedded in slot bodies. Otherwise use an address-qualified positional name
(`W8Dialog005A80A0`) and say what the class *does* in the evidence text instead. Offsets are
evidence; names are not.

> **Attribution warning.** Outside Ghidra there is no reliable way to say which function a vptr
> write belongs to. The census infers it from inter-function padding, which merges adjacent small
> bodies; the tracked function census proposes starts *inside* real functions. Adjudicating all 295
> canonical disagreements against Ghidra: padding right 55, function census right 119, neither
> right 118. The replay uses Ghidra's containment, so the in-program `candidate-class` comments are
> authoritative and the report's writer columns are leads. Resolve any site with
> `just ghidra query <program> function-of <site>,<site>`.

## 3. Promote into the reviewed model

`build/reports/class-candidates/promotion/candidate_<vtable>.md` prefills the row shapes. Fill
every `<placeholder>` from the review and append to:

- `evidence/reviewed/wiz8/classes.csv` — identity, the triad addresses, `minimum_size`,
  `base_classes` (spell the base as `unnamed +0x0 base (ctor …; dtor …; N bytes)` with
  `base_name_origin=address-qualified-positional`), and an `evidence` column that says *how each
  fact was established*. Leave `layout_proof` empty until a port proves it.
- `vtables.csv` and `vtable-slots.csv` — one vtable row, one row per slot.
- `fields.csv` — the vptr with `pointee=virtual_function *`, one row per established field, opaque
  runs as `bytes`. Offsets must not overlap and must fit the class size; the loader enforces both.
- `functions.csv` plus `function-evidence.csv` — the writers, with `name_origin=descriptive` unless
  a string names them, and `authority` matching the origin's ceiling.

Then:

```sh
just test
```

The reviewed-model loader cross-validates sizes, slot counts, contiguity and field overlap, so a
pass here means the rows are internally coherent. `tests/unit/test_wiz8_source_model.py` pins the
reviewed vtable and slot totals — it will fail with an off-by-your-new-rows count, and updating it
is part of the promotion, not a workaround.

Rematerialize by running any Ghidra query, then confirm the manifest:

```sh
just ghidra query wiz8--gog-base--wiz8--18a74ff61c65 function-of <any-address>
# then read validation.failure_count in the newest projects/*/materialization.json
```

Zero failures, and the candidate structure count drops by one: the class has left the candidate
layer and joined the reviewed model.

## 4. Port for the byte proof

This is where a hypothesis becomes evidence. Start from
`build/reports/class-candidates/headers/candidate_<vtable>.h`, but **name the ported class after
the reviewed class** — a second name for a class the model already names recreates the duplicate
-model problem the repo spent a whole bead eliminating.

Model the hierarchy honestly:

- The base as a real base class with a virtual destructor (extern, never defined) over opaque
  storage sized to the proven base extent.
- The derived class with pure-virtual placeholders so the method you are porting lands at its
  canonical slot. Eight placeholders put `Close` at slot 9.
- Callees as externs with any convenient name — call targets are relocation-masked, so only the
  instruction shape matters.

Write the body, add the file to the `WIZ8_GAMEPLAY_BOUNDARIES` list in `CMakeLists.txt`, mark it
`// FUNCTION: WIZ8 0x<ADDR>` immediately above the definition, and add a
`config/reccmp/wiz8-gameplay-boundaries.csv` row at `structurally-strong` with an empty hash.

```sh
just build WIZ8_GAMEPLAY_BOUNDARIES
just wiz8 diff-boundary '<Class>::<Method>' --all
```

`--all` prints the full aligned listing, which is what makes a near miss diagnosable. Lines marked
`~~` are relocation differences and are expected noise; `>>` marks a real difference.

When it reports `0 differing`, compute the masked digest and promote the row to `exact`:

```python
from wiz8decomp.boundaries import (
    collect_object_candidates,
    masked_digest,
    read_canonical_body,
    resolve_boundary_function,
)

canonical = read_canonical_body(image, address, size)
fn = resolve_boundary_function(collect_object_candidates(objects).get(symbol, ()), size, canonical)
masked_digest(fn, size)
```

Every `exact` row must carry a 64-character hash; a pinned test enforces it. Finish with
`just verify-boundaries` (exit 0), `just check-markers`, and `just test`, then record the proof in
the class's `layout_proof` and upgrade the writers' `functions.csv` confidence to `exact` with
their hashes.

## 4a. When the evidence tables conflict

They will. Several agents append to `classes.csv`, `functions.csv` and the boundary map at once,
so a rebase lands on the same tables routinely. Do not merge them by hand:

```sh
just wiz8 resolve-evidence-conflict config/reccmp/wiz8-gameplay-boundaries.csv
```

Keeping both halves is right for rows only one side has, and wrong the moment both sides carry the
same identity — appending both duplicates it, and taking whichever came last can silently demote a
row somebody already promoted. That happened here: a boundary row went back to
`structurally-strong` and lost its recorded hash, and only `just verify-boundaries` noticed, after
the push. The resolver keys each table by the columns that name its rows, keeps the stronger
confidence on a collision, prefers a recorded hash at equal confidence, and lists in its summary
exactly which identities appeared on both sides so a demotion cannot pass unremarked.

Then **rebuild before verifying**, and re-run `just test` and `just verify-boundaries` before
pushing. A merge that loses a promotion is invisible to the test suite and visible only to the
boundary gate — and that gate compares against whatever objects are currently on disk, so a rebase
that brings in another agent's sources leaves it reading stale ones. The symptom is a `regressed`
verdict on a row you never touched; the cause is usually that you have not built since the rebase,
not that anything is wrong with their work.

## 4b. Mining a shape, and knowing when it is spent

Some teardowns are nearly free: an empty derived destructor over a base that has one compiles to
exactly two instructions - restore the class vtable, tail-jump to the base destructor - and the
compiler generates the scalar deleting destructor beside it. Two byte-exact bodies for a
declaration with no statements in it.

That makes it worth asking how many such classes remain, rather than meeting them one at a time.
The body is a fixed encoding, so scan for it:

```python
# mov dword ptr [ecx], imm32 ; jmp rel32
data.find(
    b"\xc7\x01", text.raw_offset, text.raw_offset + text.raw_size
)  # then check data[off+6] == 0xE9
```

The whole image holds **six**, of which one is an adjustor thunk for another. So the shape is close
to exhausted and is not a seam worth returning to - which is exactly the useful answer, and cheaper
to establish once than to rediscover per class. Prefer this over sampling whenever the thing you
are looking for has a fixed encoding: the census bounds vtables and writers, but a body shape is
just bytes.

## 5. What the compiler will teach you

The build is a falsifier, and its complaints are recovered facts about the original source. Three
from this run:

**An empty destructor proves a layout.** Declaring the derived destructor with an empty body over
a base of the proven size emitted exactly the canonical two instructions — restore the class
vtable, tail-jump to the base destructor, 11 of 11 bytes. Over a differently sized base it would
not. So a body with no user code still proved the vptr offset and the base extent.

**The compiler writes bodies you never write.** The scalar deleting destructor at `0x005A8170` was
generated from the virtual destructor declaration alone and matched 30 of 30 bytes. Its matching
confirms the class is polymorphic with its destructor in slot 0 — a fact no hand-written body
could establish.

**Member initializers versus body assignments is visible in the output.** The constructor came in
at 205 of 205 bytes with a single instruction misplaced: the implicit vptr store sat *before* the
two field stores, where the original has it after. Moving the two fields from body assignments
into the constructor's initializer list put them in the same scheduling group as the vptr store,
VC6 ordered all three as the original does, and the body went to `0 differing`. That is not a
build tweak — it recovers that the original constructor used an initializer list.

The general lesson: when a body is the right size and instruction count but one instruction sits in
the wrong place, stop looking for a different algorithm and start looking for a different *source
shape* expressing the same algorithm.

**A missing EH frame is a constraint, not a detail.** Under `/GX` VC6 emits an unwind frame in a
destructor as soon as something that can throw runs *before* a subobject still needs destroying.
So a canonical destructor with no frame at all says the original had no such window: either the
body is empty, or there are no subobject destructors to unwind. Three attempts at the widget
derived destructor kept growing an EH frame my target does not have, because every arrangement put
a `delete` ahead of an inlined base destructor. The class that did port - the base itself - has a
body with nothing to unwind, and matched at 52 of 52 bytes first try.

Related: a **null check before `operator delete`** means the source wrote `delete p`, not
`::operator delete(p)`. The operator form does not null-check, and that one instruction is enough
to tell them apart.

**Two tables in one body are not always a hierarchy at all.** They are a hierarchy when both go
over the same vptr, and an object with an embedded member when they go to different offsets - and the
census does not always record the offsets correctly. `0x0055D180` stores `0x005EE8F0` over its own
vptr and `0x005EE8F8` four bytes in, but both writes are recorded at offset zero, so the pair reads
as a perfect base-and-derived. The constructor at `0x0055CFD0` records the member's `0x4` correctly,
and that is enough to rule the pair out; `derived_families` now drops any pair some body places at
different offsets.

The guard needs a body that got the offsets right, so it is not a proof of derivation - only a way
to catch the case that is provably not one. Before porting a family whose writer is a teardown body,
read the decompile and check both stores really go to `this` and not to `this + n`.

**Which of a pair is the base depends on what the writer is.** A constructor runs base-first, so
the table it stores *last* is the derived class. A destructor runs the other way - it stores its own
table, runs its own body, and only then does the base destructor store the base table - so there the
*first* store is the derived class. Reading a teardown body as a constructor inverts the hierarchy,
and the inversion is silent. `families.csv` carries a `writer_role` column for this.

The test for a teardown body has to be relative to the table being written: the writer either *is*
that table's slot-0 target, or is called by it. "Called by some deleting destructor anywhere" is far
too loose - it catches ordinary constructors reached from an unrelated teardown path, and three
already-recovered families were briefly mislabelled that way.

Note also when a pair survives destruction at all. Usually it does not: with nothing between the two
stores the first is dead and VC6 drops it, which is the whole reason an empty derived destructor
stores only the base table. A destructor-written pair means the derived body does something between
them - releasing a member, typically - so seeing one at all tells you the derived class has state.

**Let the family ranking pick the target where it can.** `just wiz8 report class-candidates`
emits `families.csv` beside `candidates.csv`: every pair of one-slot tables one writer installs at
offset zero, ordered by that writer's size. The order is the point - it separates a dedicated
constructor from a large body that merely builds two objects while doing something else, which is
the distinction that decides whether a family is an afternoon or a dead end. Three families off the
top of that list ported byte-exact on identical declarations without iteration.

Once a shape is proven, select the rest of it by fingerprint rather than by eye. Join `families.csv`
against `evidence/snapshots/functions/candidates.csv` for body sizes and `calls.csv` to find the
complete destructor the derived deleting destructor calls, then keep the families whose four sizes
match a family already recovered. Seventeen matched the vector shape that way; twelve of those had
all four bodies in one address neighbourhood, which is what a single unit emitting a family looks
like, and all forty-eight bodies came out byte-exact in one build.

**Correct the writer attribution before trusting `families.csv`.** The census guesses which
function contains a vptr write from inter-function padding, and it is wrong for 241 of the 1176
canonical sites. `object_model.attribute_writers` re-attributes them against Ghidra's own
containment, which `just ghidra query <program> function-of a,b,c` supplies in bulk. Feeding the
corrected writes to `derived_families` drops six pairs the census invented and adds two it hid -
both of the latter turned out to be the vector shape and ported byte-exact. The report still reads
the census directly, so this is a manual step until it is wired in.

**A family whose four bodies are scattered across the image is still one family.** VC6 emits each
of them as its own COMDAT in every unit that needs it, and its linker does not fold duplicates, so
the surviving copy of a destructor can sit far from the constructor. Address adjacency is a nice
confirmation when you have it and not evidence of anything when you do not. What ties a family
together is the census: check the derived table is installed by that constructor and by nothing
else.

**When it is installed by several, sort them by size before concluding anything.** Nine functions
install `0x005EC294`, which looks like ambiguity and is not: eight are six-byte thunks or large
bodies that inlined the constructor, and exactly one is an out-of-line copy of it. That is what an
inline constructor looks like when VC6 emits a copy for some call sites and folds it into others,
and only the out-of-line copy is claimable. Two writers of the *same* size are the case that
genuinely needs two source files, one per emission - check which you have rather than assuming.

**Those two size sources do not agree, and the difference is not an error.** The function census
sizes a body by the distance to the *next body it identified*; Ghidra and the reviewed rows state the
body itself. The vector family is `84/17/44/30` in reviewed terms and usually `96/32/48/32` in census
terms. Calibrate a fingerprint against a family you have already recovered rather than against the
sizes you wrote in the boundary rows - filtering census sizes by reviewed numbers silently matches
nothing, which reads like "no more of these exist".

Calibrating is not enough on its own, because the census inflation is not constant. The filler
between bodies is nops, not the `0xCC` the phrase "padding" suggests, and the span can swallow a body
the census never identified: after `0x005178C0` an eighteen-byte body sits in the gap, so a
thirty-byte deleting destructor is recorded as sixty-four. That one family passed every other test
and was dropped by its one inflated number. Confirm a spent vein by measuring real bodies -
`read_canonical_body(image, address, census_size).rstrip(b"\x90\xcc")` - rather than by trusting the
census twice.

**Identify a family by its destructors, not by its constructor.** A fingerprint keyed on an
eighty-four byte out-of-line constructor misses every family that has no out-of-line constructor at
all, because the only construction site inlined it. `0x0057E5D0` is one: a 140-byte factory that
heap-allocates the list, constructs it in place with the default capacity and stores it in a
file-scope pointer. So a writer much larger than a constructor is not automatically a heap builder
to be skipped - the pair of deleting destructors at 44 and 30 bytes identifies the shape either way,
and that is the more reliable key.

**A byte-valued return that is one instruction shorter than the canonical is a collapsed branch.**
`return pointer != 0;` computes the flag; `if (!pointer) return 0; return 1;` tests it and returns a
literal from each arm. The second is what the original wrote in that factory, and the difference is
two bytes and one instruction - the sort of residual that looks like a frame or calling-convention
problem and is neither.

**A destructor alone may compile to nothing at all.** VC6 emits a class's vtable, its
compiler-generated deleting destructor and its out-of-line complete destructor in the translation
units that *construct* the class - not in the ones that merely destroy it. Port only a destructor
and the object file can come out empty, and `diff-boundary` will report the symbol as missing
rather than as wrong, which reads like a naming mistake and is not one. Check what actually landed:

```sh
uv run python -c "
from pathlib import Path
from wiz8decomp.boundaries import parse_coff_functions
for f in parse_coff_functions(Path('build/decomp/CMakeFiles/<TARGET>.dir/<path>.obj')):
    print(len(f.body), f.name)"
```

The fix is to port a constructing site from the same unit in the same commit. That is usually the
right scope anyway, because a constructor fixes offsets a destructor only hints at - and the sizes
in that listing are themselves evidence, since a body that comes out at the canonical byte count
before you have diffed it is a strong sign the declaration is right.

**A derived class that adds nothing still leaves a trace, but only on the constructor side.**
Its constructor installs the base table and then its own; its destructor installs only the base's,
because the derived store is immediately overwritten by the inlined base destructor and VC6 drops
it as dead. A complete destructor that stores a *different* class's vtable is therefore not a
contradiction and not a misattribution - it is the ordinary encoding of an empty derived
destructor. Count vtables from constructors; destructors undercount.

**Each `delete` names the shape of what it destroys**, which makes a destructor that releases
several members the densest layout evidence available. Three forms to read apart, all present in
`W8Prop005EC1E0::~W8Prop005EC1E0`:

| Emitted | What the member points at |
| --- | --- |
| null check, load vtable, `push 1`, call slot 0 | a class with a **virtual** destructor |
| destructor called directly, then `operator delete` | a class with a **non-virtual** destructor |
| null check, then a bare `operator delete` | a class with a **declared but empty** destructor |
| bare `operator delete`, no check | something **trivially destructible** — `unsigned char*` and friends |

The last two are one instruction apart and easy to conflate. That port sat at 137 bytes against 141
until the `+0x20` member changed from `unsigned char*` to a class with an empty declared
destructor: a trivially destructible pointer lets VC6 drop the null check, while a declared
destructor keeps it and then inlines to nothing.

The constraint cuts the other way too, and that is how it earns its keep. `W8Control005ED654`'s
constructor first came out 122 bytes against 85, entirely because its base had a declared
destructor and the embedded vector's `operator new` can throw after the base is built. Making the
base destructor implicit dropped the frame and the body went to 90 bytes and the right instruction
count in one step. **So a frameless canonical constructor is positive evidence that a base
destructor is implicit** - not merely a modelling convenience.

**Count the vtable stores at one offset.** Two stores to the same object offset in a constructor
are a base table followed by a derived one, so a member showing two stores is a two-level object,
not a single class. The last five bytes of that same control came from modelling its embedded
vector as one `W8GrowableVector<T>` when the family map plainly showed `0x005ED660` then
`0x005ED65C` both landing at `+0x10`; deriving a small class from the template so both tables get
installed took it to exact.

**But know when to stop.** The shared dialog base's constructor is the counter-example: the same
six-before/four-after split does move its vtable store to the canonical position, and it costs more
than it buys, because VC6 then materializes the zero constant into EAX at first use where the
original holds it in ECX from the top of the function. Which register a constant lives in, and how
early, is not something source shape directs — this repository already records that residual for
`Function4E3340` and `Function54B560`. The arrangement that keeps all nineteen instructions and
both constants correct is the one to keep, recorded as `structurally-strong` with the residual
written out, rather than a worse arrangement that happens to fix one instruction.

## 6. Worked result

`W8Dialog005A80A0`, vtable `0x005EEF6C`, `0xa0` bytes over a `0x98` unnamed base, 15 slots:

| Address | Body | Bytes |
| --- | --- | ---: |
| `0x005A80A0` | constructor | 205/205 |
| `0x005A8190` | complete destructor | 11/11 |
| `0x005A8170` | scalar deleting destructor | 30/30 |
| `0x005A81A0` | `Close`, vtable slot 9 | 61/61 |

Between them they prove the vptr at `0x0`, the `0x98` base extent, both own fields at `0x98` and
`0x9c`, and two base byte fields at `0x54` and `0x55` — each by more than one independent route,
which is why the class is recorded at `exact` rather than `strong`.
