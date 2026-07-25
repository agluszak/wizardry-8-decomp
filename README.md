# Wizardry 8 matching-decompilation bootstrap

This repository contains reproducible tooling and analysis metadata, not Wizardry 8 game files.
All commands use the single typed `wiz8` CLI through `uv`.

Copy `.env.example` to `.env`, set the three absolute machine paths, and copy
`config/local-inputs.example.yml` to the gitignored `config/local-inputs.yml`. Input paths in
that file are relative to `WIZ8_INPUT_DIR`; roles are explicit and are never guessed from names.

```sh
uv sync
uv run wiz8 doctor
uv run wiz8 inputs scan
uv run wiz8 extract all
uv run wiz8 variants materialize
uv run wiz8 inventory
uv run wiz8 pipeline verify
uv run wiz8 ghidra import --all
uv run wiz8 ghidra fid build-image
uv run wiz8 ghidra fid build-seeds
uv run wiz8 ghidra fid extract-libraries
uv run wiz8 ghidra fid build
uv run wiz8 report bootstrap
```

Generated reports live under the gitignored `build/` directory. Extracted files, materialized
variants, live Ghidra projects, and Wine prefixes live under `WIZ8_WORK_DIR` outside this checkout.
The FID workflow and current VC6 evidence are recorded in [docs/fid.md](docs/fid.md).

Variant materialization and PE inventory intentionally produce separate validated documents:

- `build/manifests/variant-provenance.json` records how each runnable tree was assembled.
- `build/manifests/variant-module-inventory.json` records module counts for each tree.

Their exact schemas are checked when loaded; command order cannot change the meaning of either
path. Generated state is disposable, so these initial schemas deliberately have no compatibility
version or migration layer.

Extraction and variant trees are published only after successful construction in a temporary
sibling directory. Their receipts bind input hashes, the relevant configuration, implementation
source hashes, extractor identities, and the complete output tree. `uv run wiz8 pipeline verify`
rehashes those trees. A rejected generated tree must be removed explicitly with either:

```sh
uv run wiz8 pipeline clean --stage variants
uv run wiz8 pipeline clean --stage extractions
```

Cleaning `extractions` also removes downstream variants; neither command touches configured inputs.
