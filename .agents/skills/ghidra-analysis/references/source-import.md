# Regenerate canonical Ghidra state from source

Use this workflow only when the task explicitly asks to regenerate/update canonical Ghidra analysis
from the current recovered source. It mutates reviewed state and is intentionally separate from
ordinary inspection, native edits, checkpoint refresh and source-layout auditing.

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

   The importer matches recompiled entities to original addresses and applies names, signatures,
   types and source lines. Review its statistics; known unsupported insertion/type/parameter cases
   need interpretation rather than a blanket zero-error requirement.

4. Spot-check the affected identities, then export the reviewed checkpoint:

   ```sh
   uv run wiz8 report context ADDRESS...
   uv run wiz8 ghidra seed refresh wiz8
   ```

Do not point the importer at a derived/cached project and do not run it speculatively. When only the
source-layout audit is needed, use `uv run wiz8 analyze source-layouts`; that workflow operates on a
disposable derived project and leaves canonical state untouched.
