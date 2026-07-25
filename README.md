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
