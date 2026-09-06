---
name: matching-decomp
description: Recover Wizardry 8 C++ bodies and declarations against the pinned VC6 target; use for source-model changes and interpreting focused comparison mismatches.
---

# Matching workflow

Recover connected groups of ordinary circa-2000 C++ bodies, preserving canonical types and translation-unit ownership. Gather their evidence together with `just context ADDRESS... --listing --no-match` and reuse it. Inferred signatures and generated bodies can be wrong; retail instructions and call sites decide. A failed automatic export is a reason to recover manually or fix the exporter, not a prerequisite to wait on.

Build and compare the first-pass batch once with `just compare ADDRESS...` or `just compare --changed`. Use `--no-build` if that source state was already built. Changed-file selection covers all marked functions in those files; explicitly include unchanged callers affected by a header/ABI change. Reserve immediate before/after comparisons for experiments on previously matched code.

Review structured divergences against the instruction evidence. Neither a low percentage nor an inconclusive classifier result proves a semantic defect. Record actual regressions and negative experiments in the active Bead; do not tune scratch registers or compiler scheduling. Run broad validation for the completed batch, not after each body. Use `--deep` only when the listing and decompiler cannot answer a concrete question.

Use the references for the specific evidence pattern encountered: `references/full-guide.md` is the
legacy detailed guide; consult only the relevant section when a mismatch, layout, or ABI question
requires it.
