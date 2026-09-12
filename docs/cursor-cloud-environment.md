# Cursor Cloud Agent environment

This document is for agents running in the prebuilt **Cursor Cloud Agent** environment only. It is
deliberately kept out of `AGENTS.md` so the normal contributor guidance stays host-agnostic. A
local checkout does not use any of this — it follows the [README](../README.md) (`.env` plus
`config/local-inputs.yml`).

The cloud VM already has every external tool, the pinned toolchain image, and the licensed input at
fixed absolute paths. Do not re-download, rebuild, or search for these; use them directly.

## Machine configuration

The environment `start` script recreates the gitignored machine config on every boot, so `.env` and
`config/local-inputs.yml` are already present in the checkout. `.env` pins:

| Variable | Value |
| --- | --- |
| `GHIDRA_INSTALL_DIR` | `/home/ubuntu/ghidra/ghidra_12.1.2_PUBLIC` |
| `WIZ8_INPUT_DIR` | `/home/ubuntu/wiz8-inputs` |
| `WIZ8_WORK_DIR` | `/home/ubuntu/wiz8-work` |
| `JAVA_HOME` | `/usr/lib/jvm/java-21-openjdk-amd64` |
| `DOCKER_BUILDKIT` | `0` (nested Docker needs the legacy builder) |

## Where things live

- **Python / CLI**: `uv` is at `~/.local/bin` (make sure it is on `PATH`). Sync with
  `uv sync --frozen`; run everything as `uv run wiz8 …`.
- **Licensed retail input**: `$WIZ8_INPUT_DIR/setup_wizardry_8_2001_12_23_(22306).exe` (the
  `gog-media` role; SHA-256 `48e31728…20a68d`). The materialized canonical matching target is
  `$WIZ8_WORK_DIR/variants/gog-base` (`Wiz8.exe`, `sr.dll`, `Dll/`); pinned public source
  dependencies (zlib, IJG JPEG, Info-ZIP) are under `$WIZ8_WORK_DIR/fid/sources/unpacked`.
  Regenerate with `uv run wiz8 prepare`.
- **VC6 matching compiler**: Docker image `wizardry8-msvc600:sp5` (Debian trixie, clang/LLVM 19).
  The Docker daemon is started per boot by `start` with the `fuse-overlayfs` storage driver; build
  images with the legacy builder (`DOCKER_BUILDKIT=0`). Rebuild with
  `uv run wiz8 toolchain build vc6-sp5`.
- **Ghidra**: `12.1.2 PUBLIC` at `$GHIDRA_INSTALL_DIR`, driven through PyGhidra by the CLI (for
  example `uv run wiz8 ghidra import`). The JDK is at `$JAVA_HOME`.
- **Host tooling on `PATH`**: `7z`, `innoextract`, `cabextract`, `unshield`, `git-lfs`,
  `llvm-undname`, and host `wine` (used by reccmp's cvdump and by `just run`).

## Quick verification

- `uv run wiz8 doctor` validates every path and tool above.
- `uv run wiz8 build WIZ8` then the reccmp linked-image comparison exercise the VC6 matching path
  end to end against retail `Wiz8.exe`.
