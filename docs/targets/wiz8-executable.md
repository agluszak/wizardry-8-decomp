# Wiz8 executable target

The CMake graph contains the three actual products:

* `WIZ8` links the recovered objects, producing `Wiz8.exe` and `Wiz8.pdb` for PE, PDB, and reccmp
  integration.
* `WIZ8_RUNTIME` is the runnable vertical-slice image. It adds the vendored SGP probe objects and
  links with `/OPT:REF`, so only source-backed COMDAT functions reached by recovered Wizardry code
  survive. `WIZ8` retains `/OPT:NOREF` and remains the whole-image comparison surface.
* `WIZ8_RUNTIME_TEST` runs the semantic scenarios with the same optimized recovered objects. Its
  results are authoritative runtime evidence.

Build and open the recovered main menu from the retail install directory with:

```sh
just build runtime
just run
```

`just run` does not build, invoke Python, configure the registry, or catch failures. It passes the
released SGP `/WINDOW` option so a development run cannot take exclusive control of the desktop. It seeds the two
reviewed CFG files when absent and creates one managed `Wiz8Runtime.exe` symlink in the retail
directory; Wine otherwise searches beside the build-tree EXE and substitutes a fake `SR.dll`.
It defaults `WINEDEBUG` to `-all`; set it explicitly to enable Wine tracing. A native crash is
reported directly by Wine.
Use the separate debugger only when debugging is intended:

```sh
just debug
```

The debugger is interactive: there is no wrapper timeout, default backtrace, or command-forwarding
syntax. Its small launcher owns the Wine process so quitting GDB does not leave the game running,
and it resolves the sound-startup breakpoint from `build/decomp/Wiz8Runtime.map`.

The runtime now reaches the main-menu state, but the recovered presentation remains visually
incomplete. Continue that recovery in this order:

1. Reproduce the missing intro or brown main-menu frame with `just run` on the host display and
   capture the first incorrect visible frame; Xvfb screenshots are not evidence because this
   DirectDraw path captures black output there.
2. For an intro failure, trace `BeginVideoPresentation` and the Bink frame copy into the target
   surface. For a main-menu failure, trace `DrawCatalogImage`, `EnsureCatalogFrameLoaded`, and the
   surrounding render transaction.
3. After correcting the first proven presentation defect, validate both the visible transition and
   the ordinary close/`WM_DESTROY` path so the launcher exits without leaving Wine processes behind.

Bare `just build` builds the `WIZ8` comparison image; `just build runtime` builds the runnable image.
Pass an explicit target only for a library surface such as `just build SREXT_JPEGIMPORTER`.

The direct launcher uses `$WIZ8_WORK_DIR/variants/gog-base` as its working directory, so the game
sees the extracted retail `Data`, `Dll`, `Levels`, Miles, Bink, and SurRender files without a mirror
of that installation under `build/`. It refuses to replace an unmanaged file at the executable-link
path.

`just runtime-test` defaults to the private display and judges only `WIZ8_RUNTIME_TEST`; set
`WIZ8_RUNTIME_DISPLAY=host` for visual debugging. Its controlled staging directory owns test config,
saves, display, and audio policy. The native exception handler records the actual crash and stack
candidates in-process; Python symbolizes recovered-image candidates against `Wiz8RuntimeTest.map`.
It never launches GDB or reruns a failed scenario. Off-screen Wine is configured to own its windows
because Xvfb has no window manager.
Mouse and keyboard events still traverse released SGP input and the recovered region callbacks.
Exiting the launcher terminates only this dedicated Wine prefix. The launcher stays attached to the
game and returns its status instead of guessing its lifetime from Wine's desktop helper.

`just build <target> --jobs <count>` invokes the pinned 32-bit JOM 1.1.3 through the Python build
driver. It validates the checkout-local build directory and configures automatically when required;
CMake's generated dependency check handles later build-graph changes. `just wiz8 prepare` separately
owns idempotent source/input preparation.

The comparison image remains intentionally link-incomplete and uses `/FORCE:UNRESOLVED`. Removing it
from either `/OPT:REF` runtime product currently exposes 603 unresolved externals, so both retain it
until source recovery narrows that live boundary. The runtime-test exception record identifies
image-base read/write/execute faults directly and MAP-symbolizes plausible stack values. This
is retained debt, not a claim that a forced executable is generally safe.

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
just build WIZ8
just compare WIZ8
```

`src/wiz8/vc6_runtime.cpp` marks the twelve currently reviewed CRT/linker identities with
`// LIBRARY: WIZ8 0x...`. These annotations let reccmp account for library-owned bodies without
claiming them as first-party recovery; `--nolib` can exclude them from a report.
