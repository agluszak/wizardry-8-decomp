---
name: runtime-bringup
description: Recover or debug Wizardry runtime behavior, UI flows, input, persistence, loading, or startup using the original game and deterministic runtime harness.
---

# Runtime bring-up

## Run the existing products

| Need | Command |
| --- | --- |
| Recomp | `uv run wiz8 run` |
| Retail behavioral oracle | `uv run wiz8 run --original` |
| Recomp under Wine debugger | `uv run wiz8 debug` |
| Deterministic semantic scenarios | `uv run wiz8 runtime-test` |

Launchers stage one writable tree under `build/runtime/<product>` from the immutable `gog-base`
variant, seed the reviewed CFG files, and pass `/WINDOW`; do not reinvent Wine staging or launch
commands. They launch an already-built product: pass `--build` to `debug` or `runtime-test` when that
product is missing or stale, or use `uv run wiz8 build runtime`/`runtime-test` directly. `run
--original` stages retail; `run` and `debug` stage the recomp; `runtime-test` stages its semantic-test
image. `debug` drives Wine's GDB proxy with a deterministic
stop policy and symbolizes the captured frames. For visual harness debugging use
`WIZ8_RUNTIME_DISPLAY=host uv run wiz8 runtime-test`.

The native `--list-scenarios` registry owns each scenario's lifecycle, tier, semantic kind, deadline
and validator. `new-game-ui` is the UI acceptance spine: create, name, commit, start, and move.
`main-game-start` uses a game-thread-created single-character party for fast gameplay checks.
Use `--tier main --check-order` to check order leakage and `--repeat 3 --scenario NAME` to probe
intermittency in separate writable stages. Do not infer stability from one passing run. When scenario
input depends on UI geometry, derive it from live regions/controls instead of hard-coded coordinates.

For original-binary reachability/order questions use the existing
[dynamic oracle](../../../docs/dynamic-oracle.md); its observations are scenario-bounded and never prove
unreachability. Do not create another trace harness when the existing one answers the question.

Both runnable products install the same in-process exception filter. The filter records every
general-purpose register, scans registers as well as stack words for image addresses, and
`uv run wiz8 run`/`runtime-test` symbolize the record through the product MAP, including the
unresolved externals of the caller's object. Do not chase a bare Wine backtrace by hand.

`Wiz8Runtime.exe` and `Wiz8RuntimeTest.exe` link without `/FORCE:UNRESOLVED`. Unrecovered calls enter
a build-generated `// STUB:` trap that prints
`WIZ8_RUNTIME_STUB address=... symbol=... name=...` and breaks before touching the caller's stack.
`uv run wiz8 build runtime`/`runtime-test` regenerate the stub set automatically; recovering a retail
body removes its stub on the next build. Stub generation requires reviewed retail-address evidence:
an addressless unresolved first-party callable is a build error, because an identity-free trap would
hide an invented or stale declaration. If stubgen maps an unresolved spelling to an already recovered
address, or reports no retail address evidence, fix the declaration/linkage/signature or attach the
reviewed retail address; never add a handwritten fake body. Unchanged stub files and staged executables
keep their bytes and timestamps; `staged.executable_written` says whether
the stage actually republished the image. `build runtime-test` reports `phases_ms` for
configure/regenerate/prereq compile/index/stubs/link.

## Recover the failing behavior

1. Establish the requested transition/observation with `uv run wiz8 run --original`, then compare the
   recomp under the shared setup. If retail also fails, investigate the environment first.
2. Drive real product pathways: input queue/hooks, screen dispatch, callbacks, persistence and resource
   loading. The harness may inject input/events externally; matching source must never gain test-only
   branches. Menu/new-game scenarios use the real input path, not downstream-handler bypasses.
3. At failure, recover the nearest missing/wrong framework boundary instead of hard-coding a
   screen-specific bypass. Mapped `STUB` traps name a retail address when identity is established;
   inspect that function with `uv run wiz8 ghidra decompile ADDRESS` / `ghidra asm ADDRESS`, search
   existing helpers, then recover a coherent body. Use [matching-decomp](../matching-decomp/SKILL.md)
   for source recovery, [ghidra-analysis](../ghidra-analysis/SKILL.md) for unanswered retail facts, and
   [type-modeling](../type-modeling/SKILL.md) when the failure exposes an ABI/layout defect.
4. Validate the relevant behavior with the existing deterministic harness and the requested observable
   result. Prefer stable semantic observations over coordinates, arbitrary sleeps, incidental call
   counts or internal helper order. Reuse successful results until relevant inputs change.

Runtime behavior is authoritative for behavioral questions. A build, live process, or semantic harness
result alone does not prove visible rendering/input: observe the requested presentation and transition.
Private-display DirectDraw captures can be black; use host-visible evidence for visual acceptance.
Report only what was observed and stop at the requested flow; unrelated screens do not expand the task.

## Testing the harness itself

- Batching is the default: consecutive `batch=yes` cases sharing a fixture run in one process; verify
  grouping in the stderr `RUN batch [...]` lines and that `runs` observations match singleton runs of
  the same set. `--isolate` gives every case a fresh process; `--workers 1` serializes jobs (default
  `--workers 2`, capped at the job count so a single-case run spawns one worker).
- Exe-level `--scenarios` rejects unknown names, `batch=no` members and mixed fixtures with usage
  error exit 64 before the game starts — no display needed to check this (`wine ./Wiz8RuntimeTest.exe
  --scenarios a,b` in a staged tree).
- To exercise the poisoned-batch re-run path without code edits, SIGKILL the game process mid-batch:
  `pkill -9 -f 'Wiz8RuntimeTes[t]'` (bracket avoids matching the pkill command itself). Batch processes
  live ~11-12s; the engine-ready wait (~8.5s) is the only wide kill window — poll stderr for the
  `RUN batch [...]` line then kill on a fixed timer, because echoed stderr can lag the process >1s and
  post-startup cases complete in tens of ms, making partial-report kills impractical.
- For a visible run use `WIZ8_RUNTIME_DISPLAY=host WIZ8_WINE_VIRTUAL_DESKTOP=1`: the game maps inside
  a 640x480 Wine desktop window on the desktop instead of resizing the real display — recordable.

`runtime-test` is for relevant behavior, not mandatory for every recovered function. Extend its
existing observations only when needed; do not create a second harness/reporting framework unless the
existing one fundamentally cannot observe the required behavior. Product architecture and environment
details: [Wiz8 executable](../../../docs/targets/wiz8-executable.md).
