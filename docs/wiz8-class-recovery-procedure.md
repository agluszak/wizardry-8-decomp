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
    collect_object_candidates, masked_digest, read_canonical_body, resolve_boundary_function)
canonical = read_canonical_body(image, address, size)
fn = resolve_boundary_function(collect_object_candidates(objects).get(symbol, ()), size, canonical)
masked_digest(fn, size)
```

Every `exact` row must carry a 64-character hash; a pinned test enforces it. Finish with
`just verify-boundaries` (exit 0), `just check-markers`, and `just test`, then record the proof in
the class's `layout_proof` and upgrade the writers' `functions.csv` confidence to `exact` with
their hashes.

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
