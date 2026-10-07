# Recovered-source fidelity

These are the recovery-specific constraints that remain important after bulk function recovery. They supplement the general rules in [AGENTS.md](../AGENTS.md).

## Reconstruct source, not compiler output

Recover the most plausible authored circa-2000 C/C++ first: behavior, types, ownership, object layout, abstractions, source placement and header/TU visibility. Comparison is evidence and validation, not an objective function.

A clean decompilation does not prove original spelling or abstraction. A difference does not justify a less plausible source form. If faithful ordinary source leaves a compiler-only difference, keep the faithful source.

Do not add dummy variables, artificial aliases, overlapping locals, dead branches, redundant counters, fake wrappers, manual inlining, register-shaped temporaries, volatile barriers, casts or unions solely to manipulate code generation or decompiler output.

## Shared structure and compiler emissions

Recover shared helpers/templates before reproducing lowered bodies at callers. Repeated expansions can support a shared/header-visible owner; they do not by themselves prove exact original spelling or an `inline` keyword.

Do not use `__forceinline`, noinline attributes, optimizer pragmas or per-TU compiler-option overrides as matching knobs. Let VC6 choose once source structure and visibility are correct.

ICF, storage reuse, widened copies, template emissions, thunks and deleting destructors are compiler/linker evidence, not automatic source constructs:

- folded functions do not prove source aliases;
- deleting destructors and adjustor thunks do not require handwritten wrappers;
- an emitted template body does not establish exact template arguments unless the body or independent evidence distinguishes them;
- compiler-owned stack/register reuse does not justify overlapping source variables.

Compiler-generated identities belong in generated reccmp metadata, not invented source bodies or `FOLDED` markers.

## Types, fields and ownership

A machine-width access establishes an access width, not automatically a source field of that width. Before adding a wider field, overlay or union, test ordinary assignment, an embedded record and aggregate copies.

A source union requires positive evidence for overlapping authored storage, such as a discriminant, mutually exclusive lifetime/state or accepted source evidence. Coincident offsets or decompiler disagreement are insufficient.

Trace pointer identity through callers before assigning an offset or field to a class. If a modeled repository object only fits a signature through a reinterpret cast to an unrelated modeled object, the type/owner model is wrong or unresolved.

Do not convert typed W8/sr/st objects to byte pointers and use literal offsets. Raw byte arithmetic is for genuinely raw/external storage, not an escape hatch from the source model.

New `reinterpret_cast` and C-style casts in recovered C++ remain exceptional and must use the repository's existing waiver comments where the source-model gates require them. Do not add a waiver merely to preserve a known type disagreement.

## Behavior and lifecycle

Preserve retail bugs and undefined behavior when evidence establishes them. Do not initialize, clamp or guard recovered code merely to make the recomp safer.

Treat suspicious recovered code as an investigation, not an automatic fix. State only what the evidence establishes.

Construction/destruction should follow ordinary C++ object lifetime: bases and members at their real owners, initializer lists for construction, assignment only to already-live objects. Do not hand-write compiler-generated deleting destructors or cleanup wrappers.

## Source ownership

Preserve the established source/TU model in [wiz8-source-model.md](wiz8-source-model.md). Recover placement and visibility before using compiler controls.

Matching markers bind to the following source entity. Keep `// FUNCTION:` adjacent to its declaration/definition. Library/compiler-emission identities belong in reccmp metadata rather than duplicate source ownership.

SGP's released baseline remains the source oracle for SGP-owned code; `src/sgp` is the reconstructed Wizardry fork. Preserve documented historical API spelling at that boundary rather than normalizing it to modern preferences.
