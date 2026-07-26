# `Wiz8.exe` startup and shutdown spine

`config/analysis/wiz8/startup-spine.csv` is the address-backed map from PE entry to the main loop
and back out. Each node records its caller, its evidence, and — importantly — whether it is
**library** code or **first-party** code.

## Library code is named in Ghidra, then linked — never modelled

Three nodes on the path are MSVCRT: `__WinMainCRTStartup` at `0x00401000`, its SEH handler at
`0x004011C0`, and the two `_initterm` calls at `0x00401164`. These are **not recovery targets**. A
runtime target links the real CRT and the real Win32 import libraries and the linker supplies them,
exactly as `imperialism-decomp` links `gdi32 user32 winmm vfw32 …` rather than reimplementing them.

Not being a recovery target is not an excuse to leave them as `FUN_`. They are recorded in
`config/analysis/functions/wiz8-vc6-runtime.csv` and applied by
`just ghidra apply-functions … --map config/analysis/functions/wiz8-vc6-runtime.csv`, so the
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
* **`srAssertFail`.** Recovered source deliberately declares a fixed-arity `int` form because VC6
  only emits the canonical bodies that way, but the real export is
  `?srAssertFail@@YAXPBD0J0ZZ` — variadic, `long` line. The declaration therefore mangled to a symbol
  the import library did not contain, in both a C++ and a C form. `src/wiz8/imports/sr.def` now
  aliases both onto the real export, so it links without touching a single recovered body.

`MSS32` (Miles) and `BINKW32` (Bink) are imported by the canonical executable but no recovered source
references them yet, so they are deliberately not linked until something needs them.

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

## `BringUpEngine`

`0x00401570` is the whole engine bring-up, and its first act is telling: `atexit(0x004017F0)`
registers the shutdown handler *before* anything is created, so teardown is armed even if a later
gate fails. It then registers the window class from `"Wizardry8"` / `"Wizardry8key"`, points the
module loader at the `"DLL"` subdirectory the shipped tree uses for plug-ins, and runs a chain of
bool-returning gates, each of which aborts `WinMain` on failure:

```text
0x004018C0  0x00404B00  0x00404BA0  0x005B1740  0x004023A0
0x00401EA0  0x00421BB0(hInstance, showCmd, 0x004011E0)  0x00405E60  0x00402970
```

`0x00421BB0` receives `0x004011E0`, which is the window procedure. The gates themselves are
enumerated but not individually named.

The shutdown handler at `0x004017F0` is guarded by a once-flag at `0x00650DB5`, clears the run flag
at `0x006F0628`, and tears down through `0x00408850` and `0x004E34B0`.

## A caveat about the shared stub

`0x005B1740` is `mov al, 1; ret`. It appears 17 times in the frame dispatch table **and** as one of
`BringUpEngine`'s gates. That is almost certainly identical-COMDAT folding rather than one function
used eighteen ways: the linker merges byte-identical bodies, so a single address can stand for many
distinct source functions that all just return true.

The practical consequence is that the 17 stub slots should not be read as "seventeen slots share one
handler". They are seventeen slots whose handlers were each trivial enough to fold. The bring-up
target links with `/OPT:NOICF` precisely so recovered code does not inherit this ambiguity.

## Unresolved boundaries

Recorded explicitly rather than guessed:
* The per-frame tick's table at `0x00647BD4` is now enumerated in
  `config/analysis/wiz8/frame-dispatch-table.csv`: a flat 62-entry function-pointer table indexed by
  state id, of which 17 slots hold one shared default stub at `0x005B1740` (`mov al,1; ret`, i.e.
  "handled") and 45 are real handlers. Joining it against `translation-unit-intervals.csv` attributes
  five handlers, and all five land in `Local Screens\*.cpp` — `MainGameScreen.cpp` (three),
  `MainMenuScreen.cpp` and `ReviewCharacterScreen.cpp` — which is what identifies this as the
  screen/state table rather than a generic callback array. The other 40 handlers fall outside the
  current interval coverage and are recorded unattributed.
* No node on the spine is attributable to an original translation unit yet. The assertion map covers
  606 functions and none of them is on this path, which is consistent with startup code that
  asserts little.
