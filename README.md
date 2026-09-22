# Wizardry 8 decompilation

A matching decompilation of the original Windows release of *Wizardry 8* (2001).

The project reconstructs the game's C and C++ source from the retail executable and its supporting
libraries. Recovered code is compiled with the original-era Microsoft Visual C++ 6 toolchain and
compared against the shipped machine code. The longer-term goal is a faithful, understandable source
reconstruction of the game, not merely code that happens to behave similarly.

This is a work in progress. It is not yet a complete or independently playable replacement for the
original game.

The project is heavily inspired by the [LEGO Island decompilation](https://github.com/isledecomp/isle)
and builds on tooling developed by contributors to that project, most notably
[reccmp](https://github.com/isledecomp/reccmp). Its matching-oriented workflow and source annotations
owe a lot to the work done by the ISLE decompilation community.

## Current state

The repository already contains a substantial recovered codebase spanning the game executable, shared
SGP code, SurRender-facing code, UI, game systems, file formats, and runtime infrastructure.

There are three useful views of the reconstruction:

- **Recovered source** — ordinary C/C++ organized into reconstructed translation units and types.
- **Matching build** — a recompilation used to compare recovered functions directly with the retail
  executable.
- **Runnable build** — a partially recovered executable that uses the original game data and can be
  exercised under Wine.

The runnable build is intentionally incomplete. Unrecovered first-party calls trap instead of being
silently replaced with fake implementations.

For exact, current recovery and matching statistics, developers can run:

```sh
uv run wiz8 report status
```

## How the reconstruction works

The project combines several kinds of evidence rather than treating decompiler output as source code:

1. **Ghidra** is used to analyze the original executable, data structures, call graph, vtables, and
   machine code.
2. **Original and related source material** is used where available, most notably the released Standard
   Gaming Platform sources.
3. **Recovered C/C++** is written as plausible authored source rather than a transcription of compiler
   lowering.
4. **MSVC 6** recompiles the recovered code using the original ABI and compiler family.
5. **reccmp** compares the rebuilt code with the retail executable at function and data level.
6. **Runtime tests** exercise recovered behavior against the real game data.

Matching matters because it gives unusually strong feedback about recovered source, but byte identity
is not used as an excuse to write decompiler-shaped or compiler-shaped C++.

## Browsing the repository

- [`src/wiz8/`](src/wiz8/) — recovered Wizardry 8 executable source.
- [`include/wiz8/`](include/wiz8/) — recovered declarations, classes, structures, and ABI definitions.
- [`src/sgp/`](src/sgp/) — the reconstructed Standard Gaming Platform component, based on released
  source.
- [`src/surrender/`](src/surrender/) — recovered SurRender-side code used by the game.
- [`docs/`](docs/) — reverse-engineering notes, format documentation, library research, and project
  architecture.
- [`evidence/`](evidence/) — reviewed provenance and observations that do not belong directly in
  recovered source.

Some useful starting points are the
[Wizardry executable overview](docs/targets/wiz8-executable.md),
[source and class model](docs/wiz8-source-model.md), and
[data-format documentation](docs/wiz8-data-formats.md).

## Game files and licensing

This repository does **not** contain Wizardry 8 game files. Running or analyzing the canonical target
requires a legally obtained copy of the game; the development workflow currently uses the GOG release
as its primary input.

The Standard Gaming Platform source under `src/sgp` is distributed under Strategy First's
non-commercial SFI Source Code License Agreement. This project accepts those terms; reconstructed code
derived from that source is not offered under broader or commercial-use terms.

## Development

The current development workflow is built around `uv`, Docker, Wine, Ghidra, CMake, and reccmp.
`uv run wiz8` is the project command-line entry point.

Create local configuration:

```sh
cp .env.example .env
cp config/local-inputs.example.yml config/local-inputs.yml
```

Set the absolute paths in `.env`, including Ghidra, the local game-input directory, and a
checkout-specific `WIZ8_WORK_DIR`. Point `gog-media` in `config/local-inputs.yml` at the GOG
installer.

Bootstrap the environment and prepare the canonical game input:

```sh
uv sync --frozen
uv run wiz8 doctor
uv run wiz8 toolchain build vc6-sp5
uv run wiz8 prepare
```

To restore the reviewed Ghidra analysis for reverse-engineering work:

```sh
uv run wiz8 ghidra restore
uv run wiz8 doctor
```

Each checkout needs its own `WIZ8_WORK_DIR` and live Ghidra project.

Common development commands:

| Command | Purpose |
| --- | --- |
| `uv run wiz8 build` | Build the WIZ8 comparison executable. |
| `uv run wiz8 build SURRENDER` | Build the partial SurRender comparison DLL. |
| `uv run wiz8 compare 0xADDRESS... --build` | Rebuild and compare selected WIZ8 functions. |
| `uv run wiz8 compare 0x1003bee0 --program sr.dll --build` | Rebuild and compare selected SurRender provider functions. |
| `uv run wiz8 build runtime` | Build the runnable recovered executable. |
| `uv run wiz8 run` | Run the recovered executable under Wine. |
| `uv run wiz8 run --original` | Run the retail executable in the same staged environment. |
| `uv run wiz8 runtime-test --build` | Build and run the semantic runtime suite. |
| `uv run wiz8 ghidra decompile 0xADDRESS...` | Inspect the retail decompilation for selected functions. |
| `uv run wiz8 ghidra asm 0xADDRESS...` | Inspect annotated retail assembly. |
| `uv run wiz8 check` | Run the fast repository checks. |
| `uv run wiz8 lint` | Compile recovered C++ with the structural diagnostics lane. |
| `uv run wiz8 pr-check` | Run the checks required before opening or updating a pull request. |

Use `uv run wiz8 --help` for the complete command set.

More detailed developer documentation:

- [Contributor workflow](docs/contributor-workflow.md)
- [Evidence and artifact policy](docs/evidence-policy.md)
- [Wizardry evidence and provenance model](docs/wiz8-evidence-model.md)
- [Wizardry symbol evidence](docs/wiz8-symbol-evidence.md)
- [Runtime/build target](docs/targets/wiz8-executable.md)
