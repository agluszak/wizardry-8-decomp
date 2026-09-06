# Mismatch patterns

Use this reference after focused comparison identifies a structural divergence. Compare complete
instructions, operands, encoded size, control flow, and memory effects. Raw percentages and
classifier failures are diagnostic. A register difference is harmless only when the bodies otherwise
line up instruction for instruction.

| Signal | Likely source fact | Focused experiment |
| --- | --- | --- |
| `JB`/`JBE` versus `JL`/`JLE` | unsigned versus signed value | correct the declaration or field type |
| `JE` versus `JBE` on unsigned data | truthiness versus explicit `> 0` | spell out the comparison |
| `neg eax` versus `neg al` | byte-sized return | narrow the declaration |
| changed call receiver or memory operand | wrong owner, field, or type | trace the retail receiver and declaration |
| register roles permuted with changed flow | wrong block or local lifetime | fix control flow before registers |
| initialization in the wrong order | declaration or initializer order differs | reorder locals or initializers |
| `dec`/`je` ladder versus comparisons | small `switch` | test the ordinary switch form |
| merged cleanup versus separate cleanups | duplicated compiler tail was written literally | write one source-level tail |
| repeated global reloads | original retained a local | load once and reuse |
| tail `jmp` versus `call`/`ret` | return shape enabled a tail call | test a named local result |

These are hypotheses, not recipes. Test one source fact at a time when practical. A worse structured
status, earlier divergence, or changed machine effect falsifies the hypothesis. Reordered independent
stores or stack slots alone do not. Record a rejected hypothesis only when it changes a shared model,
prevents repeated work, or explains an unresolved blocker.
