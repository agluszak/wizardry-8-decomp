# Wiz8 executable target

The CMake graph now has separate matching and bring-up surfaces:

* `WIZ8_MATCHING` builds only recovered Wizardry-owned objects and the owned zlib adapters. It
  contains no synthetic implementations.
* `WIZ8_BRINGUP` links those same objects with a bring-up-only `WinMain`, producing `Wiz8.exe` and
  `Wiz8.pdb` for PE, PDB, and reccmp integration.
* `WIZ8` is the convenient build alias for `WIZ8_BRINGUP`; `just build WIZ8` builds the executable.
* `WIZ8_RUNTIME` is the runnable vertical-slice image. It adds the vendored SGP probe objects and
  links with `/OPT:REF`, so only source-backed COMDAT functions reached by recovered Wizardry code
  survive. `WIZ8_BRINGUP` retains `/OPT:NOREF` and remains the whole-image comparison surface.

Build and open the recovered main menu on the host display with:

```sh
just run
```

For an unattended run that cannot map a window or steal focus from the host desktop:

```sh
WIZ8_RUNTIME_DISPLAY=virtual just run
```

Bare `just build` builds that same `WIZ8_RUNTIME` graph; pass an explicit target only when building
a comparison or library surface such as `just build WIZ8` or `just build SREXT_JPEGIMPORTER`.

The launcher creates `build/runtime/wiz8` for the writable executable, video configuration, and
save directory. It links the large shipped assets from `$WIZ8_WORK_DIR/variants/gog-base` without
changing that canonical materialization. Wine state is reused at
`$WIZ8_WORK_DIR/wine/wiz8-runtime`; set `WIZ8_WINE_PREFIX` to override it. On the first run it
materializes the reviewed default `Wiz8.CFG` settings record, avoiding the still-incomplete
first-party settings-discovery path; subsequent in-game configuration changes remain local to the
staging directory.

The launcher uses a named 640x480 Wine desktop. Interactive `just run` inherits the host X display;
`WIZ8_RUNTIME_DISPLAY=virtual` creates a private 640x480x16 Xvfb server and points Wine at it for the
life of the run. The exact mode matters because SurRender rejects a virtual server whose geometry or
depth differs from `3DVideo.CFG`. `WIZ8_RUNTIME_DISPLAY=:5` selects an already-running display, and
`host` explicitly selects the inherited display. Virtual mode fails closed if Xvfb is unavailable,
so an unattended command cannot silently fall back to the desktop.

`just runtime-test` defaults to the private display; set `WIZ8_RUNTIME_DISPLAY=host` for visual
debugging. Off-screen Wine is configured to own its windows because Xvfb has no window manager.
Mouse and keyboard events still traverse released SGP input and the recovered region callbacks.
Exiting the launcher terminates only this dedicated Wine prefix. The launcher stays attached to the
game and returns its status instead of guessing its lifetime from Wine's desktop helper.

`just build <target> --jobs <count>` invokes the pinned 32-bit JOM 1.1.3 through the Python build
driver. It validates the checkout-local build directory and configures automatically when required;
CMake's generated dependency check handles later build-graph changes. `just prepare` separately
owns idempotent source/input preparation.

The recovered corpus is not link-complete. `WIZ8_BRINGUP` therefore uses `/FORCE:UNRESOLVED` rather
than contaminating matching source with invented globals or function bodies. LINK reports every
unresolved first-party boundary and emits an inspectable PE/PDB. The bring-up entry only returns
zero; it carries no address marker and is excluded from `WIZ8_MATCHING`.

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
The narrow `src/wiz8/imports/sr.def` imports only the canonical decorated name, while linker
diagnostics retain the fixed-arity C++ spelling and temporary C spelling as unresolved boundaries.
VC6 LINK has no `/alternatename` support, and generating fictitious DLL import names would make the
PE fail at load time. Those spellings will disappear as the artificial bootstrap units are restored
to their original declarations; until then `/FORCE:UNRESOLVED` records them without changing exact
matching bodies.

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
