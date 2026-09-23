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
  Project established source facts with `uv run wiz8 ghidra sync`; inspect with `ghidra decompile` /
  `ghidra asm` / `ghidra sym`. Those reads do not compile, index, or synchronize.
- Search source and accepted oracles before declaring or implementing. One entity has one canonical
  owner and one evidence-backed type. Cross-TU functions/globals are declared in the owning header;
  callers include it. No local `.cpp` externs except actual C/OS/vendor interfaces without an existing
  project/dependency header.
- SurRender `SR_DLL_IMPORT` is consumer codegen, not ownership metadata. Provider exports do not prove
  a consumer import; use the retail consumer import tables and caller emission before adding/removing
  class- or member-level visibility. Follow `docs/libraries/surrender-import-visibility.md`.
- Type disagreement is a source-model defect, not a cast-site problem. Do not conceal it with casts,
  integer/pointer substitution, duplicate declarations, wrappers or aliases.
- Fix defects at their owner rather than suppressing symptoms. A recomp-only/runtime-only failure means
  some source, ABI, ownership, resource, analysis or control-flow model is wrong until evidence proves
  otherwise; do not add guards, ignores, shims or alternate paths just to make the symptom disappear.
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
- [jujutsu-workflow](.agents/skills/jujutsu-workflow/SKILL.md): start/resume Jujutsu changes, work with
  existing PR branches, rebase/squash safely, resolve conflicts and publish.

Recovery tooling is agent-only. Prefer existing/native primitives; do not add generic query protocols,
wrapper layers, parallel inventories/report frameworks or human-vs-machine output modes. Filter before
printing and put large disposable output under `build/`. Detailed operational recipes belong in skills.

## Source fidelity

- Faithfulness is mandatory; exact byte identity is incremental. Recover plausible authored circa-2000
  C++ and VC6 ABI, not compiler lowering. Never invent, omit, stub or approximate retail behavior.
- Never promote compiler output into an authored source construct. A concrete template emission proves
  only that the primary template was instantiated for those arguments; it never proves an explicit
  specialization or explicit instantiation. Likewise an inlined copy does not prove manual inlining,
  a deleting destructor does not prove a handwritten wrapper, and folded functions do not prove aliases.
  Keep template behavior in the primary template unless an accepted original-source oracle directly
  establishes otherwise. Recovered Wizardry/SurRender source gates new explicit specializations and
  instantiations without a comment waiver; an oracle-backed exception must change that reviewed gate.
- Compiler-owned storage reuse is not source evidence. Never alias a parameter/local or add overlapping
  source variables merely to reproduce stack-slot, register, spill or temporary reuse. Introduce the
  logical source variables even when that lowers comparison score.
- A machine-width access does not establish a source field of that width. Treat widened loads/stores,
  dword/block moves, `memcpy`, and same-offset alternate interpretations as compiler/aggregate-copy
  evidence first. Before adding an overlay or decomposing copied storage into sibling fields, trace the
  complete source/destination extent and test an existing embedded record or ordinary assignment.
  "Same offset", "same size", a wide move, or decompiler type disagreement is not positive union evidence.
- Trace pointer identity through callers before assigning a callee offset to a class. If satisfying a
  recovered signature requires reinterpret-casting one modeled W8/sr/st record pointer to another, the
  signature/owner model is wrong or unresolved; do not bless the cast. This includes pointer-to-pointer
  casts such as `T** -> U**`. Linker-folded sibling functions likewise do not make their source parameter
  types interchangeable.
- Search for the authored abstraction before spelling out a lowered sequence. Existing container/math/
  traversal helpers should be used when their semantics fit; repeated equivalent sequences across
  independently owned TUs trigger an inline/helper investigation rather than copy-pasted lowering.
- Establish behavior, then name it. `FunctionXXXXXX`, `FUN_...` and `unknown_...` are placeholders,
  not identities. Rename the definition, declaration, callers and ownership/provenance references in
  the same coherent change. When original spelling is unknown, use a behavior-descriptive name.
- Preserve ordinary counted `for` loops instead of reproducing guarded `do`/`while` lowering. Do not
  add redundant counters, artificial scopes, duplicate cleanup, return temporaries or rearranged
  expressions merely to change registers, CFG or comparison score.
- Preserve retail bugs/UB when evidence establishes them. Do not initialize, clamp, guard or otherwise
  normalize recovered code merely to make the recomp safer or deterministic. An explicitly requested
  compatibility deviation must be isolated and documented, never disguised as the recovered body.
- Treat suspicious recovered code as an investigation, not a mandate to produce a fix. If retail does
  the same thing, record the evidence in the issue/review and close the investigation; do not manufacture
  a comment-only C++ change unless a short source note prevents a likely future semantic mis-recovery.
- State only what the evidence establishes. A missing writer/check/free/import proves that operation is
  absent in the audited scope; it does not by itself prove intent, an authored ownership contract,
  global reachability, "by design", or that the behavior is a bug. "Not imported by Wiz8.exe" means no
  static import in that consumer, not that an export is unreachable.
- A vtable, lifecycle body, deleting destructor, address or template emission alone does not prove an
  authored class. Compare canonical bases/templates first. Compiler-generated deleting destructors,
  vtordisp/adjustor thunks and other compiler helpers are marker-only `SYNTHETIC`; template
  instantiations are `TEMPLATE`, never `FUNCTION`. `SYNTHETIC` and `LIBRARY` markers are
  marker-only; generic template implementations remain at their canonical template owner. A
  compiler-emission TU contains provenance only, not handwritten function/global definitions.
- Matching markers bind to the following source entity. Keep `// FUNCTION:` immediately adjacent to
  its declaration/definition; move pragmas/unrelated comments above the marker. Follow the matching
  skill for `TEMPLATE`, `SYNTHETIC`, `LIBRARY`, `VTABLE` and `GLOBAL` ownership.
- Preserve TU ownership/order in `src/wiz8/sources.cmake`. Recover placement before optimizer control:
  ordinary functions stay unannotated; header visibility/inlining requires cross-TU/call-site evidence.
- Legitimate `reinterpret_cast` sites express storage the type system cannot: external ABI, raw
  serialized/pixel memory, tagged storage, deliberate address/bit reinterpretation or an explicitly
  unresolved site. New casts require an attached `reinterpret-ok: <reason>` comment; a marker never justifies
  hiding known type disagreement.
- Source unions require positive evidence for overlapping authored storage: a real discriminant,
  mutually exclusive lifecycle/state, an accepted source oracle, or equivalent source-level evidence.
  Coincident offsets, equal sizes, widened copies and decompiler type disagreement do not qualify.
- Do not convert a repository-typed object to `char*`/`unsigned char*` and add a literal byte
  offset. The whole-tree source-model gate hard-rejects this for `this` and typed W8/sr/st pointers
  or references; there is no waiver. `raw-offset-ok: <reason>` remains only for genuinely
  unresolved/external raw storage that is not a modeled repository object.
- Do not invoke recovered code by reinterpret-casting storage/an address to an inline function-pointer
  type. Give the callable its evidence-backed declaration and calling convention; the whole-tree
  source-model gate has no waiver for this recovery shortcut. Ordinary declared callbacks remain valid.
- New C-style casts in recovered C++ are gated. Prefer the evidence-backed typed model or the specific
  C++ cast that states the proven conversion; a genuinely unavoidable historical C/ABI spelling needs
  attached `c-style-cast-ok: <reason>` comment. Cast comments may immediately precede the statement or
  remain on its formatter-wrapped continuation; never disable formatting to keep them on one line.
- `clang-format off` is not a matching technique. A new suppression needs same-line
  `format-off-ok: <reason>` and must cover the smallest construct the formatter genuinely cannot
  preserve; never disable formatting for a whole recovered function just to keep decompiler shape.
- SGP's released source spells textual filenames and format strings as `UINT8*` (`LoadButtonImage`
  and related APIs); preserve those declarations as historical ABI/API spelling rather than
  pretending `STR8`/`char*` was original. Wizardry text declarations use `char*`/`wchar_t*`;
  the cast at an SGP call is the documented boundary.
- Never use `unsigned char`/`UINT8` for a character merely because it is one byte.

## Scope and completion

Fix forward. When a check fails or the work exposes a concrete repository defect, fix the defect instead
of investigating whether it predates the current change. Do not spend time on blame/provenance archaeology;
use history only when it provides evidence needed to choose the correct fix. This applies to concrete
problems encountered during the task, not as an excuse for an unrelated repository-wide audit.

Complete the requested coherent task. Prefer repairing or extending the existing owner/path over creating
a parallel mechanism. Repository-owned APIs and formats have no compatibility contract: update current
producers and consumers together rather than adding old/new modes, migrations or shims. Delete replaced
scaffolding and obsolete infrastructure. Do not turn focused recovery into repository-wide cleanup merely
because a pattern exists elsewhere. Expand only when a shared owner/ABI/layout requires it, the source
model would otherwise become inconsistent, a concrete failure encountered during the task needs repair,
or the task explicitly requests an audit. Keep exploratory scripts disposable.

Do not incidentally edit `src/sgp` while recovering Wizardry/SurRender code. An SGP source change needs
its own accepted-source/retail evidence and required modification notice; a generated-code mismatch by
itself is not evidence that the released SGP source changed.

Stop when the requested bodies, ABI bundle or behavior meet acceptance criteria. Exact/effective bodies
need no independent rediscovery. If no evidence-backed correction remains, retain faithful source and
report the unresolved mismatch/evidence gap rather than manufacturing certainty.

Never add handwritten fake implementations to make a runnable product link. Unrecovered retail calls
may use the build-generated runtime `STUB` traps only when their retail address identity is established;
an addressless unresolved first-party callable is a source-model error, not a runnable stub. `FUNCTION`
means a body was actually recovered. The runtime-bringup skill owns the detailed stub/debug workflow.

## Verification

Use the smallest existing check capable of detecting the relevant failure. Validate coherent changes,
not each textual edit, and reuse a successful result until a relevant input changes.

- normal recovered body: focused `uv run wiz8 compare ADDRESS...`;
- layout/vtable/lifecycle/ABI: affected comparison bundle plus relevant compile/layout and `vtable` checks;
- runtime behavior: relevant existing scenario and the requested observable result;
- Python/tooling: focused tests/lint/type checks appropriate to the changed owner;
- prose/skill-only changes: inspect the diff;
- Ghidra inspection: `uv run wiz8 ghidra decompile ADDRESS...` / `asm` / `sym`; project facts with
  `uv run wiz8 ghidra sync` when ProgramDB is missing an established declaration.

For a substantial recovery batch, focused compares run during development are not the final audit. After
the last source edit run `uv run wiz8 compare --changed` and account for every new or materially changed
`FUNCTION`: exact/effective, an explained compiler-lowering mismatch, or explicitly inconclusive with
retail CFG/call/branch review.

`uv run wiz8 check` is the fast repository lane; `uv run wiz8 lint` is the clang-cl/tidy lane. A
gating failure is work to fix, not a provenance question: do not first establish whether it is
baseline/pre-existing. If one environment alone reports a diagnostic, fix the path/mount/compile-database
disagreement rather than suppressing the finding.

Before publishing a pull request, run `uv run wiz8 pr-check`. It always runs `wiz8 check` and also
runs `wiz8 lint` when the PR changes C/C++ source or headers; a C/C++ PR is not validated without both
lanes. `pr-check` and `report merge-preservation` first require the given `--base` to be an ancestor
of the current head and fail with merge-base/ahead/behind counts when it is not; fetch and rebase onto
`main@origin` first rather than claiming success against a stale local base.

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

For a genuinely new recovery task, adopt the intended base and run `uv run wiz8 doctor` before relying
on retail/Ghidra analysis. Doctor validates checkout-local Ghidra ownership and reviewed-seed freshness
in addition to the machine/toolchain checks, and reports source-projection freshness separately. A
current reviewed seed does not imply current source declarations have been projected. `stale`,
`untracked`, or `unknown` Ghidra freshness blocks recovery; a matching retail binary hash alone does
not prove the live analysis is current. Rerun doctor after a rebase/merge that changes the reviewed
Ghidra manifest/checkpoint. Follow the ghidra-analysis skill for reconciliation; doctor never repairs
or overwrites live analysis.

Keep one mutable change per coherent task by default. Fetch/rebase from `main@origin` only when upstream
work is needed or immediately before authorized integration. After a rebase or merge, run
`uv run wiz8 report merge-preservation --base origin/main`; every removed or duplicated retail-address
identity needs an explicit `--allow` reason. Successful push completes publication;
do not perform routine post-push proofs. Jujutsu mechanics live in
[jujutsu-workflow](.agents/skills/jujutsu-workflow/SKILL.md).
