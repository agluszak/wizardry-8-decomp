# Wizardry 8 decompilation

This is a Jujutsu repository for evidence-driven matching decompilation. Use Beads for durable task state and `just` for supported workflows.

## Authority

- Ghidra owns live analysis state: functions, names, signatures, labels, types, fields, enums, vtables, references, comments, and decompiler state.
- Git owns recovered C++, declarations, translation-unit order, compiler settings, matching markers, and provenance claims. Generated `build/` projections are disposable.
- Provenance records why an identity is accepted, its authority ceiling, aliases, confidence, and observations; it does not duplicate Ghidra's model.
- Never commit binaries, extracted trees, live Ghidra projects, or build products. Only reviewed GZF checkpoints in `vendor/ghidra/exports/manifest.json` may be tracked.
- Do not reverse engineer available source. Use pinned Windows/MSVC runtime, SGP, zlib, IJG, and Info-ZIP source/header oracles. SurRender remains recoverable.
- Search `third_party/sfi-sgp/sgp` before using any SGP interface and include its owning header. Put missing product headers under `include/wiz8/sgp-compat` by their original names; never invent replacement interfaces. Preserve SFI-SCLA and upstream notices.
- Search before declaring anything. Extend the canonical owner; do not add duplicate externs, guessed aliases, raw vtable calls, wrappers, or parallel inventories.
- Repository-owned Wizardry and SurRender code is unconditional C++. Retain `extern "C"` only for proven C linkage; never add C fallback APIs.

## Recovery

Use `just context 0x<address>` for the joined live view. It opens the reviewed program through the single short-lived owner in `ghidra/env.py`. Use Ghidra UI/API for listing, symbols, types, references, and decompiler work. Use undo or a temporary GZF for experiments. Do not create daemons, parallel stores, or manual project ownership.

`just recover 0x<address>` drafts, inserts, builds, and compares a first-pass port. It previews and restores by default; `--apply` retains the best non-regressing candidate. Recover the immediate call graph needed for the next visible product transition.

- Assume ordinary circa-2000 C++, VC6 ABI, and familiar container/lifecycle semantics unless evidence requires otherwise.
- Never invent code absent from retail, and never omit, stub, or approximate code retail contains.
- Do not invent wrapper types/APIs or speculative type boundaries to improve codegen. One object has one evidence-backed canonical type.
- A distinct vtable, lifecycle body, address, or template emission does not alone prove authored source. Compare canonical bases/templates first. Record emitted instantiations with `TEMPLATE`; keep generic definitions in canonical headers.
- Preserve proven translation-unit ownership and order in `src/wiz8/sources.cmake`. Keep address-qualified template emissions separate until ownership is proved.
- Use `matching-decomp` for source-model and comparison reasoning. Use `class-triage` before declaring an unnamed constructor, destructor, vtable, registry family, or class. Follow their linked references rather than expanding standing instructions here.

Faithfulness is absolute; byte identity is incremental. The pinned VC6 build falsifies source hypotheses. Compare immediately after changing layout, widths, signedness, return/calling convention, lifecycle shape, containers, or control flow. Revert proven regressions and record the hypothesis, exact command, before/after result, first divergence, and conclusion in the Bead. Do not brute-force inlining, scheduling, or register allocation. If instructions correspond one-for-one and only scratch registers differ, the body is done.

Marker rules enforced by `just check`: `FUNCTION` sits immediately above its declaration; `TEMPLATE` is followed immediately by a comment naming the emitted symbol and owns no body; `LIBRARY` is address-only.

Recover source placement before optimizer control. Ordinary functions stay unannotated. Header/class bodies require cross-translation-unit visibility evidence. Inline-control annotations require call-site evidence and improvement of the complete ABI bundle, and should be revisited later.

## Validation

Use the narrowest lane while iterating, then the full applicable lane after rebasing onto current `main`:

- Python, docs, evidence, markers, or inventory: `just check`.
- Ordinary local work: `just test`.
- Other C++ class declarations: `just lint`.
- Recovered body: `just build WIZ8`, focused `just compare ADDRESS...`, then `just test`.
- Inheritance, virtuals, `srClassSupport`, constructors, or destructors: `just lint`, `just build WIZ8`, focused compare and `just vtable CLASS`, then `just test`.
- Reviewed Ghidra change: representative contexts, `uv run wiz8 ghidra index`, then intentional checkpoint refresh.
- Integration: `just verify`.

Selected-function relocation-masked comparison is authoritative; whole-image comparison is diagnostic. Preserve `/OPT:NOREF` comparison and `/OPT:REF` runtime modes. `just runtime-test` is separate and must not add test branches to matching bodies.

A red integration gate requires a clean sibling baseline at the exact rebased `main`. Integrate only when narrower lanes pass and the topic adds no failures; record base commit, baseline/topic failures, and empty delta. Do not fix unrelated failures without expanded scope.

## Workspace and publication

- Give every checkout a unique absolute `WIZ8_WORK_DIR`. The live project belongs at `ghidra-project/`; never share, copy, or hardlink it. Builds lock `build/decomp`.
- Use supported `bd` commands. Claim a concrete Bead before substantial work; record partial and negative evidence; close only after acceptance and validation.
- Use Jujutsu, not raw Git. Start from integrated `main` on `agent/<bead>-<topic>`. Keep the coherent task stack there; never move global `main` to unfinished work.
- Rebase when upstream evidence is needed and once immediately before integration. If `main` moves during the final sequence, inspect, rebase again, and rerun affected gates.
- Publication commands are in `docs/contributor-workflow.md`. A requested PR uses that workflow. Direct `main` publication requires explicit authorization and post-push proof that local `main` equals `main@origin` with an empty tree diff.
