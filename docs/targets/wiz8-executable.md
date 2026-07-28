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

Build and open the recovered main menu with:

```sh
just run
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

The launcher uses one named 640x480 Wine virtual desktop in that same prefix. Retail opens a
desktop-sized popup and then asks SurRender to switch the physical display to 640x480. Modern
compositors often preserve the host mode instead, leaving the fifth texture tile stretched across
the native desktop and making mouse coordinates disagree with the 640x480 region catalog. The
named desktop preserves retail's logical display without allocating a new X server or Wine prefix
on every run. Mouse motion and button events still enter through released SGP input, are converted
to client coordinates, and traverse the recovered region catalog and callbacks; arrow, Home, End,
and Enter keys use the same menu selection and activation path. Exiting the launcher terminates only
this dedicated Wine prefix. The launcher runs the game as its foreground child, so `just run` stays
attached to the menu and returns the game's status instead of guessing its lifetime from Wine's
desktop helper.

`just build <target> [jobs]` invokes the pinned 32-bit JOM 1.1.3 directly. Configuration, source
fetching, and original-image detection happen only when `build/decomp/CMakeCache.txt` is absent (or
when `just configure` is requested); CMake's generated dependency check handles later CMake changes.
Thus a no-op build enters the existing graph without regenerating all execution inputs first.

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
