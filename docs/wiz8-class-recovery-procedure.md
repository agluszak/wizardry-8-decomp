# Recovering a class end to end

This procedure takes an unnamed vtable address to a reviewed, byte-proven class without building a
second object model beside Ghidra.

## Sources of authority

Use these in descending order:

1. **The original image and matching build.** Relocation-masked boundary verification is the final
   proof of a recovered body.
2. **The checkout's canonical Ghidra project.** It owns function containment, symbols, signatures,
   data types, vtables, references, comments, and decompiler state.
3. **Tracked observations and reports.** They provide exhaustive leads and provenance, but they do
   not outrank direct inspection of the Program database.

Create or deliberately refresh the project before a recovery session:

```sh
CANON=wiz8--gog-base--wiz8--18a74ff61c65
just ghidra rebuild "$CANON"
```

Queries are read-only one-shot batches against that existing project. They never restore a seed or
create a speculative clone.

## 1. Pick a target

Generate the class-candidate report:

```sh
just wiz8 report class-candidates
```

`build/reports/class-candidates/candidates.csv` is a lead generator. Prefer a candidate with:

- a slot-zero target;
- one or two constructor/destructor-shaped vptr writers;
- an allocation-size hint;
- few imported slots;
- no match in `evidence/observations/wiz8/ptr-vector-instantiations.csv`.

Do not promote a report row merely because it looks coherent. Resolve every address against Ghidra
first.

## 2. Review the family in Ghidra

Start with one joined context packet, then batch any extra questions:

```sh
just wiz8 report context 0x<writer> --program "$CANON" --deep --root this
just ghidra query "$CANON" \
  -q 'decompile 0x<writer>' \
  -q 'function-of 0x<writer>,0x<other-writer>' \
  -q 'read-data 0x<vtable> 0x<bytes>' \
  -q 'xrefs-to 0x<address-after-vtable>'
```

Settle these facts in order.

### Writer roles

Construction order is base construction, own fields, then the derived vptr store. Destruction order
is the reverse: restore the class vptr, destroy members, then destroy the base. A wrapper that calls
the complete destructor and conditionally calls `operator delete` is the scalar deleting destructor.

### Base extent

The first constructor call usually identifies the base constructor. Confirm the base's extent using
both its own allocation/layout evidence and the offset where derived storage begins. Record an
unnamed positional base when no original name survives.

### Allocation size

Verify each hint at a caller shaped like `operator new(N)` followed by the constructor. The size
belongs to the most-derived object at that call site, not automatically to every base constructor
that runs inside it.

### Vtable boundaries and secondary tables

Use `just wiz8 report class-family <vtable>` as a compact cross-reference view, but adjudicate the
writers and table extents in Ghidra. A code reference to the address immediately after a run is
strong evidence that the preceding slot is the table's end. Multiple writes at the same object
offset usually show base-then-derived construction; writes at non-zero offsets identify subobjects.

### Names

Use an original class-registry string, source path, export, assertion, or other program-supplied name
when one exists. Otherwise use an address-qualified positional name such as
`W8Dialog005A80A0`. Behavior can be described in evidence without pretending that a descriptive
name is original.

## 3. Record provenance and replay-backed claims

Add only facts whose tracked representation carries value beyond the native Ghidra object:

- `classes.csv` for reviewed identity, constructor/destructor triad, size, bases, provenance and
  layout proof;
- `vtables.csv` and `vtable-slots.csv` for reviewed table identity and slot relationships;
- `fields.csv` while the replay bridge still consumes reviewed externally justified layout claims;
- `functions.csv` and `function-evidence.csv` for identity provenance and independent evidence;
- `config/reccmp/wiz8-gameplay-boundaries.csv` for source-to-image proof targets.

Do not add a second CSV merely to export the current Ghidra structure, symbol, signature, or comment.
Generated views belong under `build/`.

Run:

```sh
just test
just ghidra rebuild "$CANON"
just ghidra query "$CANON" \
  -q 'facts-at 0x<class-anchor>' \
  -q 'function 0x<writer>'
```

`rebuild` is explicit because it may replace replay-backed state. Ordinary queries never mutate or
refresh the project.

## 4. Port for byte proof

Use the reviewed class name and model the hierarchy honestly:

- real base classes for proven bases;
- opaque storage for unresolved regions;
- virtual placeholders only where needed to place a recovered method at its canonical slot;
- canonical declarations for callees and globals rather than wrappers or duplicate externs.

Add the translation unit to `src/wiz8/sources.cmake`, put its marker immediately above the
definition, and add a structurally strong boundary row with an empty digest:

```sh
just build WIZ8_GAMEPLAY_BOUNDARIES
just wiz8 diff-boundary '<Class>::<Method>' --all
```

`~~` lines are relocation differences. `>>` lines are real instruction differences. When the body
has zero real differences, record the relocation-masked digest, promote the boundary to `exact`, and
finish with:

```sh
just wiz8 verify-boundaries
just check
just test
```

The compiler result may prove layout facts even when the source body is empty: destructor vptr
stores, base offsets, scalar deleting destructors, and constructor initialization order are all
useful evidence.

## 5. Resolve concurrent evidence edits safely

When a rebase conflicts in reviewed tables, use the repository resolver rather than concatenating or
choosing a side by hand:

```sh
just wiz8 resolve-evidence-conflict config/reccmp/wiz8-gameplay-boundaries.csv
```

Rebuild after the rebase before running boundary verification. A stale object directory can make an
untouched exact row appear regressed.

## Stop conditions

A class is complete enough for current work when:

- Ghidra contains the useful symbol, vtable and layout state;
- tracked rows explain the authority and evidence without duplicating the Program database;
- recovered source uses one canonical declaration;
- the relevant bodies pass relocation-masked verification;
- no speculative overlay or parallel class database is required to understand it.
