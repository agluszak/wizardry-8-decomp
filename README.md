# Wizardry 8 decompilation

A matching decompilation of the original Windows release of *Wizardry 8* (2001).

The project reconstructs the game's C and C++ source from the retail executable and its supporting
libraries. Recovered code is compiled with the original-era Microsoft Visual C++ 6 toolchain and
compared against the shipped machine code. The longer-term goal is a faithful, understandable source
reconstruction of the game, not merely code that happens to behave similarly.

The recovered executable links and runs with the original game data and middleware. Semantic,
binary-structure and behavioral audits remain open; linking and function pairing do not establish
retail equivalence.

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
- **Runnable build** — an executable that uses the original game data and can be
  exercised under Wine.

Both runnable products require complete native linking. Unresolved functions fail at the linker;
the builds do not generate first-party trap implementations.

## How the reconstruction works

The project combines several kinds of evidence rather than treating decompiler output as source code:

1. **Ghidra** is used to analyze the original executable, data structures, call graph, vtables, and
   machine code.
2. **Original and related source material** is used where available, most notably the released Standard
   Gaming Platform sources.
3. **Recovered C/C++** is written as plausible authored source rather than a transcription of compiler
   lowering.
4. **MSVC 6** recompiles the recovered code using the original ABI and compiler family.
5. **reccmp** pairs rebuilt functions with retail ones; Ghidra decompiles both and Ghidriff diffs them,
   alongside data and vtable checks.
6. **Runtime tests** exercise recovered behavior against the real game data.

The comparison gives strong feedback about recovered source, but a clean diff is not used as an excuse
to write decompiler-shaped or compiler-shaped C++.

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

Use Java 25 and the Linux distribution built from our pinned Ghidra fork. CI and
local installation use the same fork-owned installer; no release-source patches
are applied. After setting `GHIDRA_INSTALL_DIR` to a new directory in `.env`:

```sh
set -a
source .env
set +a
revision=$(sed -n 's/^  GHIDRA_REVISION: //p' .github/workflows/ci.yml)
checksum=$(sed -n 's/^  GHIDRA_SHA256: //p' .github/workflows/ci.yml)
curl -fsSL "https://raw.githubusercontent.com/agluszak/ghidra/$revision/support/installForkDistribution.sh" -o /tmp/installForkDistribution.sh
bash /tmp/installForkDistribution.sh "$revision" "$checksum" "$GHIDRA_INSTALL_DIR"
```

Ghidra commands validate the pinned Ghidra fork and PyGhidra version when they start.

Bootstrap the environment and prepare the canonical game input:

```sh
uv sync --frozen
uv run wiz8 toolchain build vc6-sp5
uv run wiz8 prepare
```

To restore the reviewed Ghidra analysis for reverse-engineering work:

```sh
uv run wiz8 ghidra restore
```

Ghidra commands reject stale or untracked live analysis state directly. Each checkout needs its own `WIZ8_WORK_DIR` and live Ghidra project.

Common development commands:

| Command | Purpose |
| --- | --- |
| `uv run wiz8 build` | Build the WIZ8 comparison executable. |
| `uv run wiz8 build SURRENDER` | Build the partial SurRender comparison DLL. |
| `uv run wiz8 compare 0xADDRESS... --build` | Rebuild and compare selected WIZ8 functions. |
| `uv run wiz8 compare 0x1003bee0 --program sr.dll --build` | Rebuild and compare selected SurRender provider functions. |
| `uv run wiz8 build runtime` | Build the runnable recovered executable. |
| `uv run wiz8 run` | Run the recovered executable with GE-Proton. |
| `uv run wiz8 run --original` | Run the retail executable in the same staged environment. |
| `uv run wiz8 runtime-test --build` | Build and run the semantic runtime suite. |
| `uv run wiz8 ghidra decompile 0xADDRESS...` | Inspect the retail decompilation for selected functions. |
| `uv run wiz8 ghidra asm 0xADDRESS...` | Inspect annotated retail assembly. |
| `uv run wiz8 check` | Run the fast repository checks. |
| `uv run wiz8 lint` | Compile recovered C++ with the structural diagnostics lane. |

Use `uv run wiz8 --help` for the complete command set. CI runs `wiz8 check` and `wiz8 lint` as separate gates; run those directly when needed locally.

`wiz8 check` enforces clang-format 21.1.8 on reconstructed C/C++, runtime tests and local
analysis headers. `wiz8decomp.build.cpp_format_files` defines the shared ownership list;
imported SDK headers, vendored source and released source oracles are excluded. Normalize
the complete owned tree with the same list:

```sh
uv run python -c 'from pathlib import Path; import subprocess; from wiz8decomp.build import cpp_format_files; subprocess.run(["clang-format", "-i", *cpp_format_files(Path.cwd())], check=True)'
```

Keep retail diagnostic line constants explicit where evidenced, as SGP's `ATTEMPT_AT`
does. Formatting must not depend on historical whitespace or decompiler-shaped source.

`uv run wiz8 prepare` downloads and verifies the pinned umu-launcher and GE-Proton runtime under
`WIZ8_WORK_DIR/runtime-toolchain`. umu's Steam Runtime and cache are kept there as well instead of
using the user's Steam/XDG directories. The default renderer is Glide2x at 800×600. CI
uses the same prepared umu + GE-Proton runtime and Glide2x configuration as local development.
`PROTONPATH` or `WIZ8_UMU_RUN` are explicit developer overrides of the prepared tools.

More detailed developer documentation:

- [Repository policy and task workflows](AGENTS.md)
- [Wizardry evidence and provenance model](docs/wiz8-evidence-model.md)
- [Runtime/build target](docs/targets/wiz8-executable.md)
