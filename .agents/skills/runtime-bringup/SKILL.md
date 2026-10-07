---
name: runtime-bringup
description: Validate Wizardry retail/recomp behavior with the existing runtime harness and debugger; use runtime evidence only when the recovery batch affects observable behavior.
---

# Runtime validation

The game is past initial bring-up. Use this skill to validate or debug behavior, not as a routine gate
for source-mismatch campaigns.

| Need | Command |
| --- | --- |
| Recomp | `uv run wiz8 run` |
| Retail behavioral oracle | `uv run wiz8 run --original` |
| Recomp debugger | `uv run wiz8 debug` |
| Deterministic semantic scenarios | `uv run wiz8 runtime-test` |

Use the existing staging, Wine/GE-Proton setup, exception filter, debugger integration and scenario
registry. Do not build another runtime/trace harness.

## Behavioral recovery

1. Establish the requested observation in retail, then compare recomp under the same setup.
2. Drive real product paths; never add test-only branches or screen-specific bypasses to recovered code.
3. At a divergence, recover the nearest wrong/missing source, ABI, ownership or
   framework boundary. Use [matching-decomp](../matching-decomp/SKILL.md) and
   [type-modeling](../type-modeling/SKILL.md) rather than hard-coding around the symptom.
4. Validate the actual observable behavior with the smallest existing scenario/debug run.

Runtime behavior is authoritative for behavioral questions, but a scenario only establishes what it
observes. A passing scenario does not prove unrelated rendering, input or source equivalence.

## Testing discipline

Do not run runtime scenarios after ordinary mismatch edits. In a large recovery campaign, wait until a
substantial batch is complete and run only scenarios whose behavior the batch plausibly affects.

Use `--build` only when the relevant runtime product is missing/stale. Reuse successful scenario
results while relevant source/build/runtime inputs agree.

Both runnable products require complete native links; unresolved functions are linker errors.
Never add fake implementations or generated traps to make a runnable product link.

Low-level harness mechanics should be investigated from the owning runtime/tooling code only when the
task is specifically about the harness itself; ordinary recovery does not need to preload them.
