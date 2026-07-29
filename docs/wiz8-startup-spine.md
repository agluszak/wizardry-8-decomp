# `Wiz8.exe` startup and shutdown spine

`evidence/reviewed/wiz8/startup-spine.csv` is the address-backed map from PE entry to the main loop
and back out. Each node records its caller, its evidence, and — importantly — whether it is
**library** code or **first-party** code.

## Library code is named in Ghidra, then linked — never modelled

Three nodes on the path are MSVCRT: `__WinMainCRTStartup` at `0x00401000`, its SEH handler at
`0x004011C0`, and the two `_initterm` calls at `0x00401164`. These are **not recovery targets**. A
runtime target links the real CRT and the real Win32 import libraries and the linker supplies them,
exactly as `imperialism-decomp` links `gdi32 user32 winmm vfw32 …` rather than reimplementing them.

Not being a recovery target is not an excuse to leave them as `FUN_`. Analysis-only identities are
accepted in `evidence/reviewed/wiz8/claims.csv` and named in the canonical Ghidra project, so the
disassembly reads correctly:

| Address | Name | Provenance |
| --- | --- | --- |
| `0x0040115E` | `_XcptFilter` | `original-export` — six-byte thunk, MSVCRT import table names it |
| `0x00401164` | `_initterm` | `original-export` — same, IAT slot `0x005EB0FC` |
| `0x004011C0` | `_except_handler3` | `original-export` — same, IAT slot `0x005EB0DC` |
| `0x00401000` | `__WinMainCRTStartup` | `descriptive` — toolchain convention, not stated by the binary |

The three thunks are genuinely ABI-backed: the import table spells the names out, so reading them
off the IAT is evidence rather than inference. `__WinMainCRTStartup` is deliberately *not* claimed
that way — the entry point runs the unmistakable MSVC 6 GUI startup sequence, but the name is
convention, and it stays `descriptive` until a FID match against a pinned CRT snapshot corroborates
it.

The startup path alone needs:

| Library | Used for |
| --- | --- |
| MSVCRT | `__getmainargs`, `__setusermatherr`, `_acmdln`, `_initterm`, `exit`, `malloc`, `strtok`, `_strnicmp`, `sprintf`, `_spawnl` |
| KERNEL32 | `GetModuleHandleA`, `GetStartupInfoA`, `GlobalMemoryStatus` |
| USER32 | `FindWindowExA`, `SetForegroundWindow`, `ShowWindow`, `ShowCursor`, `PeekMessageA`, `GetMessageA`, `TranslateMessage`, `DispatchMessageA`, `WaitMessage`, `PostQuitMessage`, `MessageBoxA` |

`WIZ8_BRINGUP` already links `ddraw`, `gdi32`, `user32` and the generated `SR` import library, with
KERNEL32 and the `/MD` runtimes coming from the VC6 defaults. Two gaps were closed while mapping the
spine:

* **zlib.** The canonical executable statically links pristine zlib 1.0.4, so `inflateInit_`,
  `inflate` and `inflateEnd` were unresolved. `WIZ8_ZLIB_1_0_4` now builds the pinned source and the
  bring-up links it, instead of stubbing library code.
* **`srAssertFail`.** The real export is `?srAssertFail@@YAXPBD0J0ZZ` — variadic, `long` line —
  but the recovered declaration is deliberately **fixed-arity**, and that arity is itself recovered
  evidence. VC6 SP5 will not defer a pending inner-call stack cleanup across a call it believes is
  variadic, and the canonical bodies that pass a `String(...)` result as the assert message
  (`CharacterPointerToPartySlot`, `RPCPtrToPCSlot`, `MonsterInfoFromID`) fold exactly that cleanup
  across the assert call — under a variadic declaration those three bodies stop matching. So the
  original translation units saw a fixed-arity declaration while the original image imports the
  variadic name, which only an aliasing import library can produce; `src/wiz8/imports/sr.def`
  models that with one alias, `?srAssertFail@@YAXPBD0J0@Z = ?srAssertFail@@YAXPBD0J0ZZ`. The line
  is `long` to match the true ABI (only the arity is codegen-proven, not the width), and the dead
  undecorated C alias is gone now that every calling unit is C++. Known residual: our image
  imports the fixed-arity mangling where the original imports the variadic one; a `.def` cannot
  separate an import's symbol name from its name-table string, so closing that would need a
  hand-built long-form import object.

`MSS32` (Miles) and `BINKW32` (Bink) are imported by the canonical executable. The recovered Intro
Screen now reaches the `W8BinkVideo` owner and links the canonical Bink import surface. Its
`include/bink.h` declarations use the public Bink 1.5J SDK header retained by the JA2 source oracle
for names, types, callback shapes and the public handle prefix, while preserving the older
trackless `_BinkSetVolume@8` ABI proven by the Wizardry import table. Miles remains linked through
its canonical import surface as callers are recovered.

After those two fixes every remaining unresolved symbol in the bring-up link is **first-party** —
Wizardry globals, `PList` helpers and not-yet-recovered callees. No library symbol is missing.

## The spine

```text
0x00401000  __WinMainCRTStartup                     [library]
   ├─ _initterm 0x005FF430..0x005FF434              [library]  C initializers
   ├─ _initterm 0x005FF000..0x005FF42C              [library]  267 slots, 266 into .text
   │                                                           the initializers themselves are first-party
   └─ 0x00401670  WinMain
        ├─ FindWindowExA "Wizardry 8"        single-instance guard → return 0 if already running
        ├─ 0x00401950  ParseCommandLine      strtok / _strnicmp, e.g. "/NOSOUND"
        ├─ 0x00404BD0  QueryAvailableMemory  GlobalMemoryStatus, kilobytes to 0x006F0624
        ├─ 0x00427A60  GetVideoConfigFileName → "3DVideo.CFG"
        ├─ 0x00404BF0  FileExists           absent → _spawnl "3DSetup.EXE", recheck, else return 0
        ├─ 0x0042B830  CD presence check     "Please Insert the Wizardry 8 CD# 3."
        ├─ ShowCursor(0)
        ├─ 0x00401570  BringUpEngine        window class "Wizardry8" / "Wizardry8key" via 0x0040F020,
        │                                    then 0x00405740, 0x004018C0, 0x00404B00, 0x00404BA0,
        │                                    0x005B1740, 0x004023A0
        ├─ main loop   PeekMessageA → GetMessageA / TranslateMessage / DispatchMessageA
        │    └─ idle   active flag 0x006F0630 set → 0x004E3340 per-frame tick, else WaitMessage
        └─ shutdown    run flag 0x006F0628 clears → PostQuitMessage(0), return msg.wParam
```

## What the game refuses to start without

Two hard gates sit before any engine bring-up, and both matter for runtime work:

* `3DVideo.CFG` must exist, or the game launches `3DSetup.EXE` and re-checks once. This is why the
  shipped tree carries `3DSetup.exe` next to the executable.
* The CD check at `0x0042B830` must pass, or a message box ends the process.

## The C++ initializer table is a link-order witness

`_initterm` runs 267 slots at `0x005FF000..0x005FF42C`, 266 of which point into `.text`. They are
enumerated in `evidence/observations/wiz8/cpp-initializers.csv`.

The useful property is that **247 of the 265 consecutive pairs ascend in address**. The CRT builds
this table by concatenating each object's `.CRT$XC*` contribution in link order, so a table that is
93% monotonic in address says the linker laid `.text` out in essentially the same object order. That
makes the initializer table an independent witness of object link order, derived from completely
different evidence than the assertion-anchored translation-unit intervals.

Two things follow:

* The 18 descending steps are exactly the places where link order and address order disagree. They
  are recorded with their deltas and are the interesting anomalies for anyone reconciling link order
  — not noise to smooth over.
* Grouping the table into ascending runs with gaps under `0x4000` yields 49 runs. That grouping is a
  **heuristic**, not a claim that one run equals one translation unit: a unit may contribute several
  initializers and a run may span units.

Only 8 of the 266 fall inside current translation-unit interval coverage, resolving to
`Engine Code\OctPath.cpp`, `Engine Code\3d.cpp`, `Engine Code\Monster.cpp` and
`Dialog Code\AssayDialog.cpp` — four units with namespace-scope objects. The rest are recorded
unattributed.

## `BringUpEngine`

`0x00401570` is the whole engine bring-up, and its first act is telling: `atexit(0x004017F0)`
registers the shutdown handler *before* anything is created, so teardown is armed even if a later
gate fails. It then registers the window class from `"Wizardry8"` / `"Wizardry8key"`, points the
module loader at the `"DLL"` subdirectory the shipped tree uses for plug-ins, and runs a chain of
bool-returning gates, each of which aborts `WinMain` on failure:

Profiling each gate by the imports and literals inside its own extent identifies most of them, and
they line up with the subsystems the roadmap expects:

| Gate | Subsystem | Evidence |
| --- | --- | --- |
| `0x0040F020` | SGP DirectDraw / window class | references `C:\Projects\SGP\DirectDraw Calls.c` |
| `0x00405740` | module search path | rewrites `PATH` so plug-ins load from the `DLL` subdirectory |
| `0x004018C0` | SGP configuration | `"%s\sgp.ini"`, `PIXEL_DEPTH`, `GetPrivateProfileIntA` |
| `0x00404B00` | diagnostic formatting | `vsprintf` |
| `0x00401EA0` | input hook | `SetWindowsHookExA` bound to `GetCurrentThreadId` |
| `0x00421BB0` | renderer window, extensions | `srGERD::isWindowOpen`, `srExtension::load`, `ShowWindow` |
| `0x004023A0` | texture defaults | `srTextureMap::setupDefaultValues` |

The released SGP source resolves the retained parts of that chain under the single project profile
`/O2 /Ob2 /G5 /MD`: `GetRuntimeSettings` is `0x004018C0`, `InitializeInputManager` is
`0x00401EA0`, and `InitializeClockManager` is `0x00406BA0`. The input initializer installs the
exact source callbacks `KeyboardHandler` at `0x00401B30` and the Wizardry branch of `MouseHandler`
at `0x00401C70`. The clock initializer installs `Clock` at `0x00406B70`. Their object ranges
preserve the linked order from `sgp.c` startup fragments through `input.c` and the intervening SGP
managers to `timer.c`.

Only the two retained `sgp.c` functions and the eight retained `input.c` functions are configured
as evidence targets. The unretained remainder of those released units is outside this surface.

`0x00421BB0` receives `0x004011E0`, the window procedure, and its `srExtension::load` call is how
the `srEXT_*` plug-ins — including the JPEG importer this repository already recovered — enter the
process. `0x00404BA0`, `0x00405E60`, `0x00402970` and `0x005B1740` remain too small to characterise
from imports or literals alone.

The shutdown handler at `0x004017F0` is guarded by a once-flag at `0x00650DB5`, clears the run flag
at `0x006F0628`, and tears down through `0x00408850` and `0x004E34B0`.
The source matches identify lower-level shutdown edges reached by that product-specific handler:
`ShutdownInputManager` at `0x00401F70` and `ShutdownClockManager` at `0x00406BD0`.

## A caveat about the shared stub

`0x005B1740` is `mov al, 1; ret`. It appears 18 times in the complete frame dispatch table **and** as one of
`BringUpEngine`'s gates. That is almost certainly identical-COMDAT folding rather than one function
used nineteen ways: the linker merges byte-identical bodies, so a single address can stand for many
distinct source functions that all just return true.

The practical consequence is that the 18 stub slots should not be read as "eighteen slots share one
handler". They are eighteen slots whose handlers were each trivial enough to fold. The bring-up
target links with `/OPT:NOICF` precisely so recovered code does not inherit this ambiguity.

## Unresolved boundaries

Recorded explicitly rather than guessed:
* The per-frame tick's table begins at `0x00647BC8` and is enumerated in
  `evidence/observations/wiz8/frame-dispatch-table.csv`: thirteen five-pointer lifecycle records,
  of whose 65 slots 18 hold one shared default stub at `0x005B1740` (`mov al,1; ret`, i.e.
  "handled") and 47 are real-handler entries. The dispatcher references its tick column at
  `0x00647BD4`; treating that interior pointer as the start used to omit the first three fields and
  shift every state/role attribution. Joining the complete table against the generated
  `build/reports/translation-units/translation-unit-intervals.csv` attributes
  five handlers, and all five land in `Local Screens\*.cpp` — `MainGameScreen.cpp` (three),
  `MainMenuScreen.cpp` and `ReviewCharacterScreen.cpp` — which is what identifies this as the
  screen/state table rather than a generic callback array. The other 40 handlers fall outside the
  current interval coverage and are recorded unattributed.
* One node *is* attributable: `0x0040F020` references `C:\Projects\SGP\DirectDraw Calls.c`, the
  SGP translation unit whose thirteen functions are already source-matched. The rest are not, and
  the assertion map's 606 functions include none of this path — consistent with startup code that
  asserts little.

A profiling caveat worth repeating: inter-function padding in this executable contains the byte
pattern `FF 15 C8 B2 5E 00`, which disassembles as a call to BINKW32 `_BinkGetRects`. Any tool that
reads past a function's end will report that import spuriously. Bound profiling by the padding scan,
and treat a lone `_BinkGetRects` hit at a function's tail as an artifact rather than a real call.
