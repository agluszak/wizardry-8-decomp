---
name: matching-decomp
description: Recover Wizardry 8 function bodies, declarations, and layouts against the pinned VC6 target. Use when porting a function, interpreting an exact/effective mismatch, proving field or return types, or testing a source-level model against Wiz8.exe.
---

# Matching decompilation for Wiz8.exe

The pinned VC6 target is a **falsifier**. Static analysis and C++ reasoning propose a source model;
the emitted instruction sequence decides whether that model survives. A clean-looking constructor,
container abstraction, field type, or control-flow rewrite is still wrong when VC6 moves farther
from the retail body.

## Supported recovery loop

1. Start with the joined context report:

   ```sh
   just context 0x<address>
   ```

   It combines Ghidra decompilation, source ownership, provenance, callers/callees, strings and
   assertions, fields, cross-build mappings, and current match state. Use the Ghidra UI/API for
   ordinary listing, type, namespace, xref, and function-boundary work. There is no generic
   `just ghidra query` command.

2. Confirm the canonical owner before declaring anything. Search by address, symbol, class, and
   aliases. Extend the existing declaration and translation unit; do not add duplicate externs,
   raw vtable calls, guessed wrappers, or a second inventory.

3. Recover ordinary C++ under `src/wiz8/`. Update `src/wiz8/sources.cmake` when adding or moving a
   translation unit. Source markers and declarations are authoritative; do not maintain a parallel
   boundary CSV.

4. Build with the pinned compiler and compare the smallest useful batch:

   ```sh
   just build WIZ8
   just compare 0x<address>...
   # or:
   just compare --file src/wiz8/<unit>.cpp
   ```

   `just compare` builds by default. Use `--no-build` only when the current checkout's product was
   already rebuilt after the last source change.

5. Run the required repository lane after the focused result is understood:

   ```sh
   just test
   just lint        # class declarations or inheritance
   just check       # source inventory, markers, evidence, Python, or docs
   ```

6. Record accepted evidence and compiler-falsified experiments in the Bead. Do not copy live progress
   totals into Markdown or add a second machine-readable evidence store.

## Read the structured verdict, not the percentage

Focused comparison classifies each selected function as:

- `exact`: instruction-identical after the comparison's supported normalization;
- `effective`: the remaining difference is proved semantically harmless, such as a residual register
  permutation;
- `mismatch`: a real machine-state divergence remains;
- `inconclusive`: the analysis reached a limit and has not proved a source defect.

Use the first-divergence section of `just compare` for structured mismatch evidence. Its categories point at the evidence-bearing
part of the model: field/global selection, stored value, callee identity, receiver or argument,
branch condition, branch target, return value, or preserved state.

The raw linked-image percentage is diagnostic, not a choice function. Relocated operands and scratch
register choices can lower it even when a selected body is exact or effective. Never select between
two source candidates because one has the larger raw percentage. Compare instruction sequence,
operands, control flow, and the structured status.

A register difference is harmless only after the bodies line up instruction for instruction. When
instruction count, order, tests, calls, or memory operands differ, the register permutation is a
symptom of the wrong source shape rather than an excuse.

## Falsification discipline

Run a focused comparison immediately after any change that can alter the source model:

- field width, order, offset, signedness, or constness;
- parameter or return type, especially byte-sized values;
- calling convention, virtual dispatch, or inheritance;
- constructor, destructor, allocation, deletion, or ownership shape;
- container representation or template instantiation;
- loop, branch, early-return, reload, or local-lifetime structure.

A worse exact/effective status, an earlier first divergence, or a newly changed memory/call operand
falsifies the experiment unless the diff proves a stale build, wrong checkout, or unrelated pairing
problem. Do not reason past the compiler because the model looked more idiomatic.

Revert code that the experiment proved worse, but preserve the negative result. A useful Bead note
contains:

```text
symbol/address:
hypothesis:
source shape tried:
comparison command:
before:
after / first divergence:
conclusion:
```

“This shape is not the original's” is reusable evidence. A silent revert throws that evidence away;
leaving the known-regressed code in the recovered source is equally wrong.

## Build trust

Each checkout owns `build/decomp`, and the tooling rejects a build directory configured by another
checkout. Trust comparison output only when it comes from the same checkout and after the relevant
source change was rebuilt.

If output looks implausibly stale, run the supported build/compare command again rather than invoking
reccmp manually against arbitrary artifacts. Preserve the separate `/OPT:NOREF` matching image and
`/OPT:REF` runtime image; do not “fix” comparison by changing the product mode.

## Recover normal C++ first

Use the ordinary circa-2000/2001 C++ model as the default hypothesis:

- recover a constructor-shaped routine as a constructor;
- recover a destructor-shaped routine as a destructor;
- model vector/list/string-like operations with the corresponding ordinary container semantics;
- recover typed template operations rather than hand-written special-case wrappers;
- use typed construction and deletion so VC6 emits compiler-owned lifecycle machinery.

Special calling conventions, explicit scalar-deleting destructor methods, custom alloc/free helpers,
manual vtable calls, and novel one-off containers require positive evidence.

### Deleting destructors

Never hand-write a scalar-deleting destructor. A decompiler rendering such as a vtable call with a
delete flag is the lowering of typed `delete object`. Give the class an ordinary virtual destructor,
write typed `delete`, and let VC6 emit or inline the deleting wrapper. Treat the complete destructor,
deleting destructor, adjustor thunks, constructors, and vtable installation as one ABI bundle.

### Duplicated compiler tails

Ghidra shows emitted code, including tails duplicated by VC6. Writing both copies literally can make
the compiler merge cleanups that the original kept separate. When a duplicated epilogue or call tail
looks compiler-generated, write the source-level tail once and test whether VC6 reproduces the
copies.

### Inline control

Do not add `__forceinline`, `__declspec(noinline)`, or inline pragmas to improve one isolated score.
First recover declaration ownership, header visibility, source order, class shape, and callers. An
inline-control annotation needs call-site evidence and must improve the complete callee/caller/thunk
bundle without regressing an exact boundary.

## Reading a mismatch

Compare full instructions, including operands and encoded size, not mnemonics alone. Common signals:

| Signal | Likely source fact | Experiment |
| --- | --- | --- |
| `JB`/`JBE` versus `JL`/`JLE` | unsigned versus signed value | correct the declaration or field type |
| `JE` versus `JBE` on an unsigned value | truthiness versus explicit `> 0` | spell out the comparison |
| `neg eax` versus `neg al` | return value is byte-sized | narrow the declaration |
| constant stored through `AL` and returned | function has a real byte return | recover and use that return value |
| register roles permuted with changed flow | source block or local-lifetime shape is wrong | fix flow before chasing registers |
| `lea` for `x + 1` versus `mov`/`inc` | two locals where original reused one | merge and increment one variable |
| repeated derived cursors versus `add base, imm` | original mutates one base pointer | advance the base in place |
| initialization emitted in wrong order | declaration/initializer order differs | reorder locals and initializers |
| `dec`/`je` ladder versus comparisons | original likely used a small `switch` | test a `switch` |
| materialized call argument versus direct push | original duplicated calls per branch | put the call in each branch and let VC6 merge tails |
| merged stack cleanup versus separate cleanups | duplicated compiler tail written literally | write one source-level tail |
| duplicate return epilogues | separate conditions versus short-circuit expression | test `||`/`&&` with the observed order |
| peeled comparison or extra epilogue | hand-rolled cursor versus counted indexed loop | test the ordinary counted `for` form |
| global reloaded at every use | original kept it in a local | load once and reuse |
| parameter reloaded from stack | address-taken parameter forced to memory | copy to a local for arithmetic |
| tail `jmp` versus `call`/`ret` | direct return enabled tail call | name a local result before returning |

These are hypotheses, not recipes. Count returns and branches first. A transform that matches one
function can regress its sibling because the original intentionally used another shape.

## Proving layouts and types

Use emitted code to settle facts that static reading leaves ambiguous:

- allocation size immediately before a constructor call proves object size;
- stack-frame use bounds by-value local size;
- signed and unsigned branch opcodes type comparisons;
- byte-register operations expose byte-sized fields and returns;
- short-circuit evaluation order can prove field order;
- constructor base calls, vptr replacement, destructor restoration, adjustor thunks, and base-pointer
  uses prove subobject layout and inheritance.

Assertions can name and suggest types for members, but they do not place them. The asserting body's
memory operands place the field, and the VC6 comparison tests the complete declaration.

Do not infer multiple inheritance merely from two nearby vptr writes. Track the receiver for each
write and corroborate with lifecycle and dispatch evidence. Without that proof, retain an opaque
polymorphic subobject or additional vftable rather than inventing a base.

## SurRender registry evidence

SurRender classes expose registry names and IDs through virtual methods, and decorated imported base
names can carry the same template ID. This can identify classes without RTTI. Treat a registry string
as the concrete class's name only when the getter belongs to that class's own vtable; a Wizardry class
may register under a SurRender-facing base name.

An imported named base vftable referenced from Wizardry code proves derivation from that named base.
Virtual-base layout additionally requires vbptr/vbtable and base-constructor evidence.

## Choose targets that multiply evidence

Prefer small functions that settle reusable declarations: byte return types, field widths, field
order, object size, constructor families, container operations, and symmetric sibling pairs. A
70-byte getter/setter pair can prove more than a 5000-byte routine containing many unresolved facts.

Recover the immediate call graph needed for the next visible product transition. Do not expand the
task into unrelated library or tooling work.

## Library code is linked, not recovered

CRT startup, Win32, DirectX, MFC, SGP, zlib, IJG JPEG, and Info-ZIP implementations are not Wizardry
recovery targets when pinned source, headers, or system libraries already provide them. Classify each
traced node as first-party or library and use the canonical owner. SurRender remains recoverable where
its implementation is closed.

## Provenance

Every accepted identity records where it came from and its authority ceiling. Original assertion text,
decorated imports, registry strings, pinned headers, source paths, and behavioral descriptions are not
interchangeable evidence. Do not promote a plausible name or class model beyond what its source proves.
