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
| 9 | Dual decompiler profiles | Python `semantic.py` profiles; Java `RecoveryEngine` uses the recovery profile (no `grabFromProgram`) |

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
uv run wiz8 analyze enrichment-promote --from-latest --replace-live
# or:
uv run wiz8 analyze enrichment-promote build/enrichment-checkpoint/run-<id> --replace-live
```

Promote verifies `candidate.gzf` against `candidate.sha256` (and report
provenance: seed/binary/source-index/Ghidra/reccmp git pin/source-tree, plus PDB
when source import was used or the candidate recorded a PDB hash). Missing
current provenance for a required field is a refusal, not a pass. Staging and
aside Ghidra projects live under `build/enrichment-promote/` (not `work_dir`).
It restores into a staging directory, verifies, then swaps the live project
aside. If a live project directory already exists, promotion requires
`--replace-live` (or `--force` as an alias). That flag does **not** bypass
provenance. Seed archive hash and retail binary hash are never bypassable.
`--allow-provenance-mismatch` is a separate permission for Ghidra/reccmp/
source-tree/PDB disagreements. Seed freshness `current` is not treated as
“untouched.” Promote does **not** refresh vendor GZF seeds.

## Consolidation direction

Items below are the advanced follow-through after the `#212`/`#218`/`#219`
binding pivot. Parser template/qualifier bugs on the Wiz8 side are closed
(digit-only array extents, order-independent qualifier stripping). **reccmp
importer gaps for `LF_PROCEDURE`, unions, trailing `T_NOTYPE` variadics, and
`T_BOOL08` are fixed upstream** (pin `76ba6f9a` on the current matching lineage). Curated
`callback_typing` / `function_attributes` still exist as audit/fallback for
already-named field sites; compiler-backed `Pointer(FunctionDefinition)` fields
are left alone (`compiler-backed`). Do not expand callback families. Resolve
Wizardry callback field sites through class identity → `GhidraClass` →
`find_class_structure()`, never `/wiz8/classes`. Keep `/_GUI_BUTTON` and
`/_MOUSE_REGION` as absolute paths. `_field_already_typed` compares
FunctionDefinition ABI contracts (parameter names are not ABI).

1. **Class binding first** — ordinary, derived, and secondary-base cases without custom storage. *(done)*
2. **Compiler projection** — extend reccmp importer gaps (callbacks/unions/variadics) upstream; retire overlapping handwritten declaration parsers only after that lands.
3. **Type graph projection** — identity map + field reconciliation onto bound Structures (`wiz8 analyze type-graph`). Nested Pointer/Array/Structure/Union/FunctionDef refs remap through `class_binding`; equal-richness disagreements are `conflict` (never richer-wins). Opaque shells + rich evidence → `reconcile-fields`. *(done)*
4. **Legacy `/wiz8/classes` cleanup** — dry-run inventory always (`wiz8 analyze legacy-classes-cleanup`); gated apply with `--apply` or enrichment `--cleanup-legacy-classes`. Always `replaceDataType(legacy, bound, False)` onto the bound replacement. The third argument is `updateCategoryPath`: `True` *moves* `/stLight` to `/wiz8/classes/stLight` instead of merging. Leftover types after a real replace are apply errors, never raw `remove()`. Refuses size/shape mismatches and bound-still-legacy paths; never vendor GZF rewrite. *(done)*

`stLight::vInstance` (`0x0049e3a0`) pain +1 was that false move plus a later leftover `remove()`, which untyped `this` to `undefined4` so the decompiler emitted `(void *)0x0`. Source is `return new stLight(0);`. With `updateCategoryPath=False` and a `/stLight` path lookup even when the parent is still a plain Namespace, the leftover copy disappears and `this` becomes `/stLight *`. `ensure_ghidra_class` is a separate step so auto `this` can bind. Do not add +1 tolerance.

Reviewed seed still has other split identities this merge does not paper over: agreeing `/wiz8/classes` duplicates (replaceable), field/size mismatches (conflict), and leftover copies with no non-legacy `/ClassName` Structure yet.
5. **Vtable slots as ABI declarations** — census extents preserved (unresolved slots marked, never silently truncated). Construction and agreement share `desired_slot_contract` (source declaration, else source-backed live function, else callee). Unresolved source types skip the table rather than silently demoting to analysis. Parameter names are not ABI. Namespace-safe `/wiz8/vftables/…` paths landed; source-index `base_vtables` (for-clause / secondary base) are typed. Secondary slot `this` is the Base subobject (or a proven Derived ComponentOffset), not the Derived implementation body. Derived-specific subobject views live under `/wiz8/subobjects/Derived/Base_at_0x20` and replace the Base component inside Derived; canonical `/Base` stays untouched. Unmarked construction-phase tables that share a census construction family with a marked table are typed as `Class_vftable_ctor` and do not retarget the complete-object `vfptr`. Secondary `vfptr`/`vbptr` fields retarget only at a unique incoming-ECX offset. Confirmed vbtables installed by the same lifecycle function as a marked vftable are typed under `/wiz8/vbtables` as integer displacements; existing `VBasePtr`/`o_*` PointerTypedefs at `vbptr + displacement` may receive a ComponentOffset, but those typedefs are never invented.
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
uv run wiz8 analyze enrichment-promote --from-latest --replace-live
```

## Follow-through (landed with this architecture)

1. **reccmp `LF_PROCEDURE` / `LF_MFUNCTION`** as FunctionDefinitions; trailing `T_NOTYPE` → varargs; `LF_UNION` writes `UnionDataType`; `T_BOOL08` maps to Ghidra `bool` (wider PDB bools stay integers). Procedure definitions at `/pdb/procedures/PDB_xxxx` are refreshed in place (`REPLACE`, not `KEEP`). `overwrite_ghidra_function()` always writes `setVarArgs`. Plain leaf Namespaces are converted to `GhidraClass`. Unsupported PDB leaves are censused (not implemented speculatively). Pin is the current matching lineage plus that importer. Master's indexer currently SIGSEGVs after `bounder.cpp`, so the pin is not default master.
2. **SurRender IAT ABI** — `wiz8 analyze surrender-iat` projects CSV calling conventions and resolved demangled types onto thunk/external imports, types the IAT cell itself (`Pointer(FunctionDefinition)` for callables; `T*`/`T**`/vftable pointer for data rows), and saves the Program after apply. Ordinary `CALL [IAT]` callers never qualify as the import. Full resolved ABI → `IMPORTED`; convention-only → `ANALYSIS`. Nested `ns::X` datatypes resolve at `/ns/X` before class binding. Checkpoint notices `apply_errors`.
3. **Targeted Parameter ID** — `wiz8 analyze parameter-id` on DEFAULT/ANALYSIS unrecovered functions only; never IMPORTED/USER_DEFINED. Apply saves the Program. Disposable apply; measure callers and require quality debt ≤ 0 before promote.
4. **Secondary / for-clause vtables** — `vftable-typing` consumes source-index `base_vtables`, unmarked construction-phase census families, and confirmed vbtables. Slot ABI for `Derived::{for Base}` comes from Base's virtual slot. Construction/base tables never retarget the complete-object `vfptr`.
5. **vbptr ComponentOffset** — reccmp no longer writes a generic `-4`. Enrichment sets ComponentOffset only on already-present `VBasePtr`/`o_*` PointerTypedefs at a vbtable-proven virtual-base offset, or on proven secondary-subobject `this` receivers. Vbtables stay integer displacements.
6. **Recovery decompiler profile** — Java `RecoveryEngine` uses compiled-in defaults plus explicit recovery knobs (does not `grabFromProgram`). Prettier transformations stay off the recovery profile.
7. **HighFunction census** — `wiz8 analyze high-function-debt` (CAST/CALLIND, untyped `this`/params/return, untyped CALLIND, *suspicious* PTRADD/PTRSUB, unaff locals, ranked by debt × callers); typed PTRSUB to a known component and matching PTRADD are healthy. Not part of the decompiler-quality debt gate.

Do **not** bulk-import PDB locals into retail Ghidra, guess enums/bools from value patterns, or add more manual callback families.

Also deferred:

- **RTTI on `Wiz8.exe`**: retail is `/GR-`. Optional probes on RTTI-bearing modules only.
- **FID / source-backed library signatures**: next broad lane after C++ subobject work.

## Enrichment discipline

1. Repair obvious calling conventions and known signatures from compiler-backed facts.
2. Bind classes so auto `this` and the exporter share one Structure.
3. Only then run Decompiler Parameter ID on disposable candidates.
4. Project globals/callbacks/vftables through the same identity map.
5. Promote the exact tested candidate; do not treat a second live apply as equivalent.
