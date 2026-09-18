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

Custom storage is an exceptional ABI operation, not the normal way to pick a
class Structure. Synthetic `wiz8::classes::…` namespaces must not be invented
to justify a directory layout.

Acceptance cases (ordinary method, derived, secondary-base) should bind without
custom storage. Fixture coverage for those shapes lives in
`tests/unit/test_class_binding.py` (plus lifecycle integration skips where a
full ProgramBuilder is still pending).

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

Disposable trials write `work_dir/enrichment-checkpoint/run-<id>/` (project +
`report.json`) and mirror the report under `build/enrichment-checkpoint/`.

Report `outcomes`:

- `safe_application` — no unexpected apply errors, import ok, inputs matched
- `preserved_recovery` — quality/pain deltas report no new decompiler failures
- `useful_improvement` — `null` if unmeasured; else debt improved or actionable applies landed without regression

CLI `ok` remains `safe_application and preserved_recovery` (nonzero exit when false).

When a disposable trial is accepted, promote **that** candidate — do not rerun
with `--live`:

```sh
uv run wiz8 analyze enrichment-promote --from-latest
# or:
uv run wiz8 analyze enrichment-promote "$WIZ8_WORK_DIR/enrichment-checkpoint/run-<id>"
```

Promote packs the candidate program to a temporary GZF under
`build/enrichment-promote/`, moves the live project aside when present, restores
into a fresh checkout `ghidra-project/`, and records reviewed-seed provenance.
It refuses stale/untracked/unknown live freshness without `--force`, and refuses
candidates whose recorded seed sha256/program disagree with the current
manifest. It does **not** refresh vendor GZF seeds.

## Consolidation direction

Items below are the advanced follow-through after the `#212`/`#218` binding
pivot. Parser template/qualifier bugs on the Wiz8 side are closed
(digit-only array extents, order-independent qualifier stripping). **reccmp
importer gaps remain upstream** and are not fixed by bumping the pin here:
`LF_PROCEDURE` → void, missing-union write, and `T_NOTYPE` variadic rejection.
Those gaps are why curated `callback_typing` / `function_attributes` still
exist; do not expand them as a second declaration source.

1. **Class binding first** — ordinary, derived, and secondary-base cases without custom storage. *(done)*
2. **Compiler projection** — extend reccmp importer gaps (callbacks/unions/variadics) upstream; retire overlapping handwritten declaration parsers only after that lands.
3. **Type graph projection** — identity map + field reconciliation, not component-count contests or competing universes. Consumers (`callback-typing`, `global-typing`, `class-structures`, vfptr retarget) share `class_binding`. Full field REPLACE remapper remains deferred.
4. **Vtable slots as ABI declarations** — census extents preserved (unresolved slots marked, never silently truncated); agree requires live callee FunctionDefinition contracts; apply refuses unresolved census rows.
5. **One runner owns the experiment** — unique run directory, shared open `Program`, promote the tested candidate (not an unverified `--live` rerun).
6. **Validation** — safe application / preserved recovery / useful improvement as separate outcomes.

## Deferred deliberately

- **Upstream reccmp** `LF_PROCEDURE` / union / variadic import (and any pin bump).
- **Full two-phase type-graph remapper** (arrays/unions/FunctionDefs / order-independence acceptance).
- **RTTI on `Wiz8.exe`**: retail is `/GR-`. Optional probes on RTTI-bearing modules only.
- **FID as DB enrichment**: after prototypes and class bindings land.
- Deleting leftover `/wiz8/classes` types from live seeds (migration reporting only).

## Enrichment discipline

1. Repair obvious calling conventions and known signatures from compiler-backed facts.
2. Bind classes so auto `this` and the exporter share one Structure.
3. Only then run Decompiler Parameter ID on disposable candidates.
4. Project globals/callbacks/vftables through the same identity map.
5. Promote the exact tested candidate; do not treat a second live apply as equivalent.
