# Wizardry 8 matching decompilation

This repository contains reproducible tooling and analysis metadata, not Wizardry 8 game files.
Each layer has one owner: `just` provides short human aliases, Python owns host/Docker/Wine
orchestration, CMake owns compilation and linking, and the evidence package owns canonical reads and
validation.

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
just runtime-test
just verify
```

`prepare` idempotently materializes configured inputs and pinned source dependencies. `build`
configures automatically. `compare` is a linked-image diagnostic; relocation-masked
`just wiz8 verify-boundaries` is the recovered-body criterion. `just test` runs the public unit
and repository-invariant lanes.

`just runtime-test` builds a separate test product and runs named main-menu scenarios inside the
process. The real menu handlers execute on the UI thread; the host reruns the scenarios in reverse
order and requires identical normalized observations. Test-only code is not linked into either
matching image.

For focused recovery, one reccmp process can compare or triage several original addresses, or all
`FUNCTION` markers in a source file. The wrappers consume reccmp's structured result; they do not
infer meaning by parsing rendered disassembly.

```sh
just compare 0x00406b70 0x00406ba0
just triage --file src/wiz8/local_code/Combat.cpp
just vtable W8Widget
just datacmp
just addr 0x00406b70
```

Generated reports live under the gitignored `build/` directory. Extracted files, materialized
variants, live Ghidra projects, and Wine prefixes live under `WIZ8_WORK_DIR` outside this checkout.
The CMake build directory is `build/decomp` inside the checkout, so several checkouts can share one
`WIZ8_WORK_DIR` without overwriting each other's build or comparison state.
The distinction between configuration, observations, reviewed conclusions, generated reports, and
exceptional proprietary-input snapshots is defined in
[docs/evidence-policy.md](docs/evidence-policy.md).

Ghidra owns operational analysis state: functions, symbols, signatures, structures, fields, vtables,
comments, cross-references, and decompiler state. Restore the validated reviewed checkpoint once,
then review and edit that project directly. There is no evidence replay, materialization cache,
query daemon, or speculative overlay layer.

```sh
uv run wiz8 ghidra restore
uv run wiz8 ghidra sync-source --apply
just context 0x0044bec0
uv run wiz8 ghidra index
```

`just context` is the supported joined recovery view: it combines the current decompilation and
call graph with provenance, source ownership, match state, cross-build mappings, strings, and
relevant fields. `ghidra index` writes disposable normalized review views to
`build/ghidra-index/` and refuses provenance claims whose function/type/vtable entity no longer
exists. `ghidra sync-source` is the only source-to-Ghidra mutation: it applies names for explicitly
marked functions and leaves every unclaimed analysis entity alone. Ghidra-to-source remains a
review workflow through `just context`; no generator rewrites C++. `ghidra seed refresh` is an intentional checkpoint operation, not a routine
consequence of editing evidence.

An address-marked C++ declaration is the authority for a recovered Wiz8 function's address, name,
signature, and source ownership. Ghidra owns analysis-only functions that have no owned declaration;
atomic `claims.csv` rows explain provenance without recreating the source model. Class relationships
live in `class-provenance.csv`. There is no tracked function or signature catalogue. Full
verification exports Ghidra state and uses it for claim resolution and VC6 source-layout checks.

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

No Wiz8 build carries debug information, and the game was linked without RTTI, so class and type
evidence comes from what survived anyway: C++ exception metadata, the SurRender export tables, the
literals passed to diagnostic calls, and what the relocation table says about vtables and globals.
`wiz8 evidence refresh eh-metadata`, `surrender-abi`, `call-sites`, `polymorphism`, `globals`, and
`function-census`
produce those snapshots. How they are refreshed, what each can and cannot establish, and how to join
them to find a function's translation unit, the type in a frame slot, or the next function worth
porting are in [docs/wiz8-symbol-evidence.md](docs/wiz8-symbol-evidence.md).

Variant materialization and PE inventory intentionally produce separate validated documents:

- `build/manifests/variant-provenance.json` records how each runnable tree was assembled.
- `build/manifests/variant-module-inventory.json` records module counts for each tree.

Their exact schemas are checked when loaded; command order cannot change the meaning of either
path. Generated state is disposable, so these initial schemas deliberately have no compatibility
version or migration layer.

Extraction and variant trees are published only after successful construction in a temporary
sibling directory. Their receipts bind input hashes, configuration, implementation source hashes,
extractor identities, and the complete output tree. `just wiz8 corpus verify` rehashes those trees.
A rejected generated tree must be removed explicitly with either:

```sh
just wiz8 corpus clean --stage variants
just wiz8 corpus clean --stage extractions
```

Cleaning `extractions` also removes downstream variants; neither command touches configured inputs.
