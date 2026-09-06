# Mismatch patterns

Use this reference for an unexplained focused-comparison divergence. Compare operands, widths,
flag-producing instructions, control flow, memory effects, and calls before proposing a source
correction. A mismatch or a higher score does not by itself establish which C++ was authored.

| Signal | Evidence to check before changing source |
| --- | --- |
| Signed versus unsigned branches | Trace compared values, promotions, widths, and flags; correct a type only when that evidence supports it. |
| Different operation or return width | Check producers, truncation/extension, and caller use; one `neg al` does not establish the return type. |
| Changed call receiver or memory operand | Trace the canonical owner, field, layout, and ABI. |
| Changed calls, stores, initialization, or cleanup | Check ordering dependencies, aliasing, object lifetime, and exceptional exits; fix demonstrated differences in effects. |
| Repeated global loads versus a retained local | Check whether intervening calls or writes can change the value; preserve proven snapshot or reload semantics. |

## Preserve source-level control flow

A guarded bottom-tested loop or machine countdown does not prove an authored `if` plus `do`/`while`.
Keep a counted `for` when it expresses the recovered operation. Preserve the count's width,
signedness, narrowing, and evaluation frequency; those may be behavioral facts even when loop shape
is not. Do not add an index and a redundant remaining-iteration counter just to reproduce registers.
Use `while` or `do` when the operation's semantics or available original source warrants it, not as
a spelling experiment.

Likewise, register roles and stack slots do not prove a missing source block. A tail `jmp` versus
`call`/`ret` does not by itself justify rewriting a return or adding a result local. Investigate
supported ABI, lifetime, or call-site differences; do not manufacture scopes or expression variants.
Unsigned `x != 0` and `x > 0` are equivalent for an ordinary integer value: interpret the flags and
operands rather than changing between those spellings to chase `JE` versus `JBE`.

## Stop without inventing certainty

Test one supported source fact at a time. Revert demonstrated semantic or ABI regressions; a worse
score, reordered independent stores, or changed stack slots alone does not establish one.
When no evidence-backed source correction remains, retain straightforward C++ and report the
unresolved mismatch. Do not call it codegen-only, relocation noise, or a classifier error without
supporting evidence. This stopping rule does not turn a mismatch into an exact/effective result.
