# Wizardry 8 decompilation

This is a Jujutsu repository for evidence-driven matching decompilation.

## Evidence and ownership

- Retail instructions/call sites and accepted original-source oracles outrank inferred Ghidra types,
  generated output, comparison results and workflow documentation. Correct errors at their owner; keep
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
  interpret focused decompiled comparisons and use source oracles.
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

Recover plausible authored circa-2000 C++ first, semantic/ABI fidelity second, and code generation
third. Comparison validates a coherent source model; it is not an objective function. A clean diff
proves neither equivalence nor original spelling. Keep faithful source when only established lowering
or decompiler representation remains.

Before changing recovered C++, read [source fidelity](.agents/skills/matching-decomp/references/source-fidelity.md)
through the matching-decomp skill. It owns the detailed recovery constraints and historical API rules.
The essential invariants are:

- Never invent, stub, approximate, omit or normalize retail behavior. Preserve established bugs/UB.
- Recover canonical abstractions, types, ownership, layout and TU/header placement. Fix disagreement
  at its owner rather than introducing casts, duplicate declarations, wrappers or aliases.
- Compiler output is not authored source: storage reuse, ICF, widened copies, inlined copies, template
  emissions and deleting destructors do not establish aliases, unions, manual inlining, specializations
  or handwritten wrappers. Use ordinary logical variables and counted loops.
- Never shape source solely to influence registers, stack layout, scheduling, CFG or decompiler text.
  Compiler controls require independent authored-source evidence. Existing matching tricks are debt.
- Trace receiver/argument identity before assigning offsets or types. Unions require positive
  source-level overlap evidence. Do not byte-offset modeled repository objects or call recovered code
  through an address cast; these gates have no waiver.
- Respect matching-marker ownership, template and synthetic emission classification, source units,
  visibility, licences and the documented cast/format waivers. Do not reintroduce FOLDED markers or
  identity aliases. Do not incidentally edit released SGP source.
- State what the audited evidence establishes; absence of an operation does not establish intent,
  ownership, reachability or a bug. Stronger source/retail evidence wins even if the diff grows.

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
its own accepted-source/retail evidence and required modification notice; a generated-code difference by
itself is not evidence that the released SGP source changed.

Stop when the requested bodies, ABI bundle or behavior meet acceptance criteria. Bodies whose comparison
shows no differences need no independent rediscovery. If no evidence-backed correction remains, retain
faithful source and report the unresolved difference/evidence gap rather than manufacturing certainty.

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
`FUNCTION`: no differences, differences explained as decompiler representation or established lowering,
or an unstable diff with retail CFG/call/branch review.

`uv run wiz8 check` is the fast repository lane; `uv run wiz8 lint` is the clang-cl/tidy lane. A
gating failure is work to fix, not a provenance question: do not first establish whether it is
baseline/pre-existing. If one environment alone reports a diagnostic, fix the path/mount/compile-database
disagreement rather than suppressing the finding.

Before publishing a pull request, run `uv run wiz8 pr-check`. It always runs `wiz8 check` and also
runs `wiz8 lint` when the PR changes C/C++ source or headers; a C/C++ PR is not validated without both
lanes. `pr-check` and `report merge-preservation` first require the given `--base` to be an ancestor
of the current head and fail with merge-base/ahead/behind counts when it is not; fetch and rebase onto
`main@origin` first rather than claiming success against a stale local base.

Formatting and command recipes belong to the owning skills. Formatting alone does not invalidate a
successful build/comparison. Preserve evidence of successful checks and rerun only lanes whose
relevant inputs changed; the final changed-body audit and pre-publication gates remain required.

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
`uv run wiz8 report merge-preservation --base origin/main`. The checker recognizes evidence-backed
marker reclassification and globals absorbed into a known aggregate extent; any remaining loss,
duplicate or demotion is a source/model/tooling defect to resolve, not a waiver to add. Successful push
completes publication;
do not perform routine post-push proofs. Jujutsu mechanics live in
[jujutsu-workflow](.agents/skills/jujutsu-workflow/SKILL.md).
