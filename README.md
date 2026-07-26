# Wizardry 8 matching decompilation

This repository contains reproducible tooling and analysis metadata, not Wizardry 8 game files.
`just` is the normal task surface: it invokes CMake, reccmp, and pytest directly, while `just wiz8`
and `just ghidra` forward corpus- and Ghidra-specific arguments to the typed Python CLI. CMake owns
all compilation units and compiler flags for recovered binaries and source-built FID seeds.

The shared Standard Gaming Platform source is distributed under Strategy First's non-commercial
SFI Source Code License Agreement in `third_party/sfi-sgp/sgp`. This project accepts those terms;
the vendored subtree is not offered under broader or commercial-use terms.

Copy `.env.example` to `.env`, set the three absolute machine paths, and copy
`config/local-inputs.example.yml` to the gitignored `config/local-inputs.yml`. Input paths in
that file are relative to `WIZ8_INPUT_DIR`; roles are explicit and are never guessed from names.

```sh
uv sync
just wiz8 doctor
just wiz8 inputs scan
just wiz8 extract all
just wiz8 variants materialize
just wiz8 inventory
just wiz8 pipeline verify
just ghidra cache materialize
just ghidra fid build-image
just ghidra fid build-seeds
just ghidra fid extract-libraries
just ghidra fid build
just wiz8 report bootstrap
just build WIZ8
just compare WIZ8
```

The active matching target has a PDB-backed build and comparison loop:

```sh
just build-image
just build
just compare
just compare --verbose 0x10001000
just wiz8 report status
just test
```

The complete comparison is important: it loads the accepted IJG identities needed to prove calls
across object files. `just configure` asks reccmp to find the hash-pinned original in the materialized
GOG DLL tree. Build products and `reccmp-build.yml` remain under `WIZ8_WORK_DIR`; the local original
path is written only to the gitignored `reccmp-user.yml`.

Generated reports live under the gitignored `build/` directory. Extracted files, materialized
variants, live Ghidra projects, and Wine prefixes live under `WIZ8_WORK_DIR` outside this checkout.
The CMake build directory is `build/decomp` inside the checkout, so several checkouts can share one
`WIZ8_WORK_DIR` without overwriting each other's build or comparison state.
The distinction between configuration, observations, reviewed conclusions, generated reports, and
exceptional proprietary-input snapshots is defined in
[docs/evidence-policy.md](docs/evidence-policy.md).

The normal Ghidra path is GZF-first. `just ghidra query <program> ...` transparently restores the
validated canonical seed into a `CODEX_THREAD_ID`- or `WIZ8_GHIDRA_AGENT_ID`-isolated project,
replays current reviewed evidence, validates it, and reuses a persistent daemon. The tracked GZF is
a disposable binary cache, not canonical analysis knowledge. `just ghidra rebuild <program>` performs
the slower fresh import and auto-analysis parity check; `just ghidra cache build` validates and packs
an intentionally refreshed canonical seed.

Several read-only queries can share one daemon request by repeating `--query` (or `-q`). Results are
returned in request order, and each clause uses shell-style quoting when one argument contains spaces:

```sh
just ghidra query wiz8--gog-base--wiz8--18a74ff61c65 \
  -q 'function 0x004B6900' \
  -q 'read-data 0x005ED090 16' \
  -q 'search "Monster Info"'
```

The FID workflow and current VC6 evidence are recorded in [docs/fid.md](docs/fid.md).
Active source recovery starts with the byte-identical SurRender JPEG extension; its address-backed
ownership and interface findings are in
[docs/targets/srext-jpegimporter.md](docs/targets/srext-jpegimporter.md).
The second reviewed library boundary covers Info-ZIP 5.4 and its SurRender adapter in
[docs/targets/srext-unzip.md](docs/targets/srext-unzip.md).
The canonical executable's recovered zlib block is documented in
[docs/libraries/zlib-1.0.4.md](docs/libraries/zlib-1.0.4.md).
Its exact compiler-support matches are separated in
[docs/libraries/msvc6-runtime.md](docs/libraries/msvc6-runtime.md).

Names in this repository come from sources of very different authority: an exact-matching released
SGP source function, an official cross-build boundary, a CFAgent signature seed, and a Cosmic Forge
editor label are not interchangeable evidence. Every reviewed identity therefore records where its
name came from and how far that may be trusted, validated against a closed vocabulary. The model is
[docs/wiz8-evidence-model.md](docs/wiz8-evidence-model.md).

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
