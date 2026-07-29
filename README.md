# Wizardry 8 matching decompilation

This repository contains reproducible tooling and analysis metadata, not Wizardry 8 game files.
Each layer has one owner: `just` provides short human aliases, Python owns host/Docker/Wine
orchestration, CMake owns compilation and linking, Ghidra owns live analysis state, and the evidence
package owns provenance and externally reproducible reviewed claims.

The shared Standard Gaming Platform source is distributed under Strategy First's non-commercial
SFI Source Code License Agreement in `third_party/sfi-sgp/sgp`. This project accepts those terms;
the vendored subtree is not offered under broader or commercial-use terms.

Copy `.env.example` to `.env`, set its absolute machine paths, and copy
`config/local-inputs.example.yml` to the gitignored `config/local-inputs.yml`. Then use the
supported daily workflow:

```sh
uv sync --frozen
just wiz8 doctor
just prepare
just check
just build
just compare
just run
just verify
```

`prepare` idempotently materializes configured inputs and pinned source dependencies. `build`
configures automatically. `compare` is a linked-image diagnostic; relocation-masked
`just wiz8 verify-boundaries` is the recovered-body criterion. `just test` runs the public unit
and repository-invariant lanes.

Generated reports live under the gitignored `build/` directory. Extracted files, materialized
variants, the live Ghidra project, and Wine prefixes live under `WIZ8_WORK_DIR` outside this
checkout. The CMake build directory is `build/decomp` inside the checkout, so several checkouts can
share immutable input trees without sharing build or Ghidra state. The distinction between
configuration, observations, reviewed conclusions, generated reports, and exceptional
proprietary-input snapshots is defined in [docs/evidence-policy.md](docs/evidence-policy.md).

## Ghidra workflow

Each checkout owns one live project at `WIZ8_WORK_DIR/ghidra` unless
`WIZ8_GHIDRA_PROJECT_DIR` overrides it. Create or deliberately refresh it with:

```sh
CANON=wiz8--gog-base--wiz8--18a74ff61c65
just ghidra rebuild "$CANON"
```

Queries are read-only one-shot PyGhidra batches. They open that project once, execute every
repeated `--query` clause in order, and close it. They do not restore a GZF, replay CSVs, create a
content-addressed clone, or keep a socket daemon alive:

```sh
just ghidra query "$CANON" \
  -q 'function 0x004B6900' \
  -q 'read-data 0x005ED090 16' \
  -q 'search "Monster Info"'
```

The tracked GZF under `vendor/ghidra/exports/` is a validated disaster-recovery seed, not the
canonical analysis database. `just ghidra seed refresh "$CANON"` intentionally validates and packs
the current project. No accepted fact may exist only in that archive.

Source recovery normally starts from the joined context packet. Add `--deep --root this` only when
the full listing, normalized P-code, rooted field accesses and anonymous type variables are needed:

```sh
just wiz8 report context 0x0044bec0 --program "$CANON"
just wiz8 report context 0x0044bec0 --program "$CANON" --deep --root this
```

Speculative work belongs in an ordinary disposable Ghidra project copy or a Ghidra transaction,
not in a project-specific overlay/inference framework. Promote durable conclusions through the
reviewed evidence workflow and deliberately rebuild when the replay representation changes.

Reconstructed debug transfer additionally requires current body proof:

```sh
just build WIZ8_GAMEPLAY_BOUNDARIES
just wiz8 verify-boundaries
just wiz8 reconstructed-transfer
```

The command recomputes the recorded relocation-masked digests itself. Calling convention and stack
shape may have exact-body authority, while reconstructed semantic types and parameter names remain
separately labelled candidate/source components.

The FID workflow and current VC6 evidence are recorded in [docs/fid.md](docs/fid.md). Active source
recovery starts with the byte-identical SurRender JPEG extension; its address-backed ownership and
interface findings are in [docs/targets/srext-jpegimporter.md](docs/targets/srext-jpegimporter.md).
The second reviewed library boundary covers Info-ZIP 5.4 and its SurRender adapter in
[docs/targets/srext-unzip.md](docs/targets/srext-unzip.md). The canonical executable's recovered zlib
block is documented in [docs/libraries/zlib-1.0.4.md](docs/libraries/zlib-1.0.4.md). Its exact
compiler-support matches are separated in
[docs/libraries/msvc6-runtime.md](docs/libraries/msvc6-runtime.md).

Names in this repository come from sources of very different authority: an exact-matching released
SGP source function, an official cross-build boundary, a CFAgent signature seed, and a Cosmic Forge
editor label are not interchangeable evidence. Every reviewed identity therefore records where its
name came from and how far that may be trusted, validated against a closed vocabulary. The model is
[docs/wiz8-evidence-model.md](docs/wiz8-evidence-model.md).

No Wiz8 build carries debug information, and the game was linked without RTTI, so class and type
evidence comes from what survived anyway: C++ exception metadata, the SurRender export tables, the
literals passed to diagnostic calls, and what the relocation table says about vtables and globals.
`wiz8 eh-metadata`, `wiz8 surrender-abi`, `wiz8 call-sites`, `wiz8 polymorphism`, `wiz8 globals` and
`wiz8 function-census` produce those snapshots. How they are refreshed, what each can and cannot
establish, and how to join them to find a function's translation unit, the type in a frame slot, or
the next function worth porting are in [docs/wiz8-symbol-evidence.md](docs/wiz8-symbol-evidence.md).

Variant materialization and PE inventory intentionally produce separate validated documents:

- `build/manifests/variant-provenance.json` records how each runnable tree was assembled.
- `build/manifests/variant-module-inventory.json` records module counts for each tree.

Their exact schemas are checked when loaded; command order cannot change the meaning of either path.
Generated state is disposable, so these initial schemas deliberately have no compatibility version
or migration layer.

Extraction and variant trees are published only after successful construction in a temporary
sibling directory. Their receipts bind input hashes, configuration, implementation source hashes,
extractor identities, and the complete output tree. `just wiz8 corpus verify` rehashes those trees.
A rejected generated tree must be removed explicitly with either:

```sh
just wiz8 corpus clean --stage variants
just wiz8 corpus clean --stage extractions
```

Cleaning `extractions` also removes downstream variants; neither command touches configured inputs.
