---
name: matching-decomp
description: Recover Wizardry 8 C++ bodies and declarations against the pinned VC6 target; use for source-model changes and interpreting focused comparison mismatches.
---

# Matching workflow

1. Start with `just context <function selector>`; it is the joined recovery view.
2. Confirm canonical source/Ghidra ownership before adding declarations or markers.
3. Recover ordinary circa-2000 C++ and preserve translation-unit ownership.
4. After each source-model change, run a focused `just compare <selector>`.
5. Treat compiler/reccmp output as falsification evidence; record negative experiments in the active Bead.
6. Escalate to `--deep` or raw analysis only when compact context cannot answer the question.
7. Run broad repository validation at handoff, not after every small edit.

Use the references for the specific evidence pattern encountered: `references/full-guide.md` is the
legacy detailed guide; consult only the relevant section when a mismatch, layout, or ABI question
requires it.
