---
name: class-triage
description: Workflow for reviewing Wizardry 8 candidate classes and promoting them into the reviewed model. Use when picking the next class to recover, when decompiling an unnamed constructor/destructor and deciding what it constructs, when turning a Candidate_<vtable> into reviewed classes.csv/vtables.csv/vtable-slots.csv rows, or when interpreting candidate-class and translation-unit comments in Ghidra output.
---

# Class triage: from candidate vtable to reviewed class

The materialized Ghidra program carries a machine-derived **candidate layer** beside the reviewed
model: ~295 `Candidate_<vtable>` skeleton structs, `candidate-class` comments on every
constructor/destructor that writes a censused vtable, and `translation-unit` comments giving each
function and attributed global its bounded owning unit. It exists to make class recovery a
*fill-in-the-evidence* review instead of a hunt. **Nothing candidate-marked is evidence.** Never
cite a candidate value in an evidence column; every number is a hypothesis until the writers'
decompiles (and ideally a byte-exact port) confirm it.

## Picking a target

```sh
just wiz8 report class-candidates        # regenerates build/reports/class-candidates/
```

`candidates.csv` has one row per constructor-written vftable. Ranking heuristics:

- **Easy first**: a row with `scalar_deleting_destructor` set, one or two
  `constructor_or_destructor` writers, and an `allocation_size_hints` value is a dedicated-ctor
  class whose size is already known — usually one decompile session.
- `import_slots` high → the class derives from a SurRender base (compare
  `evidence/reviewed/wiz8/imported-vftable-sites.csv` and the slot `import_name`s in the
  polymorphism snapshot — they often spell the base outright).
- `pure_virtual_slots` > 0 → abstract base; expect derived classes among other candidates and no
  direct allocation hint of its own.
- `reviewed_class = yes` rows are done; the replay already drops them from the skeleton layer.

`families.csv` is the other entry point, and usually the cheaper one. It pairs two one-slot tables
a single writer stores with zero instruction displacement and sorts by how big that writer is. The
pair is a recovery lead, not inheritance evidence: first prove both receiver registers denote the
same complete object. Only then can lifecycle order identify a base and derived table. A small writer is a
dedicated constructor and ports in one sitting; a thousand-byte writer installing the same two
tables is a heap builder that happens to construct two objects on its way through, and the pair is
then a fact about its locals rather than a class waiting to be recovered. `writer_size` is the whole
distinction, and it is why the list is sorted by it.

Three of these families are recovered (`W8SoundEventVector005ED094`, `W8Vector005EBFB4`,
`W8Vector005EC16C`) and they came out byte-exact on identical declarations - a derived class adding
no member and no override, over a `W8GrowableVector<T*>` instantiation. If a family's constructor is
around eighty-four bytes and its deleting destructors are 44 and 30, copy one of those source files
and change the names; expect four exact bodies without iteration.

Once you have recovered one family, take the rest by fingerprint: join `families.csv` against
`evidence/snapshots/functions/candidates.csv` (sizes) and `calls.csv` (the complete destructor the
derived deleting destructor calls), and keep families whose four sizes match. **Calibrate against a
family you have already recovered, not against the sizes in your boundary rows** - the census sizes
a body by distance to the next body it identified, which makes the vector family `96/32/48/32` there
and `84/17/44/30` in reviewed terms. Filtering by the reviewed numbers matches nothing and reads
like "there are no more". The inflation is not constant either: the filler is nops, and the span can
swallow a body the census never identified, so a real match can be dropped by one wrong number.
Before calling a vein spent, re-measure with
`read_canonical_body(image, address, census_size).rstrip(b"\x90\xcc")` - that is what found the last
vector family.

Or work backwards from code: any decompile that shows a `[wiz8 observation:candidate-class:...]`
comment is telling you what the function you are reading constructs.

## Reading the program during review

- `just ghidra query wiz8--gog-base--wiz8--18a74ff61c65 decompile 0x<writer>` — the comment names
  each candidate the writer touches and its role. A writer listed on several candidates in one
  comment is usually a **heap builder** constructing unrelated objects in sequence, not a
  constructor of one big class.
- **Trust the in-program comments over the report's writer columns.** Outside Ghidra there is no
  reliable way to attribute a vptr write to a function: the census uses inter-function padding,
  which merges adjacent small bodies, and the tracked function census proposes starts *inside*
  real functions. Adjudicating the 295 canonical disagreements against Ghidra: padding right 55,
  function census right 119, neither right 118. The replay uses Ghidra's own containment, so the
  `candidate-class` comments are authoritative; the report's `constructor_or_destructor` column is
  a lead. Resolve any write site with
  `just ghidra query <program> function-of <site>,<site>`.
- **`just wiz8 report class-family <vtable>`** when a destructor restores a table that is not the
  one you expected. It prints every table in the family and, per writer function, which table goes
  through which raw store displacement. Normalize each receiver to the complete-object root before
  treating that displacement as a placement. Once that provenance is established, lifecycle order
  can distinguish base-to-derived churn, subobject teardown, and an inlined base destructor.
- In interactive Ghidra, retype `this` to `Candidate_<vtable>` (category `/wiz8/candidates`);
  virtual calls then render as `(*vptr[n])()` because every censused slot is typed
  `virtual_function *`. The struct's vptr fields and size are the census hypothesis you are
  testing.
- The `translation-unit` comment is a bounded interval attribution: trust it as "this code lives
  near unit X", not as identity. Functions in inter-anchor gaps carry none.

Facts to settle from the decompiles, in order of value:

1. **Which writers are constructors vs the complete destructor.** The census cannot separate
   them; destruction order (restores *own* vtable, then destroys members, then calls the base
   destructor last) vs construction order (base first, own fields, own vtable last) settles it
   immediately. The **base class comes free**: the constructor's first call is the base
   constructor, and if that function is another candidate's writer, that candidate is the base and
   its own allocation hint is the base extent — cross-check it against the offset where the
   derived class's first own field starts.
2. **Allocation size.** Confirm the hint at a `push N; call operator_new; ... call ctor` site.
   A hint can exceed the class's own extent — the `push` belongs to the *most-derived* class
   being constructed, so a hint on an abstract base's candidate is really a derived class's size.
3. **Polymorphic subobject placement.** The census records raw memory-operand displacements, not
   root-relative offsets. Follow register copies and `lea` adjustments from entry `this`; only a
   normalized placement may become a skeleton field. Lifecycle and dispatch evidence must then
   distinguish a base from an embedded polymorphic member.
4. **Slot-count boundary.** The census splits adjacent tables on code references; check the last
   slot's target is plausibly a method, not the next table's first entry.
5. **A name, only if the program provides one**: class-registry `getClassName` strings, assertion
   messages (`message` column of `evidence/observations/wiz8/assertions.csv`), source paths in
   slot bodies. Otherwise keep an address-qualified positional name (`W8DialogMember005DB1B0`
   pattern) — offsets are evidence, names are not.

## Promotion

`build/reports/class-candidates/promotion/candidate_<vtable>.md` prefills the row shapes:

1. Copy the rows into `evidence/reviewed/wiz8/classes.csv`, `vtables.csv`, `vtable-slots.csv`,
   replacing every `<placeholder>` from your review. Conventions: `W8`-prefixed or
   address-qualified names; `confidence` from {exact, high, strong}; `evidence` text says *how
   each fact was established*, citing writer addresses and sites; `layout_proof` stays **empty**
   until a byte-exact port proves offsets (see the `matching-decomp` skill for that loop).
2. Writers get `functions.csv` rows (plus `function-evidence.csv`) with honest `name_origin` —
   `descriptive` unless a string names them — and pointer fields with known pointees can use the
   `fields.csv` `pointee` column to type the graph.
3. If the rebase conflicts — it often will, several agents append to these tables —
   `just wiz8 resolve-evidence-conflict <file>` rather than merging by hand. It keeps the stronger
   row per identity, so a promotion the other side lacks is not silently demoted, and names the
   collisions in its summary. Then `just build WIZ8_GAMEPLAY_BOUNDARIES` **before**
   `just verify-boundaries`: that gate reads the objects on disk, so a rebase that pulled in
   another agent's sources will report `regressed` on a row you never touched until you rebuild.
   A lost promotion is invisible to the tests and visible only to that gate.
4. `just check` — the production repository validator cross-validates identities, provenance,
   sizes, slot contiguity, field overlap, references, exact digests, and reviewed observations.
5. Rematerialize (any `just ghidra query ...` after the evidence edit) and confirm
   `materialization.json` validates with zero failures. The promoted class leaves the candidate
   layer automatically.

## Header skeletons and the byte proof

`build/reports/class-candidates/headers/candidate_<vtable>.h` is a compilable starting struct
(vptr layout + size assert) for porting work. Copy it into owned source only as a draft, and
**name the ported struct after the reviewed class** — inventing a second name for a class the
reviewed model already names recreates the duplicate-model problem the repo spent a bead
eliminating. Replace `void* vptr` with real virtuals when the body makes virtual calls, keep
unreviewed regions opaque, and let `just verify-boundaries` falsify the layout.

Worked example, end to end: `W8Dialog005A80A0` (vtable `0x005EEF6C`) has its whole lifecycle
byte-proven — constructor 205/205, complete destructor 11/11, scalar deleting destructor 30/30,
slot 9 `Close` 61/61. Three compiler lessons from it, all reusable:

- An **empty derived destructor** over a correctly sized base emits exactly the canonical vtable
  restore and tail jump, so a body with no user code still proves the vptr offset and base extent.
- The **scalar deleting destructor is generated**, not written — matching it confirms the class is
  polymorphic with its destructor in slot 0.
- **Member initializers versus body assignments is visible in the output.** The constructor was
  205/205 bytes with exactly one instruction misplaced, the implicit vptr store sitting before the
  field stores instead of after. Moving the fields into the initializer list put them in the same
  scheduling group as the vptr store and the body went to `0 differing`. When size and instruction
  count match but one instruction is in the wrong place, look for a different *source shape*, not
  a different algorithm.

Never track or `#include` the generated header itself.

Registering the port: add the file to the `WIZ8_GAMEPLAY_BOUNDARIES` list in `CMakeLists.txt`,
mark the body `// FUNCTION: WIZ8 0x<ADDR>`, add a row to
`config/reccmp/wiz8-gameplay-boundaries.csv` (start at `structurally-strong`;
`just verify-boundaries` reports `promotable` when it is really exact), then set `exact` and fill
`relocation_masked_sha256` — the pinned test requires a 64-character hash on every exact row.
`just wiz8 diff-boundary <symbol>` shows the instruction-level alignment while iterating.

## Refreshing the layer

- The candidate layer regenerates from tracked inputs on every materialization; editing
  `evidence/` or anything under `tools/wiz8decomp/ghidra/` triggers the (~1 min, 51MB) rebuild.
- `just wiz8 evidence refresh polymorphism --update-snapshot` refreshes the census (and its
  `allocation_sizes`); `just wiz8 report data-segmentation --update-snapshot` refreshes the unit
  data intervals. Both need `WIZ8_WORK_DIR` binaries and gate byte-for-byte otherwise.
- Full background: `docs/wiz8-class-candidates.md`; the long-form procedure with the
  reasoning and failure modes is `docs/wiz8-class-recovery-procedure.md`.

## Pitfalls

- `scalar_deleting_destructor` empty does not mean there is none — it is only filled when the
  slot 0 target itself writes the vtable; many deleting destructors delegate to a complete
  destructor that does the writing.
- Two vtable writes with the same raw displacement are only a table-churn lead. Prove both receiver
  registers have the same complete-object provenance before inferring base-to-derived construction.
- A candidate whose writers are all large multi-object builders (the `srMaterial` pattern in
  `imported-vftable-sites.csv`) has no dedicated constructor to port; recover it from the builder
  or from its destructor instead.
- **No EH frame in the canonical is a hard constraint.** Under `/GX` VC6 adds an unwind frame to a
  destructor the moment something that can throw runs before a subobject still needs destroying,
  so a frameless canonical destructor means the original had no such window. If your port keeps
  growing a frame the target lacks, you are modelling subobject destructors the original does not
  have - port the class whose teardown has nothing to unwind first.
- **A null check before `operator delete` means the source said `delete p`**, not
  `::operator delete(p)`; the operator form does not null-check.
- **`families.csv` is built on census attribution, which is wrong for about a fifth of the write
  sites.** Before deciding a vein is exhausted, re-attribute: pull containment with
  `just ghidra query <program> function-of <comma-separated sites>`, run it through
  `object_model.attribute_writers`, and pass the corrected writes to `derived_families`. That
  removed six invented pairs and surfaced two real ones the census had split across two functions.
- **A raw displacement is not an object offset.** It becomes root-relative only when the receiver's
  affine provenance is known. A shifted register can make the displacement arbitrary or negative.
  Two tables in one body may still be an object and embedded member rather than a hierarchy;
  `0x0055D180` is the concrete warning case. Read the lifecycle and prove both receivers before
  promoting any family or subobject relationship.
- **Check `writer_role` before believing which table is the base.** A constructor stores the
  derived table last; a destructor stores it *first*, because the base destructor runs after the
  derived body. Getting that backwards inverts the hierarchy silently. A destructor-written pair
  also tells you the derived class has state - with nothing between the two stores the first would
  be dead and dropped.
- **Scattered addresses do not break a family.** VC6 emits every body as a per-unit COMDAT and its
  linker does not fold duplicates, so a destructor copy can survive far from its constructor. Tie
  the family together with the census - the derived table installed by that constructor and no
  other - not with adjacency.
- **Many writers is usually not ambiguity - sort them by size.** Nine functions install
  `0x005EC294`; eight are six-byte thunks or large bodies that inlined the constructor and one is
  the out-of-line copy, which is the only claimable body. Two writers of the *same* size are the
  case that really needs a file per emission.
- **Key a family on its deleting-destructor pair, not on constructor size.** A family whose only
  construction site inlined the constructor has no out-of-line copy to match, so a large writer can
  still be an ordinary family - `0x0057E5D0` is a 140-byte factory over a perfectly standard one.
- **`return p != 0;` and `if (!p) return 0; return 1;` are different bodies.** The first computes a
  flag, the second branches and returns literals. Two bytes and one instruction apart, and easy to
  misread as a frame problem.
- **If `diff-boundary` says the symbol is not in the objects, the body was never emitted.** VC6
  emits a class's vtable and destructors only in units that *construct* it, so a destructor-only
  port compiles to nothing. Port a constructing site from the same unit; list the object's symbols
  with `parse_coff_functions` to see what actually landed.
- **A complete destructor that stores another class's vtable is an empty derived class**, not a
  misattribution: the derived store is dead against the inlined base destructor and VC6 drops it.
  The constructor still writes both tables, so count a hierarchy from constructors.
- **Each `delete` in a destructor names its member's type shape**: through vtable slot 0 with
  `push 1` is a virtual destructor; a direct destructor call then `operator delete` is a
  non-virtual one; a null check then a bare `operator delete` is a *declared but empty* destructor;
  and a bare `operator delete` with no check is something trivially destructible. The last two
  differ by one instruction, so a destructor that is a few bytes short is often just a member typed
  as `unsigned char*` that should be a class with an empty destructor.
- Identical slot counts and identical allocation hints do **not** make two classes twins. Twins
  have identical *bodies* - check the write sites before planning to get two recoveries for one
  review.
- Promotion does not require copying new totals into a test or document. Run `just check`; the
  invariant validators consume the canonical tables directly and ordinary recovery progress does
  not require test maintenance.
- Do not hand-edit anything under `build/reports/` — regenerate; and do not promote a candidate
  wholesale "because the census says so": the census is the map, the decompiles are the territory.
