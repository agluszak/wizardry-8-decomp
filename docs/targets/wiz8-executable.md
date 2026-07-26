# Wiz8 executable target

The CMake graph now has separate matching and bring-up surfaces:

* `WIZ8_MATCHING` builds only recovered Wizardry-owned objects and the owned zlib adapters. It
  contains no synthetic implementations.
* `WIZ8_BRINGUP` links those same objects with a bring-up-only `WinMain`, producing `Wiz8.exe` and
  `Wiz8.pdb` for PE, PDB, and reccmp integration.
* `WIZ8` is the convenient build alias for `WIZ8_BRINGUP`; `just build WIZ8` builds the executable.

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
