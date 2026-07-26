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

Wiring those into a runnable target belongs to `wiz8-ls5.4`, which separates byte-matching from
runtime bring-up; this document only records what that target will have to link.

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

## Unresolved boundaries

Recorded explicitly rather than guessed:

* `BringUpEngine`'s six callees are on the path but not individually identified.
* The per-frame tick dispatches through an indexed table at `0x00647BD4`; its contents are not
  enumerated, so the screen/state machine behind the main loop is still opaque.
* No node on the spine is attributable to an original translation unit yet. The assertion map covers
  606 functions and none of them is on this path, which is consistent with startup code that
  asserts little.
