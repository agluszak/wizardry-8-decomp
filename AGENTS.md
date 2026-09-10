# Wizardry 8 decompilation

This is a Jujutsu repository for evidence-driven matching decompilation.

## Evidence and ownership

- Retail instructions/call sites and accepted original-source oracles outrank inferred Ghidra types,
  generated output, comparison scores, and workflow documentation. Correct errors at their owner;
  keep unresolved facts unknown.
- Ghidra owns live analysis: signatures/parameter storage, symbols, references, types/fields, vtables,
  comments, decompiler state, and the original binary translation-unit layout. Git/C++ owns recovered
  source, declarations, current source placement, matching annotations, compiler/build configuration,
  and provenance claims; current placement is checked against the Ghidra TU layout, not derived from
  it. Provenance explains accepted identities and authority limits; do not duplicate either model into
  another database. Generated `build/` projections are disposable.
- Search source and accepted oracles before declaring or implementing. One entity has one canonical
  owner and one evidence-backed type. Cross-TU functions/globals are declared in the owning header;
  callers include it. No local `.cpp` externs, except actual C/OS/vendor interfaces without an
  existing project or dependency header.
- Type disagreement is a source-model defect, not a cast-site problem. Trace producers, consumers,
  callers, callees, loads, and stores; correct the canonical declaration and Ghidra model. Do not use
  casts, integer/pointer substitution, duplicate declarations, or wrapper types to conceal disagreement.
- Do not invent wrappers, aliases, opaque replacement types, raw vtable calls, or parallel inventories.
  Repository-owned Wizardry and SurRender code is unconditional C++; `extern "C"` requires proven
  C linkage. Do not add C fallback APIs.
- Never commit binaries, extracted trees, live Ghidra projects, or build products. Only reviewed GZF
  checkpoints listed in `vendor/ghidra/exports/manifest.json` may be tracked. Preserve source licences
  and required notices.

## Task skills

`.agents/skills` is the canonical shared tree; integrations consume it, never separate copies.
Load the applicable skill and only the references needed for the question:

- [matching-decomp](.agents/skills/matching-decomp/SKILL.md): source recovery, native Ghidra,
  comparison, source oracles, and type investigation.
- [class-triage](.agents/skills/class-triage/SKILL.md): new class boundaries, inheritance/subobjects,
  or deciding whether lifecycle/vtable families are distinct authored classes; not ordinary methods.
- [runtime-bringup](.agents/skills/runtime-bringup/SKILL.md): runtime behavior, UI/input, persistence,
  loading, and startup.

Recovery tooling is agent-only. Use existing primitives and native APIs; do not add query protocols,
wrapper layers, report frameworks, inventories, or human/JSON modes. Filter before printing; put large
listings in named `build/` files. Operational recipes belong in skills, not README duplicates.

## Source fidelity

- Faithfulness is mandatory; exact byte identity is incremental. Recover plausible authored
  circa-2000 C++ and VC6 ABI, not compiler lowering. Never invent, omit, stub, or approximate retail code.
- Preserve counted `for` loops instead of reproducing guarded `do`/`while` lowering. Do not add
  redundant counters, artificial scopes, duplicate cleanup, return temporaries, or rearranged
  expressions merely to change registers, CFG, or score.
- A vtable, lifecycle body, address, or template emission alone does not prove an authored class.
  Compare canonical bases/templates first; generic definitions belong in canonical headers.
- Scalar/vector deleting destructors are compiler-generated MSVC glue: marker-only `SYNTHETIC`
  identities, never handwritten bodies, hidden flags parameters, or destruct-and-maybe-free helpers.
- Preserve TU ownership/order in `src/wiz8/sources.cmake`; keep address-qualified template emissions
  separate until ownership is proved. Recover placement before optimizer control: ordinary functions
  stay unannotated; header bodies need cross-TU visibility evidence; inline controls need call-site
  evidence plus improvement of the complete ABI bundle.
- Legitimate `reinterpret_cast` sites express storage the type system cannot: external ABI, raw
  serialized/pixel memory, tagged storage, deliberate address/bit reinterpretation, or an explicitly
  unresolved site. New casts require a same-line `reinterpret-ok: <reason>` comment (`just check`
  enforces this). A marker does not justify concealing known type disagreement.

## Scope and completion

Complete the requested coherent task. Do not turn focused recovery into a repository-wide cleanup
merely because the pattern exists elsewhere. Expand only when a shared owner/ABI/layout requires it,
the source model would otherwise become inconsistent, or the task explicitly requests an audit.
Fix blockers at their existing owner with the smallest reliable change; keep exploratory scripts disposable.

Stop when the requested functions, ABI bundle, or behavior meet acceptance criteria. Exact/effective
bodies need no independent rediscovery. If no evidence-backed correction remains, retain faithful
source and report the unresolved mismatch or missing evidence; do not relabel uncertainty as success.

## Verification

Use the smallest existing check capable of detecting the relevant failure. Validate coherent changes,
not each textual edit. Reuse a successful result until a relevant input changes.

- Normal recovered function: focused `just compare ADDRESS...` (builds itself).
- Layout/vtable/lifecycle/ABI: affected comparison bundle plus relevant declaration/ABI and
  `just wiz8 vtable CLASS` checks.
- Behavior/runtime: relevant runtime scenario and the requested observable behavior.
- Python/tooling: relevant existing tests (`uv run pytest -q PATH`) and lint/type checks; shared build
  or validation machinery needs checks appropriate to its reach.
- Prose/skill-only changes: inspect the diff.

`just wiz8 verify` is deliberately broad, not a completion/publication ritual. Do not repeat unrelated
baseline failures or rerun checks after descriptions, change IDs, bookmarks, or pushes alone.

Do not add tests by default. Add them only when requested or for a concrete observed correctness bug
existing checks miss, with independently justified expectations. Test behavior, not source spelling,
documentation, inventory counts, generated snapshots, deleted files, or internal implementation details.
Do not add Python tests inspecting recovered source. Remove obsolete tests/helpers with their machinery;
do not create frameworks merely to support tests.

## Workspace and publication

- Use Jujutsu here; preserve unrelated work. Never create a worktree, workspace, clone, sibling, or
  baseline checkout unless explicitly requested. Existing additional checkouts need unique absolute
  `WIZ8_WORK_DIR` values; never share, copy, or hardlink a live Ghidra project.
- Resume the task's mutable change; keep one per coherent task and unfinished work off `main`.
  Bookmarks are optional until publication. Only one agent may switch, rebase, or publish a shared checkout.
- Use `main@origin` as the upstream base; fetch/rebase when upstream work is needed or immediately
  before authorized integration. Use an assigned Bead; update records only for coordination or durable
  findings, never as a task prerequisite.
- Publish once within authorization. Move `main` only for completed authorized direct integration;
  never rewrite remote `main` or discard others' changes. Successful push completes publication;
  investigate actual rejections without routine post-push proofs. Commands:
  [contributor workflow](docs/contributor-workflow.md).
