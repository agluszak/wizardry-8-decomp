# Source → Ghidra projection

`uv run wiz8 ghidra sync` is the one established-source/evidence → ProgramDB path.
It mutates live analysis. It is not a prerequisite for ordinary inspection: address-based
`ghidra decompile` / `ghidra asm` / `ghidra sym` read the existing ProgramDB even when
working C++ or the source index is broken.

Score decompiler effects with `uv run wiz8 analyze decompiler-quality` (see
[analysis enrichment](analysis-enrichment.md)).

## Ordinary synchronization

```sh
uv run wiz8 ghidra sync
```

This refreshes compiler-backed source metadata, materializes independently established
function entries, and applies names, prototypes, conventions, class/type/global/vtable
and import facts. Each apply row owns a Ghidra transaction so a failed row rolls back
without discarding successful ones. Provenance advances only when that pass completed
with no hard apply errors. Spot-check with:

```sh
uv run wiz8 ghidra decompile ADDRESS...
uv run wiz8 analyze decompiler-quality
uv run wiz8 ghidra seed refresh wiz8
```

`seed refresh` is deliberate reviewed-checkpoint publication. It is not a second sync
path. Evidence boundary: only established source/retail facts belong in reviewed state.
Parameter ID guesses are not part of ordinary sync.

## Full canonical regeneration

Use this when the task explicitly asks to rebuild canonical Ghidra analysis from
the current recovered source/PDB as a dedicated state-management operation.

1. Preserve the current reviewed checkpoint as a rollback point.
2. Build a current VC6 PDB:

   ```sh
   uv run wiz8 build WIZ8
   ```

3. Project compiler/PDB types while the live project is closed, then reconcile
   established source facts:

   ```sh
   uv run wiz8 ghidra sync --import-source
   ```

   `--import-source` runs `reccmp-ghidra-import` first (the importer is the only
   Ghidra writer during that step), then opens ProgramDB for the established-fact
   pass. The optional importer matches recompiled entities to original addresses and
   applies names, signatures, types and source lines. Review its statistics;
   known unsupported insertion/type/parameter cases need interpretation rather
   than a blanket zero-error requirement. A complete `--import-source` records the
   PDB hash and pinned reccmp revision so `doctor` can distinguish current source
   facts from stale compiler/PDB types.

4. Spot-check the affected identities, measure the exact live ProgramDB, then export:

   ```sh
   uv run wiz8 ghidra decompile ADDRESS...
   uv run wiz8 analyze decompiler-quality
   uv run wiz8 ghidra seed refresh wiz8
   ```

   `seed refresh` requires a successful `decompiler-quality` report whose
   `program_state` fingerprint matches the live ProgramDB. Re-measure after any
   later `ghidra sync`.

When only the source-layout audit is needed, use
`uv run wiz8 analyze source-layouts`; that workflow operates on a disposable
derived project and leaves canonical state untouched.
