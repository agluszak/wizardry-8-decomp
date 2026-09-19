# Wizardry 8 matching decompilation

Evidence-driven reconstruction of the Windows release of *Wizardry 8*. The goal is to recover plausible
authored C/C++ and the original program structure while incrementally matching the retail MSVC 6
machine code and restoring runnable behavior.

This repository contains recovered source, analysis metadata, and reproducible tooling. It does **not**
contain Wizardry 8 game files.

The shared Standard Gaming Platform source under `src/sgp` is distributed under Strategy First's
non-commercial SFI Source Code License Agreement. This project accepts those terms; reconstructed code
derived from that source is not offered under broader or commercial-use terms.

## Setup

The normal workflow uses a legally obtained GOG installer as the canonical game input. Demo builds,
patches, and compatibility fixes are optional research corpus inputs.

Create the local configuration:

```sh
cp .env.example .env
cp config/local-inputs.example.yml config/local-inputs.yml
```

Edit `.env` with absolute paths for Ghidra, the input directory, and a checkout-specific
`WIZ8_WORK_DIR`. Set `gog-media` in `config/local-inputs.yml` to the canonical GOG installer.

Bootstrap the Python environment, toolchain, and extracted inputs:

```sh
uv sync --frozen
uv run wiz8 doctor
uv run wiz8 toolchain build vc6-sp5
uv run wiz8 prepare
```

For binary-analysis work, restore the reviewed Ghidra checkpoint into this checkout's live project:

```sh
uv run wiz8 ghidra restore
uv run wiz8 doctor
```

Each checkout must have its own `WIZ8_WORK_DIR` and live Ghidra project. Do not share or copy a live
Ghidra project between checkouts.

## Common commands

`uv run wiz8` is the single workflow interface. Use `uv run wiz8 --help` and subcommand help for
the complete command set.

| Command | Purpose |
| --- | --- |
| `uv run wiz8 check` | Fast repository validation: Python checks, repository invariants, and source-index refresh. |
| `uv run wiz8 lint` | Compile recovered C++ with the clang-cl structural diagnostics lane. |
| `uv run wiz8 build` | Build the comparison image used by reccmp. |
| `uv run wiz8 build runtime` | Build the runnable recovered image. |
| `uv run wiz8 compare 0xADDRESS...` | Compare selected recovered functions with the retail executable. |
| `uv run wiz8 compare --changed` | Compare functions affected by the current C/C++ changes. |
| `uv run wiz8 ghidra decompile 0xADDRESS...` | Decompile selected functions from the live ProgramDB. |
| `uv run wiz8 ghidra asm 0xADDRESS...` | Inspect annotated retail assembly. |
| `uv run wiz8 ghidra sym 0xADDRESS...` | Resolve functions, globals, fields, and imports. |
| `uv run wiz8 ghidra sync` | Project established source/evidence facts into the live ProgramDB. |
| `uv run wiz8 runtime-test --build` | Build and run the deterministic semantic runtime scenarios. |
| `uv run wiz8 run` | Stage and run the already-built recovered runtime image under Wine. |
| `uv run wiz8 run --original` | Stage and run the retail executable as a behavioral oracle. |
| `uv run wiz8 debug --build` | Build and debug the recovered runtime through Wine's GDB proxy. |
| `uv run wiz8 report status` | Generate current project-wide recovery/matching statistics. |
| `uv run wiz8 pr-check` | Run the validation lanes required for the current pull request. |

Generated reports, build products, extracted trees, runtime stages, and other projections are
disposable and live under `build/` or `WIZ8_WORK_DIR`; they are not alternate sources of truth.

## Project model

The repository deliberately keeps ownership narrow:

- C++ owns recovered source, declarations, layouts, and source placement.
- Ghidra owns live analysis of the retail binaries.
- reccmp owns original/recompiled code comparison.
- CMake owns compilation and linking.
- Python composes the workflows exposed by `uv run wiz8`.
- reviewed evidence records provenance and conclusions without duplicating the source or Ghidra model.

Recovery is evidence-driven. Retail instructions and call sites, accepted original-source oracles, and
reviewed binary evidence outrank decompiler guesses or a higher comparison score. Matching compiler
lowering is not a reason to write compiler-lowered C++.

For current recovery statistics, use `uv run wiz8 report status` rather than maintaining counts or
percentages in documentation.

## Documentation

Repository-wide recovery rules and verification policy are in [AGENTS.md](AGENTS.md). Task-specific
agent procedures live under [`.agents/skills`](.agents/skills/).

The main architectural references are:

- [Contributor workflow](docs/contributor-workflow.md)
- [Evidence and artifact policy](docs/evidence-policy.md)
- [Wizardry evidence and provenance model](docs/wiz8-evidence-model.md)
- [Wizardry executable target and runtime workflow](docs/targets/wiz8-executable.md)
- [Wizardry symbol evidence](docs/wiz8-symbol-evidence.md)

The focused recovery, Ghidra, type-modeling, runtime, and tooling procedures live in
[matching-decomp](.agents/skills/matching-decomp/SKILL.md),
[ghidra-analysis](.agents/skills/ghidra-analysis/SKILL.md),
[type-modeling](.agents/skills/type-modeling/SKILL.md),
[runtime-bringup](.agents/skills/runtime-bringup/SKILL.md), and
[tooling-maintenance](.agents/skills/tooling-maintenance/SKILL.md).
