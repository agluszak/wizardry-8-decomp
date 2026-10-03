---
name: ghidra-analysis
description: Inspect or edit canonical Wizardry 8 retail Ghidra analysis while preserving evidence provenance.
---

# Ghidra analysis

Use this skill for the retail analysis model itself: instructions/P-code, references, functions,
prototypes/storage, data types, vtables, or evidence-backed ProgramDB corrections.

Source recovery lives in [matching-decomp](../matching-decomp/SKILL.md); source type/layout decisions
live in [type-modeling](../type-modeling/SKILL.md).

## Provenance boundary

Ghidra stores facts from several origins. Keep them conceptually distinct:

- retail-binary/manual reviewed facts;
- Ghidra analysis inference;
- recovered-source projection;
- recomp PDB/compiler projection.

A fact projected from current recovered source or recomp PDB into retail ProgramDB is useful for
decompiler quality, but it is **not independent retail evidence** that can confirm the same source
declaration. Agreement between source-projected retail ProgramDB and recomp PDB may be circular.

When comparison preparation needs independently reviewed retail ABI/type evidence, require provenance
that excludes current-source/PDB projection.

Keep uncertain facts unknown. Heuristic inference is investigation material until independently established.

## Ordinary reads

Routine recovery usually needs only:

```sh
uv run wiz8 ghidra decompile ADDRESS...
uv run wiz8 ghidra asm ADDRESS...
uv run wiz8 ghidra sym ADDRESS...
uv run wiz8 ghidra class NAME
```

Batch related addresses. Reads do not compile, refresh the source index or synchronize ProgramDB.

Open native PyGhidra only when the existing read commands cannot answer the question. Use the
checkout-owned project and native Program APIs; do not create scratch projects, daemons or generic
query wrappers.

## Editing established facts

Apply only evidence-backed corrections in coherent transactions and save once. Correct the source owner
as well when the established fact belongs in recovered C++.

`uv run wiz8 ghidra sync` is the single established-source/evidence -> ProgramDB projection path.
It improves analysis; it is not a retail-discovery oracle.

Read [source import](references/source-import.md) only when synchronizing/regenerating source/PDB
projection, [analysis enrichment](references/analysis-enrichment.md) for class/type projection, and
[checkpoints](references/checkpoints.md) only when sharing/restoring reviewed GZF state.

## Freshness

Run `uv run wiz8 doctor` before the first Ghidra-derived recovery step on a genuinely new base or
after a revision changes the reviewed Ghidra manifest/checkpoint. `stale`, `untracked` or
`unknown` live-project provenance blocks retail-derived conclusions until explicitly reconciled.

A current reviewed seed and a current source projection are separate facts.

## Efficiency

Reuse one decompiler session/interface for a related batch. After an analysis edit, flush/discard stale
decompiler results and re-decompile only affected functions. Do not rerun whole-program analysis by
default.

Decompiler output depends on the stored prototype and types. Missing high-level use is not evidence
that a parameter/field does not exist.
