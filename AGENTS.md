# Wizardry 8 decompilation

This is a Jujutsu repository for evidence-driven matching decompilation. Use Beads for durable task state and `just` for supported workflows.

## Authority

- Ghidra owns live analysis state: functions, names, signatures, labels, types, fields, enums, vtables, references, comments, and decompiler state.
- Retail instructions and call sites, plus pinned upstream source where available, outrank inferred Ghidra types, generated declarations, tool verdicts, and workflow descriptions. Treat this file and the skills as maintainable guidance, not evidence about the original program. Correct demonstrated errors at their owner.
- Git owns recovered C++, declarations, translation-unit order, compiler settings, matching markers, and provenance claims. Generated `build/` projections are disposable.
- Provenance records why an identity is accepted, its authority ceiling, aliases, confidence, and observations; it does not duplicate Ghidra's model.
- Never commit binaries, extracted trees, live Ghidra projects, or build products. Only reviewed GZF checkpoints in `vendor/ghidra/exports/manifest.json` may be tracked.
- Do not reverse engineer available source. Use pinned Windows/MSVC runtime, SGP, zlib, IJG, and Info-ZIP source/header oracles. SurRender remains recoverable.
- Search `third_party/sfi-sgp/sgp` before using any SGP interface and include its owning header. Put missing product headers under `include/wiz8/sgp-compat` by their original names; never invent replacement interfaces. Preserve SFI-SCLA and upstream notices.
- Search before declaring anything. Extend the canonical owner; do not add duplicate externs, guessed aliases, raw vtable calls, wrappers, or parallel inventories.
- Repository-owned Wizardry and SurRender code is unconditional C++. Retain `extern "C"` only for proven C linkage; never add C fallback APIs.

## Recovery

Use `matching-decomp` for the operational recovery loop and comparison reasoning. Use `class-triage`
before declaring an unnamed constructor, destructor, vtable, registry family, or class. Their targeted
references own technical examples; this file owns repository policy.

Recovery commands are agent-only structured interfaces: they emit one JSON result or failure, keep
large generated code in named `build/` artifacts, and do not provide human/JSON presentation modes.

- Assume ordinary circa-2000 C++, VC6 ABI, and familiar container/lifecycle semantics unless evidence requires otherwise.
- Never invent code absent from retail, and never omit, stub, or approximate code retail contains.
- Do not invent wrapper types/APIs or speculative type boundaries to improve codegen. One object has one evidence-backed canonical type.
- A distinct vtable, lifecycle body, address, or template emission does not alone prove authored source. Compare canonical bases/templates first. Record emitted instantiations with `TEMPLATE`; keep generic definitions in canonical headers.
- Preserve proven translation-unit ownership and order in `src/wiz8/sources.cmake`. Keep address-qualified template emissions separate until ownership is proved.

Faithfulness is absolute; byte identity is incremental. The pinned VC6 build tests source hypotheses. Build and compare a connected first-pass batch together; use `just compare --changed` for every marked function in changed C++ files, or explicit addresses/`--file` selections for a narrower batch. Compare immediately when an experiment changes previously matched layout, ABI, lifecycle, or behavior. Inspect reported divergences: a classifier failure or low percentage alone does not prove incorrect C++. Revert proven regressions and record the hypothesis, command, before/after result, first divergence, and conclusion in the Bead. Do not brute-force inlining, scheduling, or register allocation. If instructions correspond one-for-one and only scratch registers differ, the body is done.

Marker rules enforced by `just check`: `FUNCTION` sits immediately above its declaration; `TEMPLATE` is followed immediately by a comment naming the emitted symbol and owns no body; `LIBRARY` is address-only.

Recover source placement before optimizer control. Ordinary functions stay unannotated. Header/class bodies require cross-translation-unit visibility evidence. Inline-control annotations require call-site evidence and improvement of the complete ABI bundle, and should be revisited later.

## Scope

The deliverable is the requested recovery, behavior, or code change. Tooling, tests, documentation,
and process changes support that work; they do not replace it. During recovery, do not introduce new
commands, frameworks, schemas, inventories, generic abstractions, or policy gates unless an existing
defect blocks the requested work and has no small reliable workaround. Fix a blocker at its existing
owner. Record non-blocking tooling ideas briefly and continue. Keep exploratory scripts disposable
unless they solve a recurring problem that warrants maintenance.

## Verification

Validate a coherent change, not each textual edit. Finish the declaration, definition, and necessary
caller updates for one source hypothesis, then compare the affected functions. Use the smallest
existing check that can detect the relevant failure:

- Prose-only documentation or skill changes: review the diff.
- Python implementation: relevant existing tests and relevant lint/type checks.
- Recovered function body with unchanged ABI: focused `just compare ADDRESS...`; it builds by default.
- Class layout, inheritance, virtual, or lifecycle changes: focused affected ABI bundle, relevant
  `just vtable` checks, and declaration/ABI checks.
- Build system or shared validation machinery: broad checks appropriate to that machinery.
- Integration: `just verify` once, plus focused comparisons it does not provide. Do not separately
  repeat its constituent checks.

A completed check remains sufficient until a relevant source, dependency, configuration, toolchain,
or analysis input changes. Reuse a recovery command's result when it covers the retained source and
current inputs. Do not repeat a check merely because you are committing, handing off, or moving a
bookmark.

For a local recovery task, stop when the affected functions or ABI bundle meet the task's acceptance
criteria. Exact/effective does not require an independent reconstruction of the same evidence. For a
mismatch or inconclusive result, investigate a specific structural hypothesis and report unresolved
differences honestly. Broad validation belongs to integration and shared infrastructure changes.

Changed-file selection does not cover unchanged callers of an edited header, so add the affected
caller or ABI bundle when needed. Selected-function relocation-masked comparison is authoritative;
whole-image comparison is diagnostic. Preserve `/OPT:NOREF` comparison and `/OPT:REF` runtime modes.
`just runtime-test` must not add test branches to matching bodies.

## Tests and verification

Do not add tests by default.

Add a test only when explicitly requested, or when reproducing a concrete observed correctness bug
that existing checks do not detect. Use an independently justified expected result. A plausible
hypothetical failure is not, by itself, a reason to add permanent test machinery.

For ordinary C++ recovery, focused retail comparison is the default validation. Do not add Python
tests that inspect the recovered source.

Do not add tests for exact source spelling, documentation wording, deleted filenames, current
inventory counts, internal helper call order, or assertions over unchanged generated reports. Test
fixtures may exercise live code; frozen outputs are not a substitute for exercising it.

Do not introduce a framework, parser, report, baseline, command, or production abstraction merely
to support a test.

When removing obsolete code or policy, remove its tests and test-only helpers. Do not preserve the
obsolete machinery to keep its tests green. Do not write tests asserting that the removed tests or
files stay deleted.

Run only existing checks relevant to the actual change. Repeat them only after a relevant input
changes or a specific error invalidates the result. Deleting policy-only tests does not require a
game build or a broad audit. Direct targeted pytest commands are supported; `just test <file>` still
selects the complete unit and repository directories before the extra argument and is not a narrow
file-only invocation.

## Workspace and publication

- Use the provided checkout. Do not create another worktree, Jujutsu workspace, clone, or baseline
  checkout unless explicitly requested. If multiple checkouts already exist, each needs a unique
  absolute `WIZ8_WORK_DIR`; never share, copy, or hardlink a live Ghidra project.
- Use supported `bd` commands. Claim a concrete Bead before substantial work. Record accepted facts,
  consequential rejected hypotheses, and unresolved blockers; batch related experiments into one
  concise note. Include reproduction detail only for non-obvious conclusions.
- Use Jujutsu, not raw Git. Start from integrated `main` on `agent/<bead>-<topic>`. Keep the coherent task stack there; never move global `main` to unfinished work.
- Rebase when upstream evidence is needed and once immediately before integration. Rerun only checks
  invalidated by the rebase.
- Publication commands are in `docs/contributor-workflow.md`. A requested PR uses that workflow. Direct `main` publication requires explicit authorization and post-push proof that local `main` equals `main@origin` with an empty tree diff.
