# Analysis enrichment

Decompiler quality for Wiz8 is primarily an analysis-database problem. Project
established source/retail facts with `uv run wiz8 ghidra sync` before asking
Ghidra to decompile. Pretty-printer and option tweaks are secondary.

## Evidence boundary

Known source/retail facts may enter the reviewed program. Speculative Param-ID,
RTTI, or heuristic guesses must not land in reviewed GZF state. Soft inferences
belong on disposable copies until they become independently established.

**A category path is organization, not provenance.** Do not maintain two mutable
runtime type graphs (for example root PDB Structures and a parallel
`/wiz8/classes` copy set). One authoritative live Structure per class, bound to
its `GhidraClass` namespace the way Ghidra and reccmp already expect.

`doctor` reports reviewed-seed origin and source-projection freshness separately.
A current seed does not imply current source projection.

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
inside apply transactions owned by `ghidra sync`.

Custom storage is an exceptional ABI operation, not the normal way to pick a
class Structure. Ordinary class-this typing **skips** functions that already
have custom variable storage (`skip-custom-storage`) and never calls
`setCustomVariableStorage(False)` to silence ABI. Explicit convention
hard-disagreements are skipped rather than overridden to `__thiscall`.

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
| Foundation | Source class ↔ `GhidraClass` ↔ Structure binding; auto `this` without custom storage | `class_binding` plus sync-owned class-this / class-structure projection |
| 10 | Objective decompiler-quality benchmark (oracle + pain) | `wiz8 analyze decompiler-quality` |
| 1 | Calling-convention / prototype repair before Param ID | source-backed conventions applied by `wiz8 ghidra sync` |
| 2 | One ProgramDB apply path | `wiz8 ghidra sync`; no trial/promote choreography |
| 5–8 | Globals, callbacks, CF, attributes | applied by the same sync; see notes below |
| 9 | Dual decompiler profiles | Python `semantic.py` / `inspect.py` profiles; Java `RecoveryEngine` uses the recovery profile (no `grabFromProgram`) |

Score a sync with `uv run wiz8 analyze decompiler-quality` before promoting the
live project into a reviewed GZF.

## Projection notes

Parser template/qualifier bugs on the Wiz8 side are closed (digit-only array
extents, order-independent qualifier stripping). **reccmp importer gaps for
`LF_PROCEDURE`, unions, trailing `T_NOTYPE` variadics, and `T_BOOL08` are fixed
upstream** (pin `76ba6f9a` on the current matching lineage). Curated
`callback_typing` / `function_attributes` still exist as audit/fallback for
already-named field sites; compiler-backed `Pointer(FunctionDefinition)` fields
are left alone (`compiler-backed`). Do not expand callback families. Resolve
Wizardry callback field sites through class identity → `GhidraClass` →
`find_class_structure()`, never `/wiz8/classes`. Keep `/_GUI_BUTTON` and
`/_MOUSE_REGION` as absolute paths. `_field_already_typed` compares
FunctionDefinition ABI contracts (parameter names are not ABI).

1. **Class binding first** — ordinary, derived, and secondary-base cases without custom storage. *(done)*
2. **Compiler projection** — extend reccmp importer gaps (callbacks/unions/variadics) upstream; retire overlapping handwritten declaration parsers only after that lands.
3. **Type graph projection** — identity map + field reconciliation onto bound Structures. Nested Pointer/Array/Structure/Union/FunctionDef refs remap through `class_binding`; equal-richness disagreements are `conflict` (never richer-wins). Opaque shells + rich evidence → `reconcile-fields`. *(done)*
4. **Legacy `/wiz8/classes` leftovers** — historical split identities remain in reviewed seed as agreeing duplicates (replaceable), field/size mismatches (conflict), and leftover copies with no non-legacy `/ClassName` Structure yet. Do not invent a second apply command for that cleanup.

`stLight::vInstance` (`0x0049e3a0`) pain +1 was a false `updateCategoryPath=True`
move plus a later leftover `replaceDataType`/`remove()`, which untyped `this` to
`undefined4` so the decompiler emitted `(void *)0x0`. Source is
`return new stLight(0);`. With `updateCategoryPath=False` and a `/stLight` path
lookup even when the parent is still a plain Namespace, the leftover copy
disappears and `this` becomes `/stLight *`. `ensure_ghidra_class` is a separate
step so auto `this` can bind. Do not add +1 tolerance.

5. **Vtable slots as ABI declarations** — census extents preserved (unresolved slots marked, never silently truncated). Construction and agreement share `desired_slot_contract` (source declaration, else source-backed live function, else callee). Unresolved source types skip the table rather than silently demoting to analysis. Parameter names are not ABI. Namespace-safe `/wiz8/vftables/…` paths landed; source-index `base_vtables` (for-clause / secondary base) are typed. Secondary slot `this` is the Base subobject (or a proven Derived ComponentOffset), not the Derived implementation body. Derived-specific subobject views live under `/wiz8/subobjects/Derived/Base_at_0x20` and replace the Base component inside Derived; canonical `/Base` stays untouched. Unmarked construction-phase tables that share a census construction family with a marked table are typed as `Class_vftable_ctor` and do not retarget the complete-object `vfptr`. Secondary `vfptr`/`vbptr` fields retarget only at a unique incoming-ECX offset. Confirmed vbtables installed by the same lifecycle function as a marked vftable are typed under `/wiz8/vbtables` as integer displacements; existing `VBasePtr`/`o_*` PointerTypedefs at `vbptr + displacement` may receive a ComponentOffset, but those typedefs are never invented.
6. **Validation** — `decompiler-quality` and `high-function-debt` measure current ProgramDB. They do not apply facts.

## Follow-through (landed with this architecture)

1. **reccmp `LF_PROCEDURE` / `LF_MFUNCTION`** as FunctionDefinitions; trailing `T_NOTYPE` → varargs; `LF_UNION` writes `UnionDataType`; `T_BOOL08` maps to Ghidra `bool` (wider PDB bools stay integers). Procedure definitions at `/pdb/procedures/PDB_xxxx` are refreshed in place (`REPLACE`, not `KEEP`). `overwrite_ghidra_function()` always writes `setVarArgs`. Plain leaf Namespaces are converted to `GhidraClass`. Unsupported PDB leaves are censused (not implemented speculatively). Pin is the current matching lineage plus that importer. Master's indexer currently SIGSEGVs after `bounder.cpp`, so the pin is not default master.
2. **SurRender IAT ABI** — sync projects CSV calling conventions and resolved demangled types onto thunk/external imports, and types the IAT cell itself (`Pointer(FunctionDefinition)` for callables; `T*`/`T**`/vftable pointer for data rows). Ordinary `CALL [IAT]` callers never qualify as the import. Full resolved ABI → `IMPORTED`; convention-only → `ANALYSIS`. Nested `ns::X` datatypes resolve at `/ns/X` before class binding.
3. **Parameter ID** is native Ghidra investigation on disposable copies. It is not a project planner or sync apply path. Never write Param-ID guesses into reviewed IMPORTED/USER_DEFINED signatures.
4. **Secondary / for-clause vtables** — `vftable_typing` consumes source-index `base_vtables`, unmarked construction-phase census families, and confirmed vbtables. Slot ABI for `Derived::{for Base}` comes from Base's virtual slot. Construction/base tables never retarget the complete-object `vfptr`.
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
3. Only then consider native Ghidra Parameter ID on disposable candidates.
4. Project globals/callbacks/vftables through the same identity map.
5. Apply through `wiz8 ghidra sync`; do not treat a second live apply of a subset as equivalent.
