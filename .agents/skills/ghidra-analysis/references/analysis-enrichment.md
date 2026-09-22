# Analysis enrichment

Decompiler quality for Wiz8 is primarily an analysis-database problem. Project
established source/retail facts with `uv run wiz8 ghidra sync` before asking
Ghidra to decompile. Pretty-printer and option tweaks are secondary.

## Evidence boundary

Known source/retail facts may enter the reviewed program. Speculative Param-ID,
RTTI, or heuristic guesses must not land in reviewed GZF state. Soft inferences
belong on disposable copies until they become independently established.

A category path is organization, not provenance. Keep one authoritative live
Structure per class, bound to the `GhidraClass` namespace Ghidra and reccmp use;
do not maintain a parallel mutable `/wiz8/classes` type graph.

`doctor` reports reviewed-seed origin and source-projection freshness separately.
A current seed does not imply current source projection.

## Class binding

Ghidra types automatic `this` through
`VariableUtilities.findOrCreateClassStruct(function)` from the function's
`GhidraClass` parent. reccmp places ordinary classes at a namespace-based
category (typically `/ClassName`) and creates the matching class namespace.

Enrichment must:

1. map the source class identity to that `GhidraClass`;
2. make `findExistingClassStruct` resolve the reviewed Structure;
3. keep dynamic storage so automatic `this` uses that Structure;
4. let `VtableResolver.classNamespace()` resolve the same class, using the
   leaf-name fallback only when the category mapping is ambiguous.

Collect/plan is read-only. `collect_*` helpers use `find_ghidra_class` and emit
`missing-class` / `create-class` actions; `ensure_ghidra_class` runs only
inside apply transactions owned by `ghidra sync`.

Custom variable storage is an ABI fact, not a normal class-typing mechanism.
Ordinary class-this typing skips functions that already use custom storage and
does not disable custom storage to force a convenient `this` type. Explicit
calling-convention disagreements are reported rather than overwritten.

The ordinary, derived, and secondary-base cases are integration-tested through
the lifecycle fixture and `tests/ghidra/test_class_binding_integration.py`.

## Projection rules

- Compiler-backed PDB procedure/member-function definitions, unions, bools and
  varargs come from the pinned reccmp importer. Do not duplicate that parsing
  in project-specific declaration code.
- `callback_typing` and `function_attributes` are narrow audit/fallback
  layers for already-named sites. Compiler-backed
  `Pointer(FunctionDefinition)` fields are left alone. Do not grow manual
  callback-family inventories.
- Type-graph projection reconciles fields onto the one bound Structure.
  Equal-authority disagreements remain conflicts; do not use "richer wins".
  Legacy `/wiz8/classes` copies are cleanup targets, not a second owner.
- Vtable census extents are preserved. Unresolved slots stay explicit rather
  than being truncated. Slot ABI comes from the source declaration when
  available, then source-backed live analysis, then the callee.
- Secondary-table `this` is the base subobject, or a proven ComponentOffset
  view of it. Construction/base tables never retarget the complete-object
  `vfptr`.
- Vbtables remain integer displacements. ComponentOffset is applied only to an
  already-present `VBasePtr` / `o_*` pointer or a proven secondary-subobject
  receiver; never invent one from a generic offset.
- SurRender IAT sync projects calling conventions and resolved demangled types
  onto thunk/external imports and types the IAT cell itself. A fully resolved
  callable contract is `IMPORTED`; convention-only evidence remains
  `ANALYSIS`. Ordinary `CALL [IAT]` callers are not the import.
- `uv run wiz8 analyze parameter-id` is collect-only. Never apply it over
  `IMPORTED` or `USER_DEFINED` signatures or silently promote it into sync.
- The Java recovery engine uses its recovery decompiler profile; ordinary
  `ghidra decompile` uses the analysis profile. Diagnostic profile differences
  are not evidence for source changes.
- `decompiler-quality` and `high-function-debt` measure the current ProgramDB.
  They do not apply facts or create reviewed evidence.

Do not bulk-import PDB locals into retail Ghidra, guess enums/bools from value
patterns, or add more manual callback families.

## Enrichment discipline

1. Repair calling conventions and known signatures from compiler-backed facts.
2. Bind classes so automatic `this` and the exporter share one Structure.
3. Project globals, callbacks, class fields, vftables and vbtables through the
   same identity map.
4. Use Param-ID only as a disposable investigation when established facts are
   insufficient.
5. Apply reviewed facts through `wiz8 ghidra sync`; do not treat a second live
   apply path for a subset as equivalent.
6. Measure the resulting ProgramDB with
   `uv run wiz8 analyze decompiler-quality` when quality changes are the task.
