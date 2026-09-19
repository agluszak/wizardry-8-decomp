# Wizardry 8 matching decompilation

Evidence-driven reconstruction of the Windows release of *Wizardry 8*. The project recovers plausible
authored C/C++ and program structure while incrementally matching the retail MSVC 6 machine code and
restoring runnable behavior.

This repository contains recovered source, analysis metadata, and tooling. It does **not** contain
Wizardry 8 game files.

The shared Standard Gaming Platform source under `src/sgp` is distributed under Strategy First's
non-commercial SFI Source Code License Agreement. This project accepts those terms; reconstructed code
derived from that source is not offered under broader or commercial-use terms.

## Getting started

The canonical input is a legally obtained GOG installer. Demo builds, patches, and compatibility fixes
are optional research corpus inputs.

```sh
cp .env.example .env
cp config/local-inputs.example.yml config/local-inputs.yml
```

Edit `.env` with absolute paths for Ghidra, the input directory, and a checkout-specific
`WIZ8_WORK_DIR`. Set `gog-media` in `config/local-inputs.yml` to the GOG installer, then bootstrap:

```sh
uv sync --frozen
uv run wiz8 doctor
uv run wiz8 toolchain build vc6-sp5
uv run wiz8 prepare
```

For binary-analysis work, restore the reviewed Ghidra checkpoint and rerun the preflight:

```sh
uv run wiz8 ghidra restore
uv run wiz8 doctor
```

Each checkout needs its own `WIZ8_WORK_DIR` and live Ghidra project. Never share or copy a live Ghidra
project between checkouts.

## Everyday commands

`uv run wiz8` is the project workflow interface. Use `uv run wiz8 --help` for the full command set.

| Command | Purpose |
| --- | --- |
| `uv run wiz8 check` | Fast repository validation and source-index refresh. |
| `uv run wiz8 lint` | Compile recovered C++ with the clang-cl structural diagnostics lane. |
| `uv run wiz8 build` | Build the comparison image used by reccmp. |
| `uv run wiz8 build runtime` | Build the runnable recovered image. |
| `uv run wiz8 compare 0xADDRESS...` | Compare selected functions with retail; use `--changed` for the current C/C++ change. |
| `uv run wiz8 ghidra decompile/asm/sym ...` | Inspect selected functions, instructions, and symbols in the live ProgramDB. |
| `uv run wiz8 ghidra sync` | Project established source/evidence facts into the live ProgramDB. |
| `uv run wiz8 runtime-test --build` | Build and run the deterministic semantic runtime scenarios. |
| `uv run wiz8 run` / `run --original` | Run the recovered or retail executable under Wine. |
| `uv run wiz8 debug --build` | Build and debug the recovered runtime through Wine's GDB proxy. |
| `uv run wiz8 report status` | Generate current recovery and matching statistics. |
| `uv run wiz8 pr-check` | Run the validation lanes required for a pull request. |

Do not copy status counts or generated inventories into documentation. Generated reports, build
products, extracted trees, runtime stages, and other projections belong under `build/` or
`WIZ8_WORK_DIR`.

## Repository guide

- `src/` and `include/` own recovered source, declarations, layouts, and source placement.
- Ghidra owns live analysis of the retail binaries; the reviewed checkpoint is under
  `vendor/ghidra/exports/`.
- `evidence/` contains observations and reviewed provenance that cannot be represented by source or
  live analysis alone.
- `tools/wiz8decomp/` provides the Python orchestration behind `uv run wiz8`; CMake owns compilation
  and linking, and reccmp owns original/recompiled comparison.
- `.agents/skills/` contains task-specific recovery procedures. `AGENTS.md` contains repository-wide
  recovery and verification policy.

Retail instructions and call sites, accepted original-source oracles, and reviewed binary evidence
outrank decompiler guesses or a higher comparison score. Recover authored source, not compiler lowering.

## Documentation

Start with [AGENTS.md](AGENTS.md) for recovery rules and verification policy. The main architectural
references are:

- [Contributor workflow](docs/contributor-workflow.md)
- [Evidence and artifact policy](docs/evidence-policy.md)
- [Wizardry evidence and provenance model](docs/wiz8-evidence-model.md)
- [Wizardry executable target and runtime workflow](docs/targets/wiz8-executable.md)
- [Wizardry symbol evidence](docs/wiz8-symbol-evidence.md)

Task-specific procedures are indexed under [`.agents/skills`](.agents/skills/).
