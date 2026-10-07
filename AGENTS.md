# Wizardry 8 reconstruction

This repository reconstructs Wizardry 8 and SurRender from retail binaries and accepted original-source evidence using the original VC6-era toolchain.

## Current stage

The products are substantially reconstructed and runnable. The remaining work is to turn the recovered source into a faithful, coherent historical codebase:

- correct residual semantic, type, layout, ABI and ownership mistakes;
- recover shared helpers and original source structure where lowered code is still duplicated;
- remove reconstruction artifacts, stale recovery machinery and unnecessary complexity;
- establish a clean historical baseline for a future portable/modernized version.

Binary similarity is evidence, not the objective. Prefer plausible authored source even when equivalent compiler output still differs.

## Source fidelity

- Retail instructions, call sites and accepted original-source evidence outrank inferred Ghidra types, recomp PDBs, decompiler output and comparison heuristics.
- Recover plausible circa-2000 C/C++. Do not shape source solely to influence registers, stack layout, instruction scheduling, CFG shape or decompiler text.
- Fix type, layout, ABI and ownership errors at their canonical declaration or owner. Do not hide them with caller-local casts, aliases, invented unions or byte-offset tricks.
- Compiler artifacts such as ICF, storage reuse, inlining, thunks, deleting destructors and template emissions are not automatically authored source.
- Prefer an existing or evidence-backed shared abstraction over duplicated lowered implementations. Let the compiler own inlining.
- Preserve established retail bugs and undefined behavior in the historical reconstruction.
- Keep genuinely unknown facts unknown. Do not turn an inference into a stronger claim than the evidence supports.

See [source fidelity](docs/source-fidelity.md) for the remaining recovery-specific constraints.

## Project ownership

Keep one owner per concern:

- C/C++ source owns the reconstructed program, declarations, source placement and build graph.
- Ghidra owns retail binary analysis.
- reccmp owns entity identity/pairing and comparison preparation.
- Ghidriff owns decompiled-code differencing.
- `uv run wiz8` composes project workflows.

Do not add parallel comparison engines, symbol databases, source inventories, generic query layers or runtime harnesses when an existing owner already covers the need.

Source and translation-unit ownership is documented in [the source model](docs/wiz8-source-model.md). SurRender consumer/provider ABI rules are documented in [import visibility](docs/libraries/surrender-import-visibility.md).

A fact projected from recovered source or a recomp PDB into retail Ghidra is useful analysis annotation, but is not independent retail evidence confirming that same source fact.

## Working style

Prefer simple code that could realistically have existed in the original codebase. Fix systemic causes rather than individual symptoms.

When repeated code looks compiler-expanded, investigate a shared helper, template or header-visible definition rather than preserving duplication. Do not use `__forceinline`, noinline controls, optimizer pragmas or source-level aliases merely to chase retail code generation.

Search the existing source and accepted oracles before inventing a new abstraction. One entity should have one canonical owner and one evidence-backed type.

Use Ghidra, comparison and runtime evidence when they answer a concrete question; they are not mandatory rituals after every edit. Runtime behavior is authoritative for behavioral questions, but a passing scenario proves only what it observes.

Repository-internal tools and formats have no compatibility contract. Delete or replace obsolete machinery instead of adding compatibility layers.

## Verification

Use the smallest check that can invalidate the change while developing. Reuse successful results while their relevant inputs have not changed.

Before publishing a substantial source change, run the relevant changed comparison and project checks. Run runtime scenarios only when the change plausibly affects the behavior they observe. Tooling-only changes should use focused tooling tests; documentation-only changes need diff/link inspection, not product tests.

A failing gate is normally a source/model/tooling defect to fix, not a reason to accumulate waivers.

## Repository state

Use the provided checkout and preserve unrelated work. Do not commit extracted trees, live Ghidra projects or build products. Only reviewed GZF checkpoints listed in `vendor/ghidra/exports/manifest.json` may be tracked.

After adopting a revision with a changed reviewed Ghidra checkpoint, run `uv run wiz8 doctor` before relying on live retail analysis. Checkpoint handling is documented in [Ghidra checkpoints](docs/ghidra-checkpoints.md).
