# The dynamic oracle

`wiz8 trace` runs the original under Wine with a debugger attached and reports
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
as `BringUpEngine` followed immediately by `ShutdownHandler`. SurRender ships
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
just wiz8 trace bring-up --seconds 90       # WinMain through the first frame
just wiz8 trace screens --seconds 180       # plus every dispatcher handler
just wiz8 trace bring-up --plan-only        # the breakpoints, no run
```

The plan is generated from `config/reccmp/wiz8-gameplay-boundaries.csv` and
`evidence/observations/wiz8/frame-dispatch-table.csv`, so it follows the ledger
rather than a hand-kept list. Results land in `build/reports/trace/`.

## What a run has already shown

The `bring-up` scenario reaches nine of its ten watched bodies in this order:

```
WinMain, ProcessCommandLine, QueryAvailableMemory, CheckCdPresent,
BringUpEngine, SetModuleSubdirectory, GetRuntimeSettings,
InitializeInputManager, VerifyDataSubdirs
```

which is exactly the order the recovered `WinMain` and `BringUpEngine` call
them in - an independent check of recovered control flow against the original
executing. `ShutdownHandler` is the one watched body not reached, which is the
run *not* failing. Two runs produce byte-identical streams, so a divergence
between builds would mean something.

## Pitfalls

- **Do not probe the proxy port by connecting to it.** `winedbg --gdb` accepts
  exactly one connection; a probe that connects consumes the one gdb needs, and
  the symptom is gdb timing out against a port that is demonstrably listening.
  `ss -ltn` observes without connecting.
- `winedbg` wants a Windows path for the executable. A bare `Wiz8.exe` fails
  with `Couldn't start process`.
- The game is a 32-bit PE with a fixed image base and no ASLR, so reviewed
  addresses are process addresses; no rebasing is needed anywhere.
- A `pkill -f` whose pattern appears in the invoking command line kills the
  invoker. Match on the process name instead.
