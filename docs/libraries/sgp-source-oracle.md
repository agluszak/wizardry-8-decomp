# Vendored SGP dependency

Wizardry 8 retains code from Sir-Tech's released Standard Gaming Platform source. The pinned tree
under `third_party/sfi-sgp` is an ordinary vendored dependency; its SFI-SCLA and upstream notices
must remain intact.

## Product boundary

The released sources refer to product headers that were not published. Their narrow recovered
surface lives under `include/wiz8/sgp-compat/` using the original include names. `builddefines.h`
is intentionally empty: released `Types.h` requires the file, but the retained Wizardry profile
does not consume a feature switch from it.

Released SGP assumes Windows case-insensitive lookup. Trivial capitalization adapters used only
by the Linux-hosted Clang lane live under `tools/lint/include`; they are not part of the product
compatibility surface.

The build never defines `WIZ8_PRECOMPILED_HEADERS`. The released translation units therefore use
their ordinary explicit include branches. There is no recovered precompiled header and no include
path shadowing by translation unit.

## Build model

`cmake/Sgp.cmake` owns three plain source lists:

- `WIZ8_SGP_WHOLE_SOURCES` contains whole retained units linked into the `/OPT:NOREF` comparison
  image and runtime products.
- `WIZ8_SGP_RUNTIME_PARTIAL_SOURCES` contains partially retained units made available only to the
  `/OPT:REF` runtime products.
- `WIZ8_SGP_ANALYSIS_ONLY_SOURCES` contains units built only by an explicit analysis request.

Source-local compiler adjustments are ordinary CMake source properties. They cover the Wizardry
`WinMain` spelling, two known symbol/definition differences, and the two missing include surfaces
that are not reached by a released unit's normal include list.

This split is required while the comparison image uses `/OPT:NOREF`: linking a partially retained
object there would introduce source functions absent from the original executable.

## Verification and optional archaeology

Normal verification builds the canonical GOG comparison image and runtime product. It does not
rebuild historical Wizardry variants or maintain a second SGP build graph.

Historical cross-build analysis remains available on demand:

```sh
uv run wiz8 sgp sweep
uv run wiz8 sgp sweep --unit random
```

The command builds the normal root-project SGP object targets and writes the untracked report at
`build/reports/sgp/harness.csv`. The checked historical snapshot and reviewed CSVs preserve the
investigation that established source identity and retention; they are provenance records, not
inputs to the product build.

Function names and operational types belong in Ghidra. Recovered C++ declarations and link order
belong in Git. New SGP investigation needs a concrete Wizardry call-chain, ABI, or matching driver.
