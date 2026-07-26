---
name: matching-decomp
description: Techniques for recovering Wizardry 8 function bodies and struct layouts byte-exactly against Wiz8.exe. Use when porting a canonical function to owned C/C++, when a ported body is close but not byte-identical, when recovering field offsets/types/names, or when locating callers and class boundaries in the executable.
---

# Matching decompilation for Wiz8.exe

The VC6 target is not just a build — it is a **falsifier**. A wrong field offset, size, signedness
or field *order* does not compile to identical bytes. Use it to prove layout hypotheses, not merely
to reproduce code you already understand.

## The loop

1. Disassemble the canonical function and find its extent (see *Function extents* below).
2. Write owned C/C++ under `src/wiz8/`, add the file to the `WIZ8_GAMEPLAY_BOUNDARIES` source list
   in `CMakeLists.txt`, and mark it `// FUNCTION: WIZ8 0x<ADDR>`.
3. `just build WIZ8_GAMEPLAY_BOUNDARIES`.
4. Compare the emitted COMDAT against every build with relocation masking.
5. Record in `config/analysis/reccmp/wiz8-gameplay-boundaries.csv`, `docs/targets/`, and the
   relevant test counts in `tests/unit/test_wiz8_source_model.py`.

Comparison masks COFF `DIR32`/`DIR32NB`/`REL32` relocation fields, so global addresses and call
targets are irrelevant to the match — only instruction shape matters. Externs may therefore be
declared with any convenient name.

## Reading a near-miss

When the size is right but bytes differ, the cause is almost always source shape, not structure.
Ranked by how often it has been the answer here:

| Symptom | Cause | Fix |
| --- | --- | --- |
| `JL`/`JE` where canonical has `JB`/`JBE` | field or variable is **unsigned** | change the type |
| Register roles permuted | control-flow shape is wrong | fix the flow; registers follow |
| Duplicated `return` epilogue | separate `if`s where the original used one short-circuit `\|\|` | merge into `\|\|` |
| Extra redundant bound test | post-loop test the original did not have | make the search a separate `__inline` helper that returns the sentinel |
| Dead guards missing | `goto` let VC6 range-propagate and delete them | same fix — an inlined helper keeps the value opaque |
| Registers clear in the wrong order | declaration/initialization order | reorder the initializers |

Iterating on shape is normal. Sizes of 261 → 162 → 247 → 231 before an exact match happened here.

## Proving layout, not guessing it

**Signedness.** `JB`/`JBE` means unsigned, `JL`/`JLE` means signed. This types fields the
decompiler shows as plain ints.

**Field order via short-circuit.** When an assertion reads `a->fFoo || a->fBar`, `||` evaluates its
left operand first, so the byte order of the two emitted tests decides which offset is which.
Swapping them reorders the tests and the body stops matching. This settles orderings that static
reading cannot.

**Object size.** Search the caller for `push <size>; call <allocator>` immediately before the
constructor call. That is the exact object size.

**By-value locals.** A caller's stack-frame size bounds a struct passed by value.

## Assertion strings are the richest name source

`srAssertFail` (imported from `SR.DLL`) is called from ~1048 sites; `config/analysis/wiz8/assertions.csv`
holds the 1038 that decode. Arguments are cdecl `(expression, source_path, line, message)`, pushed
right-to-left, so the last `68` push before the call is the expression pointer.

They yield far more than file ownership:

* member names, via `->` or `.`, using the original's `m_` convention;
* named constants and enumerators (`BAD_INDEX`, `CYCLE_NUM_UNIQUE`, `MAX_MONSTERS_IN_DATABASE`);
* **types**, via Hungarian prefixes established from the original's own text — `p` pointer,
  `ui`/`i` `UINT32`/`INT32`, `f` flag, `b`/`ub`/`us` byte/unsigned byte/`UINT16`, `g`/`gp`/`gui`
  global forms, `psr` SurRender object, `pls` `PList`, `pac`/`pst`/`h`;
* sometimes the class name, when the call also passes a *message*
  ("Too many props loaded for Octree").

Assertions name and type fields but do **not** place them. The asserting body supplies the offset,
and the port proves it.

**Pick targets by size.** Sort candidate asserting functions by extent before choosing. A dense
cluster of member names is worthless if all its asserts sit in one 5000-byte function; a 70-byte
sibling pair proves more per unit of effort. Prefer symmetric pairs — they share a layout and cost
little more than one.

## Locating things in the executable

**Do not trust linear-sweep disassembly for xrefs.** It desyncs and silently reports zero callers.
Search the encoding instead:

* direct calls — scan for `E8` and check `target == va + 5 + rel32`;
* global/vtable references — scan for the little-endian address bytes.

**Function extents.** Inter-function padding is a run of `0x90`. Scan backwards for two consecutive
`0x90` to find a start, forwards to find an end. Reliable here, unlike linear sweep.

## Class structure without RTTI

`Wiz8.exe` is `/GR-` and has zero RTTI type descriptors, but imports 461 decorated C++ symbols from
`SR.DLL` over 49 classes.

* An imported `??_7Class@@6B...` **vftable** referenced from `.text` marks a function installing a
  base vptr — proof of derivation from a *named* base, with the mangling spelling out the
  inheritance.
* Two vptr writes a few bytes apart in one function means multiple inheritance.
* A vbptr at `+4` selecting a vbtable means **virtual** inheritance: vbtable entry 1 is the offset
  from the vbptr to the virtual base subobject. Corroborate against the constructor's own
  `lea ecx, [this+offset]` base call.
* Derived constructors install base vptrs first, then overwrite with their own.

## Provenance

Every accepted name records where it came from; see `docs/wiz8-evidence-model.md`. Names taken from
assertion text are `original-runtime-string`; names from decorated SR imports are `original-export`;
behaviour-derived names are `descriptive`. Do not promote a name because it looks right.
