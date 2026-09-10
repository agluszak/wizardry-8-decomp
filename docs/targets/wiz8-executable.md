# Wiz8 executable target

The CMake graph contains the three actual products:

* `WIZ8` links the recovered objects, producing `Wiz8.exe` and `Wiz8.pdb` for PE, PDB, and reccmp
  integration.
* `WIZ8_RUNTIME` is the runnable vertical-slice image, linked with `/OPT:REF`.
  All three products share the reconstructed `WIZ8_SGP` static archive from `src/sgp`.
  `WIZ8` retains `/OPT:NOREF` for comparison; there is no separate SGP runtime override layer.
* `WIZ8_RUNTIME_TEST` runs the semantic scenarios with the same optimized recovered objects. Its
  results are authoritative runtime evidence.

Build and open the recovered main menu from the retail install directory with:

```sh
just build runtime
just run
```

`just run` passes the released SGP `/WINDOW` option so a development run cannot take exclusive
control of the desktop. It seeds the two reviewed CFG files when absent and creates one managed
`Wiz8Runtime.exe` symlink in the retail directory; Wine otherwise searches beside the build-tree
EXE and substitutes a fake `SR.dll`. The launcher tees process output and, when the in-process
crash filter fires, feeds the record through `wiz8 analyze crash` and the link MAP before returning
the child status.

Use the retail executable as the runtime oracle in the same directory, with the same CFG files and
arguments:

```sh
just run-original
```

If the retail executable fails, correct the shared Wine environment before diagnosing the recovered
image. If retail succeeds and the recovered image fails, investigate the recovered code. Run that
same recovered image under Wine's debugger with:

```sh
just debug
```

`just debug` clears stale wineserver state, feeds `cont`, `bt`, and `quit` to `winedbg` by default,
and accepts a replacement script through `DEBUG_SCRIPT`. It uses no GDB proxy, MAP parsing, port
allocation, or automatic subsystem-specific breakpoints.

For behavioral recovery and visual acceptance, use the
[runtime-bringup skill](../../.agents/skills/runtime-bringup/SKILL.md). A semantic scenario result
only establishes its observed behavior; it is not independent proof of visible presentation.

Bare `just build` builds the `WIZ8` comparison image; `just build runtime` builds the runnable image.
Pass an explicit target only for a library surface such as `just build SREXT_JPEGIMPORTER`.

The direct launcher uses `$WIZ8_WORK_DIR/variants/gog-base` as its working directory, so the game
sees the extracted retail `Data`, `Dll`, `Levels`, Miles, Bink, and SurRender files without a mirror
of that installation under `build/`. It refuses to replace an unmanaged file at the executable-link
path.

`just runtime-test` defaults to the private display and judges only `WIZ8_RUNTIME_TEST`; set
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

`just build <target> --jobs <count>` invokes the pinned 32-bit JOM 1.1.3 through the Python build
driver. It validates the checkout-local build directory and configures automatically when required;
CMake's generated dependency check handles later build-graph changes. `just wiz8 prepare` separately
owns idempotent source/input preparation.

The comparison image remains intentionally link-incomplete and uses `/FORCE:UNRESOLVED`. Removing it
from either `/OPT:REF` runtime product currently exposes 603 unresolved externals, so both retain it
until source recovery narrows that live boundary. Both runnable products link the shared exception
filter; it identifies image-header read/write/execute faults as forced-unresolved calls and names
the register that consumed the return address when the PE "MZ" stub ran. Python then MAP-symbolizes
the recorded candidates and correlates the owner with `wiz8 analyze unresolved`. This is retained
debt, not a claim that a forced executable is generally safe.

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
`src/wiz8/imports/sr.def` retains the canonical provider name. Build preparation generates an
import-data-only COFF member whose fixed-arity IAT symbol points to that variadic PE hint/name.
The final PE imports the actual provider export; there is no call wrapper or unresolved assertion
alias. The ordinary build checks this mapping and the retail-backed MSS/Bink/JPEG/ZIP import sets.

## reccmp

`reccmp-project.yml` defines `WIZ8` against the canonical unpacked executable hash. Configure-time
detection records the original `Wiz8.exe`, while generated `reccmp-build.yml` points at the new PE
and PDB:

```sh
just compare 0x0044e010
```

Selected comparison builds current inputs itself; `WIZ8` is not an address selector.
Use the [comparison reference](../../.agents/skills/matching-decomp/references/comparison.md)
when COFF contributions, folding, or relocation targets require a different modality.

`src/wiz8/vc6_runtime.cpp` marks the twelve currently reviewed CRT/linker identities with
`// LIBRARY: WIZ8 0x...`. These annotations let reccmp account for library-owned bodies without
claiming them as first-party recovery; `--nolib` can exclude them from a report.
