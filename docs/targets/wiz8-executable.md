# Wiz8 executable target

The CMake graph contains the three actual products:

* `WIZ8` links the recovered objects, producing `Wiz8.exe` and `Wiz8.pdb` for PE, PDB, and reccmp
  integration.
* `WIZ8_RUNTIME` is the runnable vertical-slice image, linked with `/OPT:REF`.
  All three products share the reconstructed `WIZ8_SGP` static archive from `src/sgp`.
  `WIZ8` retains `/OPT:NOREF` for comparison; there is no separate SGP runtime override layer.
* `WIZ8_RUNTIME_TEST` runs the semantic scenarios with the same optimized recovered objects. Its
  results are authoritative runtime evidence.

Build and open the recovered main menu from the staged retail tree with:

```sh
uv run wiz8 build runtime
uv run wiz8 run
```

`uv run wiz8 run` passes the released SGP `/WINDOW` option so a development run cannot take exclusive
control of the desktop. It assembles one writable tree under `build/runtime/wiz8` through the shared
`stage_game` primitive: asset directories link to the immutable `gog-base` variant, the reviewed CFG
files are copied in when absent, and the built `Wiz8Runtime.exe` is copied next to them so Wine
resolves the game's DLLs from staged retail data, not from the build tree. Process output is
forwarded; when the in-process crash filter fires, the same result carries the MAP-symbolized crash
report.

Use the retail executable as the runtime oracle in its own staged tree, with the same CFG files and
arguments:

```sh
uv run wiz8 run --original
```

If the retail executable fails, correct the shared Wine environment before diagnosing the recovered
image. If retail succeeds and the recovered image fails, investigate the recovered code. Run that
same recovered image under Wine's debugger with:

```sh
uv run wiz8 debug
```

`uv run wiz8 debug` stages the recomp under `build/runtime/debug`, clears stale wineserver state,
starts Wine's GDB proxy on a free port, and connects system GDB with a deterministic stop policy
(`SIGTRAP` stop/print, `SIGSEGV` pass, full backtrace, registers, shared libraries, code and stack).
The captured frames are symbolized through `Wiz8Runtime.map` and matched against the runtime-stub
manifest. The report and raw session are written under `build/debug/`.

For behavioral recovery and visual acceptance, use the
[runtime-bringup skill](../../.agents/skills/runtime-bringup/SKILL.md). A semantic scenario result
only establishes its observed behavior; it is not independent proof of visible presentation.

Bare `uv run wiz8 build` builds the `WIZ8` comparison image; `uv run wiz8 build runtime` builds the runnable image.
Pass an explicit target only for a library surface such as `uv run wiz8 build SREXT_JPEGIMPORTER`.

The runnable stage under `build/runtime/wiz8` is the working directory, so the game sees the
extracted retail `Data`, `Dll`, `Levels`, Miles, Bink, and SurRender files through managed links
while every writable path stays under `build/`. The prepared `gog-base` variant is never modified.
`run --original` stages the retail executable the same way under `build/runtime/original`.

`uv run wiz8 runtime-test` defaults to the private display and judges only `WIZ8_RUNTIME_TEST`; set
`WIZ8_RUNTIME_DISPLAY=host` for visual debugging. Its controlled staging directory owns test config,
saves, display, and audio policy. The native exception filter records every general-purpose register
in-process and scans registers as well as stack words for recovered-image candidates; Python
symbolizes them against `Wiz8RuntimeTest.map` and reports the unresolved externals of each owning
object. It never launches GDB or reruns a failed scenario. Off-screen Wine is configured to own its
windows because Xvfb has no window manager.
Mouse and keyboard events traverse reconstructed SGP input and the recovered region callbacks.
Exiting the runtime-test harness terminates only its dedicated Wine prefix. The harness stays
attached to the game and returns its status instead of guessing its lifetime from Wine's desktop
helper.

`uv run wiz8 build <target> --jobs <count>` drives the pinned VC6 container through the Python build
driver. It configures when `build/decomp/CMakeCache.txt` is absent and lets CMake's generated
dependency check handle later build-graph changes. `uv run wiz8 prepare` separately owns idempotent
primary source/input preparation; optional corpus variants stay explicit.

The comparison image remains intentionally link-incomplete and uses `/FORCE:UNRESOLVED`. The two
runnable products do not: before their link, the build driver derives the unresolved first-party
symbols from the completed comparison link and emits one trap thunk per symbol plus a COFF alias
object, so every unrecovered call enters a debugger-friendly trap with the caller's stack intact.
Both runnable products link the shared exception filter; when a genuine runtime fault occurs it
records the register file and candidate image addresses, which Python symbolizes through the
runtime MAP and correlates with the unresolved externals of each owning object.

## Platform and import libraries

`include/wiz8/wiz8_windows.h` is the common VC6 Windows boundary. It selects the DirectX 7 declaration
surface and includes the toolchain's real `<windows.h>` and `<ddraw.h>` rather than local type
facsimiles. CMake links `ddraw.lib`, `gdi32.lib`, and `user32.lib`; KERNEL32 and the dynamic
MSVCRT/MSVCP60 runtimes come from the VC6 defaults and `/MD`. This follows Imperialism's rule of
linking the import libraries actually used by the product, but not its product-specific
winmm/vfw/DirectSound/DirectPlay set: canonical `Wiz8.exe` does not import those DLLs.

The current source calls `srAssertFail`, whose actual Wiz8 import is the variadic C++ ABI
`?srAssertFail@@YAXPBD0J0ZZ`. `include/wiz8/sr_api.h` owns the fixed-arity call declaration proven by
the exact recovered bodies. For these four-argument calls, its 32-bit cdecl ABI is compatible with
the DLL export; changing the declaration itself to variadic changes otherwise-exact VC6 output.
`src/wiz8/imports/sr.def` retains the canonical provider name. The product build generates an
import-data-only COFF member whose fixed-arity IAT symbol points to that variadic PE hint/name.
The final PE imports the actual provider export; there is no call wrapper or unresolved assertion
alias. The link resolves each consumer against the `.def`-generated import library.

## reccmp

`reccmp-project.yml` defines `WIZ8` against the canonical unpacked executable hash. Configure-time
detection records the original `Wiz8.exe`, while generated `reccmp-build.yml` points at the new PE
and PDB:

```sh
uv run wiz8 compare 0x0044e010
```

Selected comparison builds current inputs itself; `WIZ8` is not an address selector.
Use the [comparison reference](../../.agents/skills/matching-decomp/references/comparison.md)
when COFF contributions, folding, or relocation targets require a different modality.

`src/wiz8/vc6_runtime.cpp` marks the twelve currently reviewed CRT/linker identities with
`// LIBRARY: WIZ8 0x...`. These annotations let reccmp account for library-owned bodies without
claiming them as first-party recovery; `--nolib` can exclude them from a report.
