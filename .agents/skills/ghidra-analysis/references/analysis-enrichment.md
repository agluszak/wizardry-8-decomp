# Analysis enrichment roadmap

Decompiler quality for Wiz8 is primarily an analysis-database problem. Enrich the
reviewed program with high-confidence facts before asking Ghidra to decompile.
Pretty-printer and option tweaks are secondary.

## Evidence boundary

Known source/retail facts may enter the reviewed program. Speculative Param-ID,
RTTI, or heuristic guesses must not land in reviewed GZF state without an
explicit, measured promotion step. Soft inferences belong on disposable copies
or behind dry-run reports until validation separates safe application, preserved
recovery behavior, and useful improvement.

**A category path is organization, not provenance.** Do not maintain two mutable
runtime type graphs (for example root PDB Structures and a parallel
`/wiz8/classes` copy set). One authoritative live Structure per class, bound to
its `GhidraClass` namespace the way Ghidra and reccmp already expect.

## Class binding (foundation)

Ghidra types automatic `this` through
`VariableUtilities.findOrCreateClassStruct(function)` from the function's
`GhidraClass` parent. reccmp places ordinary classes at a namespace-based
category (typically `/ClassName`) and creates the matching class namespace.

Enrichment must:

1. Ensure the source class identity maps to that `GhidraClass`.
2. Ensure the Structure `findExistingClassStruct` returns is the reviewed layout.
3. Keep dynamic storage so auto `this` picks up that Structure.
4. Keep `VtableResolver.classNamespace()` able to resolve the same class
   (leaf-name fallback when category mapping is ambiguous).

**Collect/plan is read-only.** `collect_*` helpers use `find_ghidra_class` and
emit `missing-class` / `create-class` actions; `ensure_ghidra_class` runs only
inside apply transactions.

Custom storage is an exceptional ABI operation, not the normal way to pick a
class Structure. Ordinary `class-this-typing` **skips** functions that already
have custom variable storage (`skip-custom-storage`) and never calls
`setCustomVariableStorage(False)` to silence ABI. Explicit convention
hard-disagreements (same classification as `prototype-repair`) are skipped
rather than overridden to `__thiscall`.

Synthetic `wiz8::classes::…` namespaces must not be invented to justify a
directory layout.

Acceptance cases (ordinary method, derived, secondary-base) should bind without
custom storage. Full ProgramBuilder / lifecycle-fixture coverage is required for
those shapes; CI gates that via ``tests/ghidra/test_class_binding_integration.py``
after the recovery lifecycle self-test. Unit tests document the policy and skip
cleanly when the fixture project is unavailable (see ``tests/unit/test_class_binding.py``
and ``tests/unit/test_enrichment_ghidra.py``).

## Ordered work

| Step | Goal | Status |
| --- | --- | --- |
| Foundation | Source class ↔ `GhidraClass` ↔ Structure binding; auto `this` without custom storage | `class_binding` + rewritten `class-this-typing` / `class-structures`; fixtures present |
| 10 | Objective decompiler-quality benchmark (oracle + pain) | `wiz8 analyze decompiler-quality` |
| 1 | Calling-convention / prototype repair before Param ID | `prototype-repair` (source-backed); heuristics report-only except census vtable thiscall |
| 2 | Owned enrichment candidate (unique run dir, outcome split) | `enrichment-checkpoint` (disposable by default) + `enrichment-promote` |
| 5–8 | Globals, callbacks, CF, attributes | consumers retargeted onto `class_binding`; see consolidation below |
| 9 | Dual decompiler profiles | Python `semantic.py` profiles; Java recovery still uses program options |

## Promote candidate

Disposable trials write all authoritative artifacts under
`build/enrichment-checkpoint/run-<id>/`:

- `ghidra-project/`
- `before.json` / `after.json` / `delta.json` / `pain-*.json`
- `report.json`, `input-manifest.json`
- `candidate.gzf` + `candidate.sha256` (frozen immediately when the disposable
  trial finishes — promote consumes these exact files and does **not** re-pack
  a mutable project later)
- per-pass reports under `steps/`

Prefer `build/` over `work_dir`: Ghidra rejects project path elements that start
with `.` (common for work dirs like `.wiz8-work`). `uv run wiz8 analyze
enrichment-promote --from-latest` searches the same `build/enrichment-checkpoint/`
tree.

Report `outcomes`:

- `safe_application` — no unexpected apply errors, import ok, inputs matched
- `preserved_recovery` — True only when a mutated run measured quality/pain and
  reported no decompiler regression; `null` when mutation happened without
  measurement (not promotable)
- `useful_improvement` — `null` if unmeasured; `true` only when measured debt/pain
  improved with evidence. Applied-row count alone does **not** make this true.
  Promotion may still be manually accepted when usefulness is inconclusive.

CLI `ok` remains `safe_application is True and preserved_recovery is True`
(nonzero exit when false). Unmeasured mutated candidates freeze but are not
promotable.

When a disposable trial is accepted, promote **that** candidate — do not rerun
with `--live`:

```sh
uv run wiz8 analyze enrichment-promote --from-latest
# or:
uv run wiz8 analyze enrichment-promote build/enrichment-checkpoint/run-<id>
```

Promote verifies `candidate.gzf` against `candidate.sha256` (and report
provenance: seed/binary/source-index/Ghidra/reccmp git pin/source-tree, plus PDB
when source import was used or the candidate recorded a PDB hash). Missing
current provenance for a required field is a refusal, not a pass. Staging and
aside Ghidra projects live under `build/enrichment-promote/` (not `work_dir`).
It restores into a staging directory, verifies, then swaps the live project
aside. If a live project directory already exists, promotion requires `--force`;
seed freshness `current` is not treated as “untouched.” Promote does **not**
refresh vendor GZF seeds.

## Consolidation direction

Items below are the advanced follow-through after the `#212`/`#218`/`#219`
binding pivot. Parser template/qualifier bugs on the Wiz8 side are closed
(digit-only array extents, order-independent qualifier stripping). **reccmp
importer gaps remain upstream** and are not fixed by bumping the pin here:
`LF_PROCEDURE` → void, missing-union write, and `T_NOTYPE` variadic rejection.
Those gaps are why curated `callback_typing` / `function_attributes` still
exist; do not expand them as a second declaration source.

1. **Class binding first** — ordinary, derived, and secondary-base cases without custom storage. *(done)*
2. **Compiler projection** — extend reccmp importer gaps (callbacks/unions/variadics) upstream; retire overlapping handwritten declaration parsers only after that lands.
3. **Type graph projection** — identity map + field reconciliation onto bound Structures (`wiz8 analyze type-graph`). Nested Pointer/Array/Structure/FunctionDef refs remap through `class_binding`; equal-richness disagreements are `conflict` (never richer-wins). Opaque shells + rich evidence → `reconcile-fields`. *(done)*
4. **Legacy `/wiz8/classes` cleanup** — dry-run inventory always (`wiz8 analyze legacy-classes-cleanup`); gated apply with `--apply` or enrichment `--cleanup-legacy-classes`. Refuses size/shape mismatches and bound-still-legacy paths; never vendor GZF rewrite. *(done)*
5. **Vtable slots as ABI declarations** — census extents preserved (unresolved slots marked, never silently truncated). Construction and agreement share `desired_slot_contract` (source declaration, else source-backed live function, else callee). Unresolved source types skip the table rather than silently demoting to analysis. Parameter names are not ABI. Namespace-safe `/wiz8/vftables/…` paths landed; secondary/construction/for-clause vtables and true subobject slot ABI remain deferred (see Deferred).
6. **One runner owns the experiment** — unique run directory, shared open `Program`, promote the tested candidate (not an unverified `--live` rerun).
7. **Validation** — safe application / preserved recovery / useful improvement as separate outcomes.

### How to run remapper + cleanup

```sh
# Dry-run identity + field plan
uv run wiz8 analyze type-graph

# Apply shells / reconcile-fields / remap-nested onto bound Structures
uv run wiz8 analyze type-graph --apply

# Inventory leftover /wiz8/classes (default dry-run)
uv run wiz8 analyze legacy-classes-cleanup

# Gated delete after replaceDataType (prefer disposable enrichment first)
uv run wiz8 analyze enrichment-checkpoint --apply-enrichment --cleanup-legacy-classes
uv run wiz8 analyze enrichment-promote --from-latest
```

## Deferred deliberately

- **Upstream reccmp** `LF_PROCEDURE` / union / variadic import (and any pin bump).
- **RTTI on `Wiz8.exe`**: retail is `/GR-`. Optional probes on RTTI-bearing modules only.
- **FID as DB enrichment**: after prototypes and class bindings land.
- Retiring all legacy path fallbacks from `global_typing` / `callback_typing` once a promoted seed is clean.
- **Secondary / construction / for-clause vtables** and true base-subobject virtual-slot ABI (primary source-index `vtable_address` tables only for now).

## Enrichment discipline

1. Repair obvious calling conventions and known signatures from compiler-backed facts.
2. Bind classes so auto `this` and the exporter share one Structure.
3. Only then run Decompiler Parameter ID on disposable candidates.
4. Project globals/callbacks/vftables through the same identity map.
5. Promote the exact tested candidate; do not treat a second live apply as equivalent.
