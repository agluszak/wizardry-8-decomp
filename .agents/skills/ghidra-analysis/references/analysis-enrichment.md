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

## Ordered work

| Step | Goal | Status |
| --- | --- | --- |
| Foundation | Source class ↔ `GhidraClass` ↔ Structure binding; auto `this` without custom storage | `class_binding` + rewritten `class-this-typing` / `class-structures` |
| 10 | Objective decompiler-quality benchmark (oracle + pain) | `wiz8 analyze decompiler-quality` |
| 1 | Calling-convention / prototype repair before Param ID | `prototype-repair` (source-backed); heuristics report-only except census vtable thiscall |
| 2 | Owned enrichment candidate (unique run dir, hard-fail deltas) | `enrichment-checkpoint` (disposable by default; `--live` opt-in) |
| 5–8 | Globals, callbacks, CF, attributes | existing passes; consolidate onto shared resolver next |
| 9 | Dual decompiler profiles | Python `semantic.py` profiles; Java recovery still uses program options |

## Consolidation direction (before claiming the enrichment loop is complete)

1. **Class binding first** — ordinary, derived, and secondary-base cases without custom storage.
2. **Compiler projection** — extend reccmp importer gaps (callbacks/unions/variadics); retire overlapping handwritten declaration parsers (`srVector3T<float>` array misparse, qualifier order).
3. **Type graph projection** — identity map + field reconciliation, not component-count contests or competing universes.
4. **Vtable slots as ABI declarations** — census extents, full slot contracts, no silent truncation or stale FunctionDefinition agreement.
5. **One runner owns the experiment** — unique run directory, shared open `Program`, promote the tested candidate (not an unverified `--live` rerun).
6. **Validation** — safe application / preserved recovery / useful improvement as separate outcomes.

## Deferred deliberately

- **RTTI on `Wiz8.exe`**: retail is `/GR-`. Optional probes on RTTI-bearing modules only.
- **FID as DB enrichment**: after prototypes and class bindings land.

## Enrichment discipline

1. Repair obvious calling conventions and known signatures from compiler-backed facts.
2. Bind classes so auto `this` and the exporter share one Structure.
3. Only then run Decompiler Parameter ID on disposable candidates.
4. Project globals/callbacks/vftables through the same identity map.
5. Promote the exact tested candidate; do not treat a second live apply as equivalent.
