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
| Deterministic semantic scenarios (builds itself) | `uv run wiz8 runtime-test` |

Launchers stage one writable tree under `build/runtime/<product>` from the immutable `gog-base`
variant, seed the reviewed CFG files, and pass `/WINDOW`; do not reinvent Wine staging or launch
commands. They launch an already-built product: use `uv run wiz8 build runtime` when that product is
missing or stale. `run --original` stages retail; `run` and `debug` stage the recomp; `runtime-test`
stages and builds its own semantic-test image. `debug` drives Wine's GDB proxy with a deterministic
stop policy and symbolizes the captured frames; `WIZ8_DEBUG_GDB` supplies extra GDB commands. For
visual harness debugging use `WIZ8_RUNTIME_DISPLAY=host uv run wiz8 runtime-test`.

Both runnable products install the same in-process exception filter. The filter records every
general-purpose register, scans registers as well as stack words for image addresses, and
`uv run wiz8 run`/`runtime-test` symbolize the record through the product MAP, including the
unresolved externals of the caller's object. Do not chase a bare Wine backtrace by hand.

`Wiz8Runtime.exe` and `Wiz8RuntimeTest.exe` link without `/FORCE:UNRESOLVED`. Unrecovered calls enter
a build-generated `// STUB:` trap that prints
`WIZ8_RUNTIME_STUB address=... symbol=... name=...` and breaks before touching the caller's stack.
`uv run wiz8 build runtime`/`runtime-test` regenerate the stub set automatically; recovering a retail body
removes its stub on the next build. If stubgen reports an unresolved identity at an already recovered
address, fix the declaration/linkage/signature; never add a handwritten fake body.

## Recover the failing behavior

1. Establish the requested transition/observation with `uv run wiz8 run --original`, then compare `uv run wiz8 run`
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
