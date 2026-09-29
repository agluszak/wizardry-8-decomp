# Mismatch campaigns

Use this reference to turn a saved mismatch population into shared recovery work. The classifications
below are descriptive. They do not prove equivalence, compiler lowering, or a source bug.

## Calls

For call-heavy differences, compare canonical direct-call identities rather than rendered names.
Classify factual deltas:

- identical canonical call sequence;
- one different canonical callee;
- retail-only or recomp-only call;
- same callees reordered;
- unresolved/unpaired target;
- helper call on one side and apparent expansion on the other;
- same callee with receiver/argument differences.

Cluster by the actual call delta. A repeated `retail X -> recomp Y` across many callers is one
declaration/identity/class-model investigation, not many caller fixes.

## Fields, widths and ABI

Repeated offsets or access widths are high-value signals. Trace the receiver and all producers/consumers
before editing the declaration.

Check:

- field offset and owning record;
- byte/word/dword width and extension/truncation;
- signed/unsigned branch behavior;
- argument count and calling convention;
- receiver presence/adjustment;
- return production and caller consumption.

Fix the canonical owner and its full consumer family. Do not repair caller sites with casts or local
aliases.

## Globals, literals and referenced data

A different global identity, string, integer or floating constant is a logical difference until retail
evidence explains it. Resolve duplicate globals and interior aliases at the global owner. Exhaust
referenced-data and literal/address-only queues before spending time on noisy structural diffs.

Relocated addresses and paired-object addresses may be presentation differences, but only the existing
catalog/reference machinery may establish that correspondence.

## Classes, templates and lifetime

Audit constructor/destructor/copy/assignment/vtable/template/exception-frame differences by owner.
Exception-frame changes can reflect real lifetime differences; do not dismiss them as compiler noise
before checking construction, cleanup and exceptional exits.

Compiler emissions stay compiler emissions. Follow source-fidelity and type-modeling rather than
creating handwritten deleting destructors, FOLDED aliases or explicit template specializations to
match emitted code.

## Floating point

Treat float/double storage, promotion, x87 rounding, accumulation precision, threshold constants,
NaN handling and expression grouping as one dedicated family. VC6/x87 output is sensitive to apparently
small source-type differences.

Do not add broad floating-point normalization to Ghidriff.

## Control and statement structure

This is the residual bucket, not the first place to work. Before analyzing it, attach factual context:

- canonical call sequence;
- paired referenced globals/data;
- literal/data findings;
- ABI status;
- class/template owner;
- floating-point/lifecycle signals.

Then distinguish predicate/operator changes, extra/missing guards, branch inversion, loop bounds,
loop form, statement/store ordering, helper expansion and genuinely large structural changes.

One reviewed equivalent extra guard does not justify a generic rule that extra guards are equivalent.

## Representation-only candidates

Generated temporary names, local declarations, commutative operand spelling and known Ghidra
restructuring can be presentation. Record them so they leave the active source-recovery queue, but do
not change source merely to make the text identical.

When no source-level discrepancy remains established, keep faithful source and leave the comparison
outcome as evidence rather than manufacturing a clean result.
