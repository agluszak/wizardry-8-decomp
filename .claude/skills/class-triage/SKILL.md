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
  in at which object offset. A writer installing two tables at the *same* offset is a constructor
  doing base-then-derived; a writer installing one at a non-zero offset and one at zero is a
  destructor tearing down a subobject before its owner; and a destructor restoring a table that is
  not its own class's means the derived vptr store was dropped as dead and what you are reading is
  an **inlined base destructor**. Two bodies restoring the same table while touching different
  members is that case, not a contradiction.
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
3. **Subobject offsets.** Skeleton vptrs beyond +0 passed a unanimity rule (every ctor writer
   installs them), but confirm against the constructor body; a blank there does not mean none
   exist — a single shared writer can hide them.
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
4. `just test` — the reviewed-model loader cross-validates sizes, slot counts and field overlap,
   and `tests/unit/test_wiz8_source_model.py` pins counts you may need to bump.
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
- `just wiz8 polymorphism --update-snapshot` refreshes the census (and its
  `allocation_sizes`); `just wiz8 report data-segmentation --update-snapshot` refreshes the unit
  data intervals. Both need `WIZ8_WORK_DIR` binaries and gate byte-for-byte otherwise.
- Full background: `docs/wiz8-class-candidates.md`; the long-form procedure with the
  reasoning and failure modes is `docs/wiz8-class-recovery-procedure.md`.

## Pitfalls

- `scalar_deleting_destructor` empty does not mean there is none — it is only filled when the
  slot 0 target itself writes the vtable; many deleting destructors delegate to a complete
  destructor that does the writing.
- Two vtable writes at the *same* offset in one function are base→derived vtable churn during
  construction, not two subobjects.
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
- Identical slot counts and identical allocation hints do **not** make two classes twins. Twins
  have identical *bodies* - check the write sites before planning to get two recoveries for one
  review.
- Promotion bumps pinned counts: `tests/unit/test_wiz8_source_model.py` asserts the reviewed
  vtable and slot totals, so `just test` will fail with an off-by-your-new-rows count until you
  update it. That failure is the model loader confirming your rows loaded, not a problem.
- Do not hand-edit anything under `build/reports/` — regenerate; and do not promote a candidate
  wholesale "because the census says so": the census is the map, the decompiles are the territory.
