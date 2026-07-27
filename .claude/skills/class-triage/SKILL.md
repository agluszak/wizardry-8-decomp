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
- In interactive Ghidra, retype `this` to `Candidate_<vtable>` (category `/wiz8/candidates`);
  virtual calls then render as `(*vptr[n])()` because every censused slot is typed
  `virtual_function *`. The struct's vptr fields and size are the census hypothesis you are
  testing.
- The `translation-unit` comment is a bounded interval attribution: trust it as "this code lives
  near unit X", not as identity. Functions in inter-anchor gaps carry none.

Facts to settle from the decompiles, in order of value:

1. **Which writers are constructors vs the complete destructor.** The census cannot separate
   them; destruction order (restores *base* vtables, then calls a base destructor last) vs
   construction order (bases first, own vtable last) settles it immediately.
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
3. `just test` — the reviewed-model loader cross-validates sizes, slot counts and field overlap,
   and `tests/unit/test_wiz8_source_model.py` pins counts you may need to bump.
4. Rematerialize (any `just ghidra query ...` after the evidence edit) and confirm
   `materialization.json` validates with zero failures. The promoted class leaves the candidate
   layer automatically.

## Header skeletons and the byte proof

`build/reports/class-candidates/headers/candidate_<vtable>.h` is a compilable starting struct
(vptr layout + size assert) for porting work. Copy it into owned source only as a draft: rename
per convention, replace `void* vptr` with real virtuals when porting a body that makes virtual
calls, and let `just verify-boundaries` falsify the layout. Never track or `#include` the
generated file itself.

## Refreshing the layer

- The candidate layer regenerates from tracked inputs on every materialization; editing
  `evidence/` or anything under `tools/wiz8decomp/ghidra/` triggers the (~1 min, 51MB) rebuild.
- `just wiz8 polymorphism --update-snapshot` refreshes the census (and its
  `allocation_sizes`); `just wiz8 report data-segmentation --update-snapshot` refreshes the unit
  data intervals. Both need `WIZ8_WORK_DIR` binaries and gate byte-for-byte otherwise.
- Full background: `docs/wiz8-class-candidates.md`.

## Pitfalls

- `scalar_deleting_destructor` empty does not mean there is none — it is only filled when the
  slot 0 target itself writes the vtable; many deleting destructors delegate to a complete
  destructor that does the writing.
- Two vtable writes at the *same* offset in one function are base→derived vtable churn during
  construction, not two subobjects.
- A candidate whose writers are all large multi-object builders (the `srMaterial` pattern in
  `imported-vftable-sites.csv`) has no dedicated constructor to port; recover it from the builder
  or from its destructor instead.
- Do not hand-edit anything under `build/reports/` — regenerate; and do not promote a candidate
  wholesale "because the census says so": the census is the map, the decompiles are the territory.
