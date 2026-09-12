# Wizardry 8 matching decompilation

This repository contains reproducible tooling and analysis metadata, not Wizardry 8 game files.
Ghidra owns binary analysis, C++ owns recovered source, Python composes agent workflows and
host/Docker/Wine orchestration, CMake owns compilation and linking, and `uv run wiz8` is the single
workflow interface.

The shared Standard Gaming Platform source is distributed under Strategy First's non-commercial
SFI Source Code License Agreement in `src/sgp`. This project accepts those terms;
the reconstructed component is not offered under broader or commercial-use terms.

For initial setup, copy `.env.example` to `.env`, set its absolute machine paths, and copy
`config/local-inputs.example.yml` to the gitignored `config/local-inputs.yml`. Only the primary GOG
installer is required for the normal bootstrap; configured demo, patch, and compatibility-fix inputs
may be absent and remain optional corpus material. The following commands are available; this is not
a required sequence. Follow `AGENTS.md` for change-specific verification:

```sh
uv sync --frozen
uv run wiz8 doctor
uv run wiz8 prepare
uv run wiz8 toolchain build vc6-sp5 # once, or after the toolchain Dockerfile changes
uv run wiz8 check
uv run wiz8 lint
uv run wiz8 build
uv run wiz8 compare ADDRESS...
uv run wiz8 run
uv run wiz8 run --original
uv run wiz8 debug
uv run wiz8 runtime-test
```

`prepare` idempotently materializes the primary game's extraction and `gog-base` variant plus pinned
source dependencies; optional corpus variants stay behind explicit `wiz8 corpus` operations. `build`
configures automatically. `compare` is reccmp's live linked-image and exact-body diagnostic. Run
Python tests directly with `uv run pytest -q PATH`.

`lint` compile-checks the recovered C++ with clang-cl diagnostics, over the same component object
targets the VC6 product build links; `diagnostics` is its non-gating variant. `check` is the fast
public lane: ruff, pyright, repository validators and Python tests, with no product build. Its
compiler-backed source-index writer also validates synthetic-marker shape and cross-TU external
declarations. The lint compile database is adapted to host paths and reccmp performs one cached native
source-index collection; selected `compare` refreshes that same projection before function selection.
`uv run wiz8 analyze source-index` is therefore an inspection/debug command, not a prerequisite for
`check` or `compare`.

`uv run wiz8 run` stages the prepared game under `build/runtime/wiz8`, copies the already-built
`Wiz8Runtime.exe` into it, and launches it with `/WINDOW`. Source variants stay immutable. The process
output is forwarded, and when a crash marker appears the MAP-symbolized report is included in the
command result. `uv run wiz8 run --original` stages and launches the retail `Wiz8.exe` the same way;
`run` does not build the product.
`uv run wiz8 debug` stages the recomp under `build/runtime/debug` and drives it through Wine's GDB
proxy with a deterministic stop policy, then symbolizes the captured frames.

`uv run wiz8 runtime-test` runs the canonical deterministic semantic-scenario suite in the optimized
semantic-test executable, including the real new-game-to-main-game transition. The real menu handlers
execute on the UI thread; the host reruns the scenarios in reverse order and requires identical
normalized observations. Its same-process exception handler records every general-purpose register
and scans registers as well as stack words for first-party image addresses; Python symbolizes the
candidates against the runtime-test MAP and correlates their objects with the unresolved-symbol
report. Failures are not rerun under GDB.

Agent workflows live in the shared [matching-decomp](.agents/skills/matching-decomp/SKILL.md),
[class-triage](.agents/skills/class-triage/SKILL.md), and
[runtime-bringup](.agents/skills/runtime-bringup/SKILL.md) skills. The matching skill routes to native
Ghidra, checkpoint reconciliation, comparison, types, and source-oracle references as needed.

Generated reports and the CMake build directory (`build/decomp`) live under the gitignored `build/`
directory. Extracted files, materialized variants, and Wine prefixes use `WIZ8_WORK_DIR`. Each existing
checkout needs its own work directory and live Ghidra project; the project defaults to
`ghidra-project` in the checkout, with an absolute `WIZ8_GHIDRA_PROJECT_DIR` override available.
Do not share or copy a live project between checkouts.
The distinction between configuration, observations, reviewed conclusions, generated reports, and
exceptional proprietary-input snapshots is defined in
[docs/evidence-policy.md](docs/evidence-policy.md).

An address-marked C++ declaration owns a recovered Wiz8 function's source identity, signature, and
placement. Ghidra owns live analysis for both recovered and analysis-only functions; disagreements
are resolved from retail/source evidence at the canonical owners. Atomic `claims.csv` rows explain
provenance without recreating the source model. Class relationships
and virtual declarations live in C++ beside `// VTABLE` markers and `WIZ8_ASSERT_SIZE` gates. There
is no tracked function, class, vtable, field, or signature catalogue. Source-layout checks export
Ghidra state and use it with the rebuilt PDB.

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

No Wiz8 build carries first-party debug information, and the game was linked without RTTI. The
reviewed Ghidra project therefore owns surviving exception, call, vtable, global, and function
facts. Read native objects through PyGhidra and project only the needed output under `build/`;
external debug artifacts and the SurRender ABI retain snapshot producers. The authority and
extension rules are in [docs/wiz8-symbol-evidence.md](docs/wiz8-symbol-evidence.md).

Variant materialization and PE inventory intentionally produce separate validated documents:

- `build/manifests/variant-provenance.json` records how each runnable tree was assembled.
- `build/manifests/variant-module-inventory.json` records module counts for each tree.

Their exact schemas are checked when loaded; command order cannot change the meaning of either
path. Generated state is disposable, so these initial schemas deliberately have no compatibility
version or migration layer.

Extraction and variant trees are published only after successful construction in a temporary
sibling directory. Their receipts bind input hashes, configuration, implementation source hashes,
extractor identities, and the complete output tree. `uv run wiz8 corpus verify` rehashes those trees.
A rejected generated tree must be removed explicitly with either:

```sh
uv run wiz8 corpus clean --stage variants
uv run wiz8 corpus clean --stage extractions
```

Cleaning `extractions` also removes downstream variants; neither command touches configured inputs.
