---
name: runtime-bringup
description: Recover or debug Wizardry runtime behavior, UI flows, input, persistence, loading, or startup using the original game and deterministic runtime harness.
---

# Runtime bring-up

## Run the existing products

| Need | Command |
| --- | --- |
| Recomp | `just run` |
| Retail behavioral oracle | `just run-original` |
| Recomp under Wine debugger | `just debug` |
| Deterministic semantic scenarios (builds itself) | `just runtime-test` |

Launchers use the retail directory/CFG files and `/WINDOW`; do not reinvent Wine staging or launch
commands. They launch an already-built product: use `just build runtime` when that product is missing
or stale. Currently even `run-original` checks that `Wiz8Runtime.exe` exists for the shared setup.
`debug` uses native `winedbg` with `cont`, `bt`, `quit` by default; set `DEBUG_SCRIPT` for chosen debugger
commands. For visual harness debugging use `WIZ8_RUNTIME_DISPLAY=host just runtime-test`.

Both runnable products install the same in-process exception filter. An unresolved first-party call
jumps to the PE header, where the "MZ" stub corrupts the frame pointer and pops the return address
into EDX before the fault. The filter records every general-purpose register, scans registers as well
as stack words for image addresses, and `just run`/`runtime-test` symbolize the record through the
product MAP, including the unresolved externals of the caller's object. Do not chase Wine's one-frame
`+0x7` MZ backtrace by hand.

## Recover the failing behavior

1. Establish the requested transition/observation with `just run-original`, then compare `just run`
   under the shared setup. If retail also fails, investigate the environment before blaming recovery.
2. Drive real product pathways: input queue/hooks, screen dispatch, callbacks, persistence, resource
   loading. The harness may inject input/events externally; matching source must never gain test-only
   branches. Menu/new-game scenarios use the real keyboard/input path, not direct downstream handlers.
3. At failure, recover the nearest missing/wrong framework boundary instead of hard-coding a
   screen-specific bypass. For source changes/comparison use [matching-decomp](../matching-decomp/SKILL.md);
   inspect unanswered binary facts with its [PyGhidra reference](../matching-decomp/references/pyghidra.md).
4. Validate the relevant behavior with the existing deterministic harness and the requested observable
   result. Prefer stable semantic observations over coordinates, arbitrary sleeps, incidental call
   counts, or internal helper order. Reuse successful results until relevant inputs change.

Runtime behavior is authoritative for behavioral questions. A build, live process, or semantic harness
result alone does not prove visible rendering/input: observe the requested presentation and transition.
Private-display DirectDraw captures can be black; use host-visible evidence for visual acceptance.
Report only what was observed and stop at the requested flow; unrelated screens do not expand the task.

`runtime-test` is for relevant behavior, not mandatory for every recovered function. Extend its existing
observations only when needed; do not create a second harness/reporting framework unless the existing
one fundamentally cannot observe the required behavior. Product architecture and environment details:
[Wiz8 executable](../../../docs/targets/wiz8-executable.md).
