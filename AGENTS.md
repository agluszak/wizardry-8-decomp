# Wizardry 8 decompilation

This is a Jujutsu repository for evidence-driven matching decompilation.

## Evidence and ownership

- Retail instructions/call sites and accepted original-source oracles outrank inferred Ghidra types,
  generated output, comparison scores and workflow documentation. Correct errors at their owner; keep
  unresolved facts unknown.
- Ghidra owns live retail analysis: functions, signatures/storage, symbols, references, types/fields,
  vtables, comments, decompiler state and original-binary TU evidence. Git/C++ owns recovered source,
  declarations, source placement, matching annotations and build configuration. Reviewed provenance
  explains accepted identities without duplicating either model. Generated `build/` projections are disposable.
- Search source and accepted oracles before declaring or implementing. One entity has one canonical
  owner and one evidence-backed type. Cross-TU functions/globals are declared in the owning header;
  callers include it. No local `.cpp` externs except actual C/OS/vendor interfaces without an existing
  project/dependency header.
- Type disagreement is a source-model defect, not a cast-site problem. Do not conceal it with casts,
  integer/pointer substitution, duplicate declarations, wrappers or aliases.
- Do not invent aggregate/class boundaries from adjacency, shared initialization, repeated offsets or
  convenient access patterns; do not split a proven object for local convenience. Repository-owned
  Wizardry and SurRender code is unconditional C++; `extern "C"` requires proven C linkage.
- Never commit binaries, extracted trees, live Ghidra projects or build products. Only reviewed GZF
  checkpoints listed in `vendor/ghidra/exports/manifest.json` may be tracked. Preserve source licences.

## Task skills

`.agents/skills` is the canonical shared tree. Load the task skill and only the references it routes to:

- [matching-decomp](.agents/skills/matching-decomp/SKILL.md): recover function bodies, place source,
  interpret focused reccmp mismatches and use source oracles.
- [ghidra-analysis](.agents/skills/ghidra-analysis/SKILL.md): inspect/edit canonical Ghidra analysis;
  checkpoint reconciliation and bulk source import are opt-in references there.
- [type-modeling](.agents/skills/type-modeling/SKILL.md): prototypes, globals, fields, enums/bools,
  layouts, packing, class boundaries, inheritance, vtables and lifecycle ABI.
- [runtime-bringup](.agents/skills/runtime-bringup/SKILL.md): runtime behavior, UI/input, persistence,
  loading, startup, debugger use and semantic scenarios.
- [tooling-maintenance](.agents/skills/tooling-maintenance/SKILL.md): Python/CMake/reccmp/clang,
  source indexing, CLI orchestration, validation and runtime infrastructure.

Recovery tooling is agent-only. Prefer existing/native primitives; do not add generic query protocols,
wrapper layers, parallel inventories/report frameworks or human-vs-machine output modes. Filter before
printing and put large disposable output under `build/`. Detailed operational recipes belong in skills.

## Source fidelity

- Faithfulness is mandatory; exact byte identity is incremental. Recover plausible authored circa-2000
  C++ and VC6 ABI, not compiler lowering. Never invent, omit, stub or approximate retail behavior.
- Establish behavior, then name it. `FunctionXXXXXX`, `FUN_...` and `unknown_...` are placeholders,
  not identities. Rename the definition, declaration, callers and ownership/provenance references in
  the same coherent change. When original spelling is unknown, use a behavior-descriptive name.
- Preserve ordinary counted `for` loops instead of reproducing guarded `do`/`while` lowering. Do not
  add redundant counters, artificial scopes, duplicate cleanup, return temporaries or rearranged
  expressions merely to change registers, CFG or comparison score.
- A vtable, lifecycle body, deleting destructor, address or template emission alone does not prove an
  authored class. Compare canonical bases/templates first. Compiler-generated deleting destructors are
  marker-only `SYNTHETIC`, never handwritten bodies or hidden-flags helpers.
- Matching markers bind to the following source entity. Keep `// FUNCTION:` immediately adjacent to
  its declaration/definition; move pragmas/unrelated comments above the marker. Follow the matching
  skill for `TEMPLATE`, `SYNTHETIC`, `LIBRARY`, `VTABLE` and `GLOBAL` ownership.
- Preserve TU ownership/order in `src/wiz8/sources.cmake`. Recover placement before optimizer control:
  ordinary functions stay unannotated; header visibility/inlining requires cross-TU/call-site evidence.
- Legitimate `reinterpret_cast` sites express storage the type system cannot: external ABI, raw
  serialized/pixel memory, tagged storage, deliberate address/bit reinterpretation or an explicitly
  unresolved site. New casts require same-line `reinterpret-ok: <reason>`; a marker never justifies
  hiding known type disagreement.

## Scope and completion

Complete the requested coherent task. Do not turn focused recovery into repository-wide cleanup merely
because a pattern exists elsewhere. Expand only when a shared owner/ABI/layout requires it, the source
model would otherwise become inconsistent, or the task explicitly requests an audit. Keep exploratory
scripts disposable.

Stop when the requested bodies, ABI bundle or behavior meet acceptance criteria. Exact/effective bodies
need no independent rediscovery. If no evidence-backed correction remains, retain faithful source and
report the unresolved mismatch/evidence gap rather than manufacturing certainty.

Never add handwritten fake implementations to make a runnable product link. Unrecovered retail calls
may use the build-generated runtime `STUB` traps; `FUNCTION` means a body was actually recovered. The
runtime-bringup skill owns the detailed stub/debug workflow.

## Verification

Use the smallest existing check capable of detecting the relevant failure. Validate coherent changes,
not each textual edit, and reuse a successful result until a relevant input changes.

- normal recovered body: focused `uv run wiz8 compare ADDRESS...`;
- layout/vtable/lifecycle/ABI: affected comparison bundle plus relevant compile/layout and `vtable` checks;
- runtime behavior: relevant existing scenario and the requested observable result;
- Python/tooling: focused tests/lint/type checks appropriate to the changed owner;
- prose/skill-only changes: inspect the diff.

`uv run wiz8 check` is the fast repository lane; `uv run wiz8 lint` is the clang-cl/tidy lane. Do not
dismiss a gating failure as baseline/pre-existing. If one environment alone reports a diagnostic, fix
the path/mount/compile-database disagreement rather than suppressing the finding.

Format manually owned C/C++ files you changed with `uv run clang-format --style=file -i <paths>` and
check them with `--dry-run --Werror --fail-on-incomplete-format`. Do not reformat imported/vendor source
such as `src/sgp`. Formatting alone does not require another build/comparison.

Do not add tests by default. Add them only for a concrete correctness bug or stable behavior existing
checks miss, with independently justified expectations. Do not test source spelling, documentation,
inventory counts, generated snapshots, deleted files or implementation-private helper order. Remove
obsolete tests/helpers with the machinery they protected.

## Workspace and publication

Use the provided Jujutsu checkout and preserve unrelated work. Never create another worktree/workspace,
clone, sibling or baseline checkout unless explicitly requested; each existing checkout needs its own
`WIZ8_WORK_DIR` and live Ghidra project. Only one agent may switch/rebase/publish a shared checkout.

Keep one mutable change per coherent task by default. Fetch/rebase from `main@origin` only when upstream
work is needed or immediately before authorized integration. Successful push completes publication;
do not perform routine post-push proofs. Command details live in
[docs/contributor-workflow.md](docs/contributor-workflow.md).
