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

`--save` is mandatory for `load`: the fixture is staged as the sandbox's only quicksave
(existing `Saves/Quick*.SAV` are cleared and it is copied in as `Quick 1.SAV`), so `/LOAD` can
only select the known file. Trace a rebuilt image with `--executable` plus `--link-map`; the plan
keeps its reviewed retail names and resolves each build's addresses through that build's MAP,
whose link timestamp must match the executable's PE header - a MAP from another build fails
before any run. When a point's decorated name is absent (marker-emitted functions have no
declaration, and the linker may keep another unit's instantiation), a unique map symbol with the
same canonical name resolves it instead. Points that still resolve to nothing are listed as
`unwatched` in the result rather than silently dropped.

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

The verdict fails closed. `affirmative` is true only when every requirement in `requirements`
holds: all three runs started under the debugger (`all_started`), all reached the terminal event
(`all_reached_terminal`), the two retail runs agree through the terminal (`retail_repeatable`),
no watched point failed to resolve in any run (`no_unwatched_points`), and the retail/rebuilt
streams agree through the terminal (`streams_agree`). Any hole - a run that never started, a load
that never completed, a function missing from the rebuilt image - leaves the comparison
inconclusive rather than green.

### State fingerprints at the terminal

Event streams say the same code ran; they do not say the world looks the same. Each scenario can
also declare a set of *state probes* - scalar words read through reviewed `// GLOBAL:` objects - and
the breakpoint at the terminal event prints them once as `STATE` lines. `load` fingerprints the
screen state (current and pending `W8ScreenStateRuntime` words) and the camera placement
(yaw/pitch/position) through `g_gd_camera`, read through a null check so an unset object
reports `missing` rather than faulting the inferior.

Two rules keep the fingerprint meaningful. It compares state, never addresses: pointer-bearing
fields legitimately differ between builds, so only scalar values are probed. And each build's probe
addresses come from its own evidence - retail resolves the reviewed `// GLOBAL:` markers, the
rebuilt image resolves the same globals through the source index's semantic ids into its own MAP.
A global the rebuilt image lacks is `unwatched`, which fails `no_unwatched_points` closed like any
other dropped point.

The verdict gains `state_repeatable` (the two retail runs fingerprint identically - this is what
proves the chosen fields are deterministic, and any field that later proves volatile fails here
first) and `state_agrees` (retail and rebuilt fingerprints identical, field by field; `diffs`
records each mismatching name with both values). `runs.<label>.state` keeps each raw fingerprint.

Every result identifies what produced it: executable name and sha256, launch arguments, fixture
name and sha256, linker-MAP sha256, the loaded provider (`sr.dll`) sha256, plan/evidence/repository
digests, tool versions and the timeout. A rebuilt executable traced under a stock provider says
nothing about a rebuilt provider; the provenance keeps the two claims separate.

## The smoke scenario: the product's own entry and exit

The runtime-test executable calls `WinMain`, invokes `SGPExit` itself and ends through
`TerminateProcess` - convenient, but not the same evidence as the runnable product's lifecycle.
`wiz8 analyze smoke` runs `Wiz8Runtime.exe` (the default; `--executable` and `--link-map`
apply as usual) through its real path: entry, the intro screen, the main menu, the quit path,
`SGPExit`, and a real process exit.

The run drives itself from breakpoints, not sleeps: when a watched handler fires, its breakpoint
commands run an input gesture through `xdotool` on the trace's own display. The intro screen's
frame handler gets a periodic `Escape` (every 30th hit, the same re-arm cadence the runtime
harness uses to dismiss intro videos); `screen_1_enter` queues `PageDown`+`Return` (the menu's
exit-screen binding); `screen_12_enter` queues the confirming `Return`. A gesture that never lands
leaves the run at the timeout and the verdict stays false - `process_exited` requires gdb to
finish inside the window, which only happens when the inferior really exited.

```sh
uv run wiz8 analyze smoke --seconds 120
```

The verdict is affirmative only when every requirement holds: the run started, `WinMain` ran, the
menu and the exit screen were each entered through their real transitions, `SGPExit` ran, and the
process exited on its own.

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
