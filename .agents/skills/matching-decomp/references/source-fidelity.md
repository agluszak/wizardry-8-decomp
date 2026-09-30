# Recovered-source fidelity

These rules apply to Wizardry/SurRender source recovery. Read this reference before changing
recovered source; compiler and comparison evidence never override its source-model constraints.

## Special members, construction and inlining

Prefer implicit compiler-generated copy constructors and assignments when retail is fully explained
by ordinary base/member special-member operations in declaration order. Before recovering a
handwritten copy constructor, attribute every call/store to the relevant subobject lifetime: a member
copy constructor may itself default-construct and call assignment. Use initializer lists for actual
base/member construction; body assignment requires an already-live object. Conflicting lifetime and
declaration order calls for investigating the class model, not rearranging initializers. Do not
duplicate lifecycle behavior supplied by bases/templates, including registration and cleanup. Preserve
established shallow or unsafe copying; modern Rule-of-Three reasoning is not original-source evidence.
See [type-modeling lifecycle guidance](../../type-modeling/SKILL.md#classes-and-lifecycle) for the
subobject procedure and emission boundaries.

Recover shared helpers/templates before reproducing their lowered bodies at callers. Inlining is a
source/visibility claim: an expanded body alone does not prove `inline`; repeated expansions suggest
a shared owner, and a standalone emission plus expanded uses is consistent with header-visible source.
A call/inline disagreement does not justify `__forceinline`, `__declspec(noinline)`, `#pragma auto_inline`
or optimizer pragmas. Investigate header/TU visibility, template ownership and source structure first;
let VC6 choose unless independent evidence establishes stronger authored control. Existing pragma-based
sites are not precedent. A complete record computed then copied should first suggest a typed value
temporary and ordinary assignment, such as `Matrix result; ...; *this = result;`, rather than invented
scalar scratch storage or field-by-field lowering. Distinct overloads remain distinct source functions;
similar emitted bodies do not merge their symbol/call identities.

## Other source-model constraints

- **Reconstruct first; investigate remaining differences afterwards.** Recovery has two distinct
  phases. First recover the most plausible authored circa-2000 C++ and VC6 ABI: behavior, types,
  ownership, object layout, abstractions, source placement and header/TU visibility. During
  reconstruction, comparison is evidence and validation, not an objective function. Prefer the better
  source model even when its decompiled diff is larger.
- Only after the affected source model is coherent may remaining differences be treated as a
  code-generation question. Investigate compiler/toolchain version differences, ABI, pragmas,
  declaration order, TU/header visibility, inlining and other genuine source/compiler causes. Do not
  distort recovered source merely to reproduce VC6 lowering or to shrink a decompiler diff.
- reccmp decompiles both sides with Ghidra and diffs them with Ghidriff. A `no-differences` result is
  useful evidence that the source reproduces retail behavior as Ghidra sees it; it proves neither
  equivalence nor that the source spelling or abstraction is original. Conversely, a difference does not
  justify a less plausible source form. Accepted source oracles and source-model evidence outrank the
  comparison; it may distinguish between independently plausible authored forms.
- Never invent, omit, stub or approximate retail behavior. Never add dummy variables, artificial aliases,
  overlapping locals, dead branches, redundant counters, fake wrappers, manual inlining, register-shaped
  temporaries, volatile barriers, unions, casts or other constructs solely to manipulate register
  allocation, stack layout, scheduling, CFG shape or a decompiled diff. If faithful ordinary source
  leaves a compiler-only difference, retain the faithful source and record the lowering gap.
- When stronger evidence changes the reconstructed model, fix the model even if the comparison
  temporarily shows more differences, then investigate them from that corrected baseline. A known-unfaithful construct
  is matching debt, not precedent; prefer removing it and fixing the shared owner, ABI, toolchain model,
  header/TU placement or tooling when the evidence supports that explanation.
- Treat fidelity as an ordered stack: source-model fidelity first, semantic/ABI fidelity second, codegen
  fidelity third. Never knowingly make the source model less plausible to improve a comparison.
  Investigate remaining logical differences after reconstructing the authored abstraction; if what
  remains is decompiler representation or established compiler/linker lowering, keep the clean source
  and record it instead.
- Source-shaping compiler controls are source claims, not matching knobs. Do not add `__forceinline`,
  noinline attributes, per-function optimizer pragmas or equivalent controls merely to change generated
  code. They require positive source/oracle evidence or strong independent evidence about authored
  visibility/control. Existing suspicious sites are recovery debt, not precedent.
- Never promote compiler output into an authored source construct. A concrete template emission proves
  only that the primary template was instantiated for those arguments; it never proves an explicit
  specialization or explicit instantiation. Likewise an inlined copy does not prove manual inlining,
  a deleting destructor does not prove a handwritten wrapper, and folded functions do not prove aliases.
  Retail ICF is linked-image evidence, not source identity: recovered source must not use reccmp
  `FOLDED` markers. Mark only the retained retail emission; independently evidenced sibling source
  functions remain ordinary unmarked functions.
  Keep template behavior in the primary template unless an accepted original-source oracle directly
  establishes otherwise. Recovered Wizardry/SurRender source gates new explicit specializations and
  instantiations without a comment waiver; an oracle-backed exception must change that reviewed gate.
- Compiler-owned storage reuse is not source evidence. Never alias a parameter/local or add overlapping
  source variables merely to reproduce stack-slot, register, spill or temporary reuse. Introduce the
  logical source variables even when that grows the decompiled diff.
- A machine-width access does not establish a source field of that width. Treat widened loads/stores,
  dword/block moves, `memcpy`, and same-offset alternate interpretations as compiler/aggregate-copy
  evidence first. Before adding an overlay or decomposing copied storage into sibling fields, trace the
  complete source/destination extent and test an existing embedded record or ordinary assignment.
  "Same offset", "same size", a wide move, or decompiler type disagreement is not positive union evidence.
- Trace pointer identity through callers before assigning a callee offset to a class. If satisfying a
  recovered signature requires reinterpret-casting one modeled W8/sr/st record pointer to another, the
  signature/owner model is wrong or unresolved; do not bless the cast. This includes pointer-to-pointer
  casts such as `T** -> U**`. Linker-folded sibling functions likewise do not make their source parameter
  types interchangeable.
- Search for the authored abstraction before spelling out a lowered sequence. Existing container/math/
  traversal helpers should be used when their semantics fit; repeated equivalent sequences across
  independently owned TUs trigger an inline/helper investigation rather than copy-pasted lowering.
- Establish behavior, then name it. `FunctionXXXXXX`, `FUN_...` and `unknown_...` are placeholders,
  not identities. Rename the definition, declaration, callers and ownership/provenance references in
  the same coherent change. When original spelling is unknown, use a behavior-descriptive name.
- Preserve ordinary counted `for` loops instead of reproducing guarded `do`/`while` lowering. Do not
  add redundant counters, artificial scopes, duplicate cleanup, return temporaries or rearranged
  expressions merely to change registers, CFG or the decompiled diff.
- Preserve retail bugs/UB when evidence establishes them. Do not initialize, clamp, guard or otherwise
  normalize recovered code merely to make the recomp safer or deterministic. An explicitly requested
  compatibility deviation must be isolated and documented, never disguised as the recovered body.
- Treat suspicious recovered code as an investigation, not a mandate to produce a fix. If retail does
  the same thing, record the evidence in the issue/review and close the investigation; do not manufacture
  a comment-only C++ change unless a short source note prevents a likely future semantic mis-recovery.
- State only what the evidence establishes. A missing writer/check/free/import proves that operation is
  absent in the audited scope; it does not by itself prove intent, an authored ownership contract,
  global reachability, "by design", or that the behavior is a bug. "Not imported by Wiz8.exe" means no
  static import in that consumer, not that an export is unreachable.
- A vtable, lifecycle body, deleting destructor, address or template emission alone does not prove an
  authored class. Compare canonical bases/templates first. Compiler-generated deleting destructors,
  vtordisp/adjustor thunks and other compiler helpers are marker-only `SYNTHETIC`; template
  instantiations are `TEMPLATE`, never `FUNCTION`. `SYNTHETIC` and `LIBRARY` markers are
  marker-only; generic template implementations remain at their canonical template owner. A
  compiler-emission TU contains provenance only, not handwritten function/global definitions.
- Matching markers bind to the following source entity. Keep `// FUNCTION:` immediately adjacent to
  its declaration/definition; move pragmas/unrelated comments above the marker. Follow the matching
  skill for `TEMPLATE`, `SYNTHETIC`, `LIBRARY`, `VTABLE` and `GLOBAL` ownership.
- Preserve TU ownership/order in `src/wiz8/sources.cmake`. Recover placement before optimizer control:
  ordinary functions stay unannotated; header visibility/inlining requires cross-TU/call-site evidence.
  The comparison build intentionally uses `/OPT:NOICF`; an ICF-derived call-target or vtable difference
  is acceptable once the fold is independently established. Do not add aliases, casts, or source
  markers merely to force the retained retail address. The retired `identity-alias:` escape hatch
  must not be reintroduced; a proven consumer/provider ABI prototype discrepancy uses the narrow
  `abi-prototype-ok: <reason>` waiver instead.
- Legitimate `reinterpret_cast` sites express storage the type system cannot: external ABI, raw
  serialized/pixel memory, tagged storage, deliberate address/bit reinterpretation or an explicitly
  unresolved site. New casts require an attached `reinterpret-ok: <reason>` comment; a marker never justifies
  hiding known type disagreement.
- Source unions require positive evidence for overlapping authored storage: a real discriminant,
  mutually exclusive lifecycle/state, an accepted source oracle, or equivalent source-level evidence.
  Coincident offsets, equal sizes, widened copies and decompiler type disagreement do not qualify.
- Do not convert a repository-typed object to `char*`/`unsigned char*` and add a literal byte
  offset. The whole-tree source-model gate hard-rejects this for `this` and typed W8/sr/st pointers
  or references; there is no waiver. `raw-offset-ok: <reason>` remains only for genuinely
  unresolved/external raw storage that is not a modeled repository object.
- Do not invoke recovered code by reinterpret-casting storage/an address to an inline function-pointer
  type. Give the callable its evidence-backed declaration and calling convention; the whole-tree
  source-model gate has no waiver for this recovery shortcut. Ordinary declared callbacks remain valid.
- New C-style casts in recovered C++ are gated. Prefer the evidence-backed typed model or the specific
  C++ cast that states the proven conversion; a genuinely unavoidable historical C/ABI spelling needs
  attached `c-style-cast-ok: <reason>` comment. Cast comments may immediately precede the statement or
  remain on its formatter-wrapped continuation; never disable formatting to keep them on one line.
- `clang-format off` is not a matching technique. A new suppression needs same-line
  `format-off-ok: <reason>` and must cover the smallest construct the formatter genuinely cannot
  preserve; never disable formatting for a whole recovered function just to keep decompiler shape.
- SGP's released source spells textual filenames and format strings as `UINT8*` (`LoadButtonImage`
  and related APIs); preserve those declarations as historical ABI/API spelling rather than
  pretending `STR8`/`char*` was original. Wizardry text declarations use `char*`/`wchar_t*`;
  the cast at an SGP call is the documented boundary.
- Never use `unsigned char`/`UINT8` for a character merely because it is one byte.
