# The dynamic oracle

`wiz8 analyze trace` runs the original under Wine with a debugger attached and reports
which reviewed bodies executed, in what order. It answers the questions the
image at rest cannot: which gates actually run, which screen handler the
dispatcher reaches, whether a body is reached at all in a given scenario.

Every claim it makes is bounded by the scenario that produced it. A stream says
what happened in *this* run to *this* point; it never says a function is
unreachable, only that this scenario did not reach it.

## Setting up a sandbox

The variant trees under `$WIZ8_WORK_DIR/variants` are hardlinked from the
canonical inputs, and Wizardry writes its configuration back into its own
directory. Running from a variant tree would therefore modify the canonical
input through the shared inode. The sandbox is a real copy for that reason.

```sh
export WIZ8_DYNAMIC_DIR="$WIZ8_WORK_DIR/dynamic"
mkdir -p "$WIZ8_DYNAMIC_DIR"
cp -a "$WIZ8_WORK_DIR/variants/gog-base" "$WIZ8_DYNAMIC_DIR/game"
WINEPREFIX="$WIZ8_DYNAMIC_DIR/prefix" wine wineboot -u
```

Two pieces of configuration decide whether the game gets past bring-up, and the
recovered `WinMain` says why. It refuses to start without a video
configuration - `if (!FileExists(GetVideoConfigFileName()))` spawns
`3DSetup.EXE`, which needs MFC and is not present - so the shipped
configuration has to be in place before the first run:

```sh
cp "$WIZ8_WORK_DIR/variants/gog-base/__support/app/"{3DVideo.CFG,Wiz8.CFG} \
   "$WIZ8_DYNAMIC_DIR/game/"
```

The shipped `3DVideo.CFG` selects `Glide2x`, which under a headless X server
fails with *Could not open video output device* - the trace shows the failure
as `InitializeStandardGamingPlatform` followed immediately by `ShutdownHandler`. SurRender ships
`srDD_Software.dll` beside its hardware drivers, and selecting it is what makes
the boot path complete:

```
Software
800
600
16
Miles Fast 2D Positional Audio
```

(The file is CRLF-terminated. The first line is the SurRender device, the next
three are width, height and depth.)

Finally the display. The game asks for the mode named in that file, and a mode
change fails on a virtual X server whose screen is something else, so the
server is created at the same geometry:

```sh
Xvfb :99 -screen 0 800x600x16 -nolisten tcp &
export WIZ8_DYNAMIC_DISPLAY=:99
```

Nothing here touches the desktop: the game renders into the virtual server.

## Running

```sh
uv run wiz8 analyze trace bring-up --seconds 90       # WinMain through the first frame
uv run wiz8 analyze trace screens --seconds 180       # plus every dispatcher handler
uv run wiz8 analyze trace bring-up --plan-only        # the breakpoints, no run
```

The bring-up selection is generated from compiler-bound startup translation units; screen targets
still come from `frame-dispatch-table.csv`. Each run allocates its own
proxy port and records the executable, variant, plan, reviewed-evidence and repository identities,
tool versions and timeout. Results land in `build/reports/trace/`.

## The load scenario and the retail/rebuilt differential

`load` launches the executable with `/LOAD`, which makes the product load the newest quicksave
during startup instead of reaching the menu - a deterministic gameplay entry that needs no input
injection. Its plan is the startup gates plus the dispatcher screens plus every recovered function
in `LoadSaveGame.cpp`.

Stage a shared fixture save with `--save` (it is copied into the sandbox's `Saves/` before launch),
and trace a rebuilt image with `--executable` plus `--link-map`; the plan keeps its reviewed retail
names and resolves each build's addresses through that build's MAP. Points the rebuilt image does
not carry are listed as `unwatched` in the result rather than silently dropped.

```sh
uv run wiz8 analyze trace load --seconds 100 --save "path/to/Quick 1.SAV"
```

`wiz8 analyze differential` runs the scenario three times in one sandbox - retail twice, then the
rebuilt image - and reports retail's own repeatability before the retail/rebuilt comparison:

```sh
uv run wiz8 analyze differential load --seconds 100 --save "path/to/Quick 1.SAV"
```

Each scenario declares a terminal event at which its semantic claim ends (`load` ends at the first
`ProcessMainGameAutoSave`: the save is loaded and the world is live). The bounded verdict compares
streams only through that event; afterwards the game sits in its steady frame loop and how many
frames a run captured before the timeout is capture noise, not behavior. The raw full-window
comparison stays in the report so the tail is visible, and `post_terminal_events` records which
event names each run reached after the claim's end. A run that never reaches the terminal event
compares in full, so a load that fails to complete still diverges.

Every result identifies what produced it: executable name and sha256, launch arguments, fixture
name and sha256, the loaded provider (`sr.dll`) sha256, plan/evidence/repository digests, tool
versions and the timeout. A rebuilt executable traced under a stock provider says nothing about a
rebuilt provider; the provenance keeps the two claims separate.

## Earlier bring-up observation

Before the plan moved to the complete reviewed startup spine, the smaller ten-point scenario
reached nine watched bodies in this order:

```
WinMain, ProcessCommandLine, QueryAvailableMemory, CheckCdPresent,
InitializeStandardGamingPlatform, SetModuleSubdirectory, GetRuntimeSettings,
InitializeInputManager, VerifyDataSubdirs
```

That remains an independent observation of the recovered control flow, but its count is not a
current-plan expectation. Rerun the generated plan before comparing current event counts.

## Pitfalls

- **Do not probe the proxy port by connecting to it.** `winedbg --gdb` accepts
  exactly one connection; a probe that connects consumes the one gdb needs, and
  the symptom is gdb timing out against a port that is demonstrably listening.
  `ss -ltn` observes without connecting.
- `winedbg` wants a Windows path for the executable. A bare `Wiz8.exe` fails
  with `Couldn't start process`.
- The game is a 32-bit PE with a fixed image base and no ASLR, so reviewed
  addresses are process addresses; no rebasing is needed anywhere.
- Every trace runs in a new Unix process group. Cleanup signals only that group and shuts down the
  dedicated `WINEPREFIX` with `wineserver -k`; it never uses a global `pkill`.
