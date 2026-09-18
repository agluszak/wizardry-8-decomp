# Source → Ghidra enrichment

Two related workflows project recovered source into the canonical Ghidra program.
Both mutate reviewed analysis state. Neither is a prerequisite for ordinary
inspection or body recovery. Score changes with
`uv run wiz8 analyze decompiler-quality` (see
[analysis enrichment](analysis-enrichment.md)).

## Periodic enrichment checkpoint (preferred)

Use this when the live program should absorb **high-confidence** recovered facts
without a full canonical regeneration ritual. Default mode is measurement-only.

```sh
# Plan + quality sample (no writes)
uv run wiz8 analyze enrichment-checkpoint

# Apply source-backed calling conventions, re-score
uv run wiz8 analyze enrichment-checkpoint --apply-conventions

# Also project matched PDB/reccmp names, signatures, types, and source lines
uv run wiz8 build WIZ8
uv run wiz8 analyze enrichment-checkpoint --apply-conventions --import-source
```

Then spot-check and, when intentionally reviewed, refresh the GZF:

```sh
uv run wiz8 report context ADDRESS...
uv run wiz8 ghidra seed refresh wiz8
```

Evidence boundary: only matched/source-backed entities belong in reviewed state.
Do not point the importer at a derived/cached project. Soft Param-ID guesses are
not part of this checkpoint.

## Full canonical regeneration

Use this when the task explicitly asks to rebuild canonical Ghidra analysis from
the current recovered source/PDB as a dedicated state-management operation.

1. Preserve the current reviewed checkpoint as a rollback point.
2. Build a current VC6 PDB:

   ```sh
   uv run wiz8 build WIZ8
   ```

3. Project matched source entities into the canonical live program:

   ```sh
   uv run python - <<'PY'
   from wiz8decomp.config import load_settings
   from wiz8decomp.ghidra.reccmp_import import import_reccmp_source

   print(import_reccmp_source(load_settings(), "wiz8"))
   PY
   ```

   Or via the checkpoint:

   ```sh
   uv run wiz8 analyze enrichment-checkpoint --apply-conventions --import-source
   ```

   The importer matches recompiled entities to original addresses and applies
   names, signatures, types and source lines. Review its statistics; known
   unsupported insertion/type/parameter cases need interpretation rather than a
   blanket zero-error requirement.

4. Spot-check the affected identities, then export the reviewed checkpoint:

   ```sh
   uv run wiz8 report context ADDRESS...
   uv run wiz8 ghidra seed refresh wiz8
   ```

When only the source-layout audit is needed, use
`uv run wiz8 analyze source-layouts`; that workflow operates on a disposable
derived project and leaves canonical state untouched.
