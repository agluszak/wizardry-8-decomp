# Candidate classes and bounded unit attribution in Ghidra

The materialized Ghidra program carries a machine-derived **candidate layer** on top of the
reviewed model. Everything in it is candidate-marked, derived deterministically from tracked
inputs during the `candidate_class_observations` replay phase, and validated on every
materialization. Nothing in it is reviewed evidence, and nothing in it may be cited as an
accepted identity.

## What the program shows

- **`/wiz8/candidates` structures.** One `Candidate_<vtable>` skeleton per unreviewed
  constructor-written vftable from the polymorphism census: the primary vptr at 0, one vptr per
  subobject vtable, opaque bytes to the allocation-size hint the census scanned at the
  constructors' call sites (`push size; call operator new`). Subobject vptrs pass a unanimity
  rule — every constructor writer of the class must install them — which separates real embedded
  subobjects from the unrelated objects a heap-builder function constructs in sequence. The
  structures are data types only; pick one on a `this` variable while triaging a writer.
- **`candidate-class` pre-comments** on every candidate writer function, stating its role per
  vtable: `candidate constructor-or-destructor of Candidate_...` or `candidate scalar deleting
  destructor of ...` (the slot 0 target that also writes the vtable). Allocation hints ride along.
- **`translation-unit` pre-comments** on every function inside an assertion-anchored `.text`
  unit interval and on every censused global inside a fitted data interval. These are *bounded
  interval attribution, not reviewed identity* — the comment says so — and the data side comes
  from the order-constrained fit snapshotted in
  `evidence/snapshots/data-segmentation/unit-data-intervals.csv`.
- **`virtual_function *` vtable slots** everywhere: the observation layer types all censused
  vftable slots through the shared `virtual_function` definition, so unreviewed virtual calls
  decompile as `(*vtbl[n])()` instead of casts through `void *`.

## Where it comes from

Tracked inputs only: the polymorphism snapshot (now carrying `allocation_sizes` per vftable),
the call-sites and globals snapshots, the reviewed assertion observations, and the
data-segmentation interval snapshot. The shared derivation cores live under
`tools/wiz8decomp/ghidra/` (`candidate_model.py`, `unit_intervals.py`) so they feed the
materialization key; the same cores drive `just wiz8 report class-candidates` and
`just wiz8 report data-segmentation`.

## Promotion workflow

1. Triage with `build/reports/class-candidates/candidates.csv` (or the writer comments met while
   decompiling). `just wiz8 report class-candidates` regenerates it plus, per unreviewed
   candidate, a promotion template under `promotion/` and a C++ struct skeleton under
   `headers/`.
2. Review the writers' decompiles. The candidate facts to confirm or refute: which writers are
   constructors vs the complete destructor, the allocation size, subobject offsets, and the
   slot-count boundary.
3. Land the accepted identity as reviewed rows — the promotion template prefills the
   `classes.csv`, `vtables.csv` and `vtable-slots.csv` shapes with the mechanical values and
   leaves the name, confidence and evidence for the review to supply. Writers go into
   `functions.csv` with provenance.
4. Rematerialize. The reviewed model takes the class over, and the candidate layer drops it
   automatically (reviewed vtables are excluded from skeleton generation).

The header skeletons are starting points for porting work only: they are generated, must not be
tracked or `#include`d as-is, and every field they contain is a hypothesis until a byte-exact
consumer proves it.
