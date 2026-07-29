---
name: matching-decomp
description: Techniques for recovering Wizardry 8 function bodies and struct layouts byte-exactly against Wiz8.exe. Use when porting a canonical function to owned C/C++, when a ported body is close but not byte-identical, when recovering field offsets/types/names, or when locating callers and class boundaries in the executable.
---

# Matching decompilation for Wiz8.exe

The VC6 target is not just a build — it is a **falsifier**. A wrong field offset, size, signedness
or field *order* does not compile to identical bytes. Use it to prove layout hypotheses, not merely
to reproduce code you already understand.

## The loop

0. **Decompile it first**: `just ghidra query <program> decompile 0x<addr>`. Ghidra carries the
   applied types, global names, and callee identities, so it names `g_fact_values` and `FileWrite`
   where a raw listing shows bare addresses. Reading instructions by hand to work out what a function
   *does* re-derives, badly, what the project already recorded. Use the full program selector
   (`wiz8` is ambiguous across 21 programs), set `COLUMNS` wide so rich does not wrap the payload,
   and parse with `strict=False` — plate comments contain raw newlines. Disassembly answers the
   narrower question of why two encodings differ; `just wiz8 diff-boundary <symbol>` is the tool
   for that.
1. Confirm the canonical function's extent (see *Function extents* below).
2. Write owned **C++** under `src/wiz8/` - the game is C++ and every original unit is a `.cpp`;
   only genuine C library code stays `.c` - add the file to the `WIZ8_GAMEPLAY_BOUNDARIES` source list
   in `CMakeLists.txt`, and mark it `// FUNCTION: WIZ8 0x<ADDR>`.
3. `just build WIZ8_GAMEPLAY_BOUNDARIES`.
4. Compare the emitted COMDAT against every build with relocation masking.
5. **`just verify-boundaries`** for the verdict, and `just compare WIZ8` for link-level problems it
   cannot see — wrong import names, unreachable functions, stale links. Only the first decides
   whether a body matches; see the warning below before reading any reccmp percentage.
6. `just check-markers`: the `// FUNCTION:` marker must sit immediately above its declaration, with
   any explanation above the marker rather than between the two.
7. Record the proved boundary in `config/reccmp/wiz8-gameplay-boundaries.csv` and update only the
   canonical evidence or target documentation that the result changes. `just check` validates
   identities and relationships without copied progress totals.

Comparison masks COFF `DIR32`/`DIR32NB`/`REL32` relocation fields, so global addresses and call
targets are irrelevant to the match — only instruction shape matters. Relocation masking does not
license a second name or declaration: search by address and aliases, then reuse the canonical
subsystem-owned declaration.

**Relink before trusting reccmp.** A successful `just build` does not guarantee the link step
reran, and a stale `Wiz8.exe`/`Wiz8.pdb` reports correct functions at 34–43% when they are actually
92–94%. Delete both and rebuild before reading any reccmp number.

To check whose sources a build dir belongs to, read `RECCMP_PROJECT_DIR_HOST` in its
`CMakeCache.txt`. The PDB itself is not the signal — the container build records `Z:\repo\...`
paths, and reccmp maps them to a host tree through that cache variable. If it names a checkout other
than yours, every number from that build dir is about someone else's code.

The build directory is `build/decomp` inside the checkout, so checkouts no longer share build state
even when they share `WIZ8_WORK_DIR`. `just configure` and `just compare` also refuse to run on a
build directory another checkout configured, so a moved or copied working copy fails loudly instead
of producing a plausible wrong number.

**Never choose between two candidate bodies by reccmp percentage.** reccmp diffs the *linked* image,
where our globals and call targets sit at different addresses than the original, so every relocated
operand counts as a difference. A byte-exact body scores far below 100%: `AddLinesToMessageBox` is
byte-identical under relocation masking and reccmp reports 75%, and most confirmed-exact bodies here
sit at 88–98%. The percentage is a whole-image progress signal, not a matching criterion.

The criterion is `just verify-boundaries`, which masks relocated operands and compares against the
original image. It reports `exact`, `near-miss`, `regressed` (reviewed exact, no longer matching --
the regression reccmp cannot see) and `promotable` (reviewed a near miss, now matching). Run it after
every transform, and read reccmp for link-level problems only.

This mattered concretely. On `GetOriginOfCharacterItem`, removing the loop peeling produced the
original's exact instruction sequence, and reccmp fell from 77.03% to 65.25% purely because a
register swap touches more operands. Reverting on that number was wrong: neither body is exact, and
the structurally-aligned one is a byte away rather than a shape away.

**Spelling `__thiscall`.** VC6 cannot put `__thiscall` on a free declaration, but `__fastcall`
passes its first argument in ECX, which is the instruction a no-argument member call emits. That
does not extend to a member call *with* arguments: `__fastcall` would place the second argument in
EDX, and the canonical passes it on the stack. Those need a real virtual or member call, which is
one more reason owned units are C++.

**A duplicated tail in the decompiler output is often the compiler's, not the source's.** Ghidra
shows what the binary does, so a compiler-duplicated epilogue appears as a real second copy. Writing
it out literally can cost bytes: on `InitializeFactState` the duplicated call before an early return
made VC6 merge two argument cleanups into one `add esp,0x10` where the original keeps `add esp,0xc`
and `pop ecx`. Writing the tail once, and letting the compiler duplicate it, was byte-exact.

**Diff whole instructions, not mnemonics.** A diff that compares only the mnemonic silently hides
operand and register differences, which are exactly what most near-misses consist of. Compare the
full instruction text and the encoded size, normalising only branch targets. A one-byte delta with
an identical instruction count usually means a register swap rather than a missing instruction:
`add eax,imm32` encodes a byte shorter than the same add on any other register, and several
`eax`-specific forms behave the same way.

## Reading a near-miss

When the size is right but bytes differ, the cause is almost always source shape, not structure.
Ranked by how often it has been the answer here:

| Symptom | Cause | Fix |
| --- | --- | --- |
| `JL`/`JE` where canonical has `JB`/`JBE` | field or variable is **unsigned** | change the type |
| `JE` where canonical has `JBE` on an unsigned value | source wrote `if (x)` where the original wrote `if (x > 0)` | spell the comparison out; truthiness and `> 0` differ in encoding even though they agree in meaning |
| Register roles permuted | control-flow shape is wrong | fix the flow; registers follow |
| `lea r,[x+1]` where canonical has `mov r,x` then `inc r` | two source variables where the original reused one | merge them into a single variable stepped by `++` |
| A second cursor built with `lea` where canonical has `add r,imm` | the original advances one base pointer in place | reuse and mutate the base rather than deriving each cursor |
| Index cleared *after* the cursor is computed | initialisation order | assign the index before the pointer, with an empty `for` init |
| `cmp r,1`/`jne` chain where canonical has `dec r`/`je` | an if/else-if chain where the original wrote a `switch` | use `switch`; VC6 emits the dec/je ladder for small dense cases |
| A constant materialised into a register then pushed, where canonical pushes it directly | one call with a selected argument, where the original wrote a call per branch | put the whole call in each branch and let VC6 tail-merge them |
| Two argument cleanups merged into one `add esp,N` | a tail the *compiler* duplicated, written out literally in the source | write the tail once and let VC6 duplicate it |
| Duplicated `return` epilogue | separate `if`s where the original used one short-circuit `\|\|` | merge into `\|\|` |
| Loop's first comparison duplicated, two `return` epilogues | hand-rolled cursor in a `do`/`while` | use a counted `for` indexing the array; let VC6 strength-reduce it |
| Extra redundant bound test | post-loop test the original did not have | make the search a separate `__inline` helper that returns the sentinel |
| Dead guards missing | `goto` let VC6 range-propagate and delete them | same fix — an inlined helper keeps the value opaque |
| A global reloaded for each use where canonical loads it once into a callee-saved register | the original read it into a local | assign it to a local and use that throughout |
| A parameter reloaded from its stack slot mid-body, costing an extra `lea` in an address chain | its address is taken elsewhere, so VC6 forces it to memory | copy it into a local and use that for the arithmetic; the slot still serves as the address |
| Registers clear in the wrong order | declaration/initialization order | reorder the initializers |
| `neg r`/`sbb`/`neg` where canonical has `test al,al` then `setne al` | a comparison returned directly, which widens to a 32-bit 0/1 | assign it to a `bool` local first, then return that |
| A constant stored with an immediate where canonical materialises it into AL first | the function also *returns* that constant | give the gate its real return value; VC6 then uses one register for both |
| `neg eax` where canonical has `neg al` | callee's **return type** is byte-sized, not `int` | narrow the extern declaration |
| A tail `jmp` to the callee where canonical has `call` then `ret` | the result was returned directly, so VC6 tail-called | name a local for it and return that; the local blocks the tail call without changing the value. Confirmed on `MonsterGetSubobjectValue120`, whose float return went 0.57 → instruction-identical |

Iterating on shape is normal. Sizes of 261 → 162 → 247 → 231 before an exact match happened here.

## Proving layout, not guessing it

**Signedness.** `JB`/`JBE` means unsigned, `JL`/`JLE` means signed. This types fields the
decompiler shows as plain ints.

**Field order via short-circuit.** When an assertion reads `a->fFoo || a->fBar`, `||` evaluates its
left operand first, so the byte order of the two emitted tests decides which offset is which.
Swapping them reorders the tests and the body stops matching. This settles orderings that static
reading cannot.

**Loop shape.** VC6 lowers a counted `for (i = 0; i < n; ++i)` over `base[i]` into a guard plus a
rotated `do`/`while` with a single backward branch, and sinks the `base` load past the guard because
it is only needed inside. Writing the same loop with your own cursor pointer instead makes it peel
the first comparison and emit a second epilogue. If a search function is close but ~8 bytes long
with a duplicated compare, try the counted-`for`-over-index form before anything else. Confirmed on
`PListIndexOf`, `IListIndexOf` and `FindFirstMonsterByID`, the last of which had been recorded
`structurally-strong` for exactly this reason.

The transform is: a guarded `do`/`while` with an early `return` inside becomes a counted `for` whose
body `goto`s a single shared exit label, with the not-found value assigned just before that label.
One backward branch, one epilogue.

Apply the shared-exit half **only when the canonical has one `ret`**. Count the `ret`s first:
`MonsterGetIndexByLocationID` has three distinct return values and three epilogues, and matched with
the loop conversion alone once the returns were left in place.

**One variable, not two.** When a search returns a position that then becomes the loop index, the
original often keeps both in the same variable and steps it with `++`. Declaring separate `position`
and `index` locals makes VC6 fold the increment into a `lea`, which costs a byte and shifts every
later offset. This closed the final difference in `FindNextExistingMonsterByID`, and it is a
plausible reading of the "register-role swap" noted on the remaining near-misses.

It does **not** apply when the *original* peels deliberately. In `FindMonGenByName` the canonical
back-edge lands past the bounds check, so the check runs only on the first iteration and the body
has two epilogues; that shape has to be reproduced as written rather than normalised.

**Object size.** Search the caller for `push <size>; call <allocator>` immediately before the
constructor call. That is the exact object size.

**By-value locals.** A caller's stack-frame size bounds a struct passed by value.

## Assertion strings are the richest name source

`srAssertFail` (imported from `SR.DLL`) is called from ~1048 sites; `evidence/observations/wiz8/assertions.csv`
holds the 1038 that decode. Arguments are cdecl `(expression, source_path, line, message)`, pushed
right-to-left, so the last `68` push before the call is the expression pointer.

They yield far more than file ownership:

* member names, via `->` or `.`. An `m_` prefix is a per-class habit here, not a project rule -
  most member accesses have none - so check the owning class rather than assuming it;
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

## The SurRender class registry

SurRender classes carry two registry slots: a virtual `getClassName` returning a literal string and a
virtual `getClassID` returning a constant. Reading them identifies a class with no RTTI at all, and
the ID ranges separate the codebases — Wizardry-registered classes use `0x10000` and up, SurRender's
own sit near `0x1200`–`0x3110`. `srClassSupport<Iface, Node, 0, ID>` in a mangled base name carries
the same ID.

A class-registry name is `original-runtime-string` evidence. Treat it as the class's own name only
when the getter belongs to that class's vtable: a Wizardry class may register under a SurRender base
name to present itself to the scene graph.

## Class structure without RTTI

`Wiz8.exe` is `/GR-` and has zero RTTI type descriptors, but imports 461 decorated C++ symbols from
`SR.DLL` over 49 classes.

* An imported `??_7Class@@6B...` **vftable** referenced from `.text` marks a function installing a
  base vptr — proof of derivation from a *named* base, with the mangling spelling out the
  inheritance.
* Two vptr writes a few bytes apart prove only that the function installs two tables. Track each
  receiver from entry `this`; then use base-constructor calls, derived-table replacement,
  destructor restoration, adjustor thunks, and base-pointer uses to prove multiple inheritance.
  Without that lifecycle and dispatch evidence, retain an additional vftable or opaque
  polymorphic subobject rather than inventing a base.
* A vbptr at `+4` selecting a vbtable means **virtual** inheritance: vbtable entry 1 is the offset
  from the vbptr to the virtual base subobject. Corroborate against the constructor's own
  `lea ecx, [this+offset]` base call.
* Derived constructors install base vptrs first, then overwrite with their own.

## Library code is linked, not modelled

CRT startup, MFC, Win32 and DirectX code are **not recovery targets**. Mark them as library and let
the linker supply them, the way `imperialism-decomp` links `gdi32 user32 winmm vfw32 dsound …` with
a comment naming which calls need each one. Never write a body for `__WinMainCRTStartup`,
`_initterm`, CRT helpers or Win32 wrappers.

Classify every node on a traced path as `library` or `first-party` before proposing work on it, and
record which library provides each library node so the runtime target knows what to link.

## Provenance

Every accepted name records where it came from; see `docs/wiz8-evidence-model.md`. Names taken from
assertion text are `original-runtime-string`; names from decorated SR imports are `original-export`;
behaviour-derived names are `descriptive`. Do not promote a name because it looks right.
