# Standard Gaming Platform source oracle

Wizardry 8 contains code from Sir-Tech's shared Standard Gaming Platform (SGP). The released
Jagged Alliance 2 history preserves explicit Wizardry build branches, so this source is used
directly rather than independently reconstructed from the executable.

The pinned source, revision, licence, excluded binaries, and modification rules are recorded in
`third_party/sfi-sgp/README.md`. The Strategy First licence is non-commercial and imposes notice
requirements on modifications; contributors must preserve it verbatim.

## Evidence surfaces

The SGP workflow deliberately separates four roles:

- `config/sgp.yml` configures the settled compiler profile, source revision, builds, and units.
- `build/reports/sgp/harness.csv` is the normal generated cross-build report.
- `evidence/snapshots/sgp/harness.csv` preserves a reviewed report because reproducing it requires
  proprietary Wizardry executables. Its README documents the refresh procedure and input identity.
- `evidence/reviewed/sgp/findings.csv` contains conclusions that require human review, while
  `evidence/observations/sgp/source-paths.csv` contains binary path observations.

Accepted analysis-only identities and their provenance live in
`evidence/reviewed/wiz8/claims.csv`. Once a retained body has an owned address-marked declaration,
that source spelling supersedes the claim value. Multiple SGP and CFAgent observations for one
address remain separate claims.

## Wizardry branches and overlays

The active Wizardry source census is generated from the vendored tree by looking for live
`WIZ8_PRECOMPILED_HEADERS` branches. `MemMan.c` contains only a commented-out branch. Tests derive
this census instead of maintaining a filename list in YAML or Markdown.

The missing product PCH cannot be reconstructed by renaming the JA2 PCH: the JA2 header pulls in
game-specific screens and utilities which are not SGP ABI. Each probed unit therefore gets a narrow
overlay under `config/sgp-overlays/`, containing only the includes and declarations its released
source proves necessary.

The established release model is:

- `WIZ8_PRECOMPILED_HEADERS` is defined.
- `JA2`, `JA2_PRECOMPILED_HEADERS`, `UTIL`, and `UTILS` are absent.
- `_DEBUG` is absent, so `MemAlloc` and `MemFree` reduce to CRT allocation.
- Missing product declarations are supplied per unit rather than through a speculative global PCH.

This is a translation-unit model, not a claim that the complete `WIZ8 SGP ALL.H`, `WizLibs.h`, or
other product headers have been recovered.

## Compiler profile and harness

Run all configured units, or select one, with:

```sh
uv run wiz8 sgp sweep
uv run wiz8 sgp sweep --unit random
```

The harness compiles every unit exactly once with the settled VC6 SP5 project profile
`/O2 /Ob2 /G5 /MD` recorded in `config/sgp.yml`. Sparse matches are not allowed to invent historical
per-file flags. The released VC6 SGP project supports this treatment because its release options are
project-level and the inspected units have no release per-file override.

COFF relocation fields are masked before comparison. Candidates are classified as exact,
relocation-equivalent, near source with Wizardry modifications, absent or stripped, or ambiguous
generic. A protected or packed executable is recorded as unavailable rather than mislabeled absent.

Normal sweeps only update the gitignored report. After reviewing a complete run, refresh the tracked
snapshot explicitly:

```sh
uv run wiz8 sgp sweep --update-snapshot
```

A partial-unit sweep cannot replace the snapshot.

## Recovery lessons

### Random.c and project flags

`Random.c` is a complete recovered unit. Its own header proves that `PRERANDOM_GENERATOR` is gated
behind `JA2`, which the Wizardry branch does not define.

The unit demonstrates why the common profile matters. `/G5` affects the arithmetic bodies, and
`/Ob2` allows `Random` to inline into `Chance`. A weaker profile can make part of the unit appear
exact while making the rest look modified or absent. That is not evidence for per-file options; it
is evidence that the tested project profile is incomplete.

The released identity `Random` supersedes the CFAgent descriptive name `GetRandomNumber`. The
CFAgent name remains an alias because it is useful external semantic evidence, but source-backed
identity has higher authority.

### Compression.c and relocation ambiguity

Wizardry retains the decompression half of `Compression.c`. `CompressFini` and `DecompressFini`
have identical bodies after relocation masking, so the generic matcher correctly leaves them
ambiguous. The retained call target resolves the body as `DecompressFini`, and surrounding source
order supports the conclusion that the compression COMDATs were eliminated rather than moved.

This is why generated similarity rows and reviewed conclusions are separate datasets.

### FileMan.c and product layout changes

Most retained FileMan functions compile directly from the released source, but
`FileCheckEndOfFile` exposes a stable Wizardry extension: released code indexes
`LibraryHeaderStruct` with its upstream stride while Wizardry uses the larger runtime structure.
The nested file-open structure retains its released stride, isolating the difference to the library
header rather than the surrounding algorithm.

Patch-specific behavior is recorded as a per-build observation, not promoted to the canonical base
executable identity.

### Container.c is not PList

The released container unit provides stack, list, queue, and ordered-list families. Wizardry retains
a source-ordered subset, including folded aliases whose identical implementations are reached by
both stack and list callers. Caller behavior and source order resolve short-body collisions that a
byte matcher cannot name safely on its own.

This SGP container API is unrelated to the first-party `3D Code/PList.cpp` family used by monster
and gameplay code. Similar purpose does not establish shared source ownership.

### Debug and exception support boundaries

`DEBUG.C` is a C translation unit: the original Windows project treats its uppercase suffix as
case-insensitive `.c`, so the host CMake model explicitly preserves that language choice. Wizardry
retains only `String`, the variadic formatter backed by an eight-slot array of 512-byte buffers.
Its unique body occurs immediately before the retained `FileMan.c` range and is called from 55
sites. The full topic logger and assertion-failure path are absent; tiny release helpers remain
generic and are not promoted from byte similarity alone.

`ExceptionHandling.cpp` is an intentional compiled-empty boundary. Outside `JA2`, its public header
does not define `ENABLE_EXCEPTION_HANDLING`, so all implementation functions are excluded before
compilation. Wizardry still uses the ordinary VC6 `_except_handler3` and `_XcptFilter` CRT boundary,
but it neither owns the SGP crash-report implementation nor imports `_CxxThrowException`. Together
with the reviewed allocator model, this preserves the executable's explicit null-return allocation
paths instead of introducing modern throwing-allocation semantics.

### Startup core input and timer

The configured `sgp.c` evidence surface contains only the two complete retained functions:
`GetRuntimeSettings` and `ProcessCommandLine`.

The configured `input.c` surface is likewise restricted to the eight retained hook callbacks and
queue functions. `timer.c` is complete; source order between exact neighboring functions resolves
the otherwise-generic six-byte `GetClock` body. The complete timer object is linked. The broader
core and input objects remain evidence-only because linking them under `/OPT:NOREF` would retain
unneeded released APIs.

### Retention and linking policy

The canonical whole/partial/empty decision for every configured unit lives in
`evidence/reviewed/sgp/units.csv`. `WIZ8_PRECOMPILED_HEADERS` proves only that a released source file
has a Wizardry branch; it does not prove that every emitted COMDAT survived the original link.

Only whole retained units are direct inputs to the current `/OPT:NOREF` bring-up. The JSON-compatible
`config/sgp.yml` is read by both CMake and the evidence sweep: each unit owns its source, overlay,
PCH/language settings, compile definitions, build groups and optional reviewed-function selection.
CMake then groups their objects by use: `WIZ8_SGP_RETAINED` for complete units,
`WIZ8_SGP_RUNTIME_SHARED` for partial units used by the `/OPT:REF` runtime,
`WIZ8_SGP_PROBE_ONLY` for evidence-only units, and the separately compiled runtime `sgp.c` whose
discarded WinMain is renamed. `WIZ8_SGP_PROBES` builds the first three groups independently of the
runnable image. Per-source overlays and PCH choices remain properties of the source record rather
than being encoded as a separate recursive build target for every file.

`Random.c` now supplies the original `Random` implementation directly. The former hand-written
`GetRandomNumber` duplicate has been removed; that CFAgent description remains only an identity
alias in reviewed evidence.

### LibraryDataBase.c and DbMan.c

These units are compiled from the vendored source directly. Their disk structures remain
source-compatible, while Wizardry extends the runtime library header with mapping state. A source
wrapper found only inside another function is retained as a rejected interior match rather than
promoted to a function start.

## Reviewed analysis model

Accepted SGP identities, source-derived structures, and prototypes live directly in the canonical
Ghidra project. The evidence ledger preserves their source-oracle provenance. Near, ambiguous,
stripped, and interior matches remain observations rather than promoted identities.
