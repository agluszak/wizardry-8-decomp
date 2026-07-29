# Repository instructions

This is a Jujutsu repository for evidence-driven Wizardry 8 matching decompilation. Use Beads for
durable task state and `just` for the supported daily workflow.

## Authority and ownership

- Ghidra owns operational analysis state: function boundaries and names, signatures, labels,
  namespaces, comments, structures, fields, enums, vtables, applied types, cross-references, and
  decompiler state.
- Git owns recovered C++ source, declarations, translation-unit ownership, source/link order,
  compiler settings, matching markers, and provenance claims.
- Provenance explains why an identity is accepted, its origin and authority ceiling, confidence,
  aliases, and supporting observations. It must not clone Ghidra's complete function or type model.
- Generated projections under `build/` bridge canonical authorities. They are disposable and must
  never become editable evidence.
- Do not reverse engineer implementations whose source or declarations are available. Windows,
  MSVC runtime, SGP, zlib, IJG JPEG, and Info-ZIP behavior comes from pinned source/header oracles.
  SurRender is closed middleware and remains a valid recovery subject.
- Search before declaring anything. A function, global, class, field, vtable, import, or constant
  has one canonical owner. Extend it; do not add duplicate externs, raw vtable calls, wrappers,
  guessed aliases, or parallel inventories.
- Preserve proven original translation-unit ownership and the explicit matching-sensitive order in
  `src/wiz8/sources.cmake`. Keep address-qualified template emissions separate until ownership is
  proved.
- Never commit game binaries, extracted trees, live Ghidra projects, or build products. The
  reviewed GZF checkpoint declared by `vendor/ghidra/exports/manifest.json` is the sole approved
  tracked analysis artifact.
- Preserve the SFI-SCLA and upstream notices under `third_party/sfi-sgp`.

## Recovery loop

Restore the reviewed project once, then edit and version that project directly:

```sh
uv run wiz8 ghidra restore
uv run wiz8 ghidra sync-source --apply
just context 0x<address>
```

The context command is the supported joined interface. It combines provenance, source ownership,
match state, cross-build mappings, strings/assertions, callers/callees, decompilation, and relevant
fields. Use the Ghidra UI/API for ordinary listing, symbol, type, and cross-reference work. For a
speculative experiment, clone or version the project, use undo, or use a temporary GZF.
`ghidra sync-source` applies source-owned names and resolvable prototypes transactionally;
unresolved source types are reported explicitly rather than replaced with guessed Ghidra types.
There is no generic `just ghidra query` command.

Recover the immediate call graph needed for the next visible product transition. Tooling is retained
only when it repeatedly saves recovery work, proves emitted code or runtime behavior, or preserves
evidence Ghidra cannot express.

Marker policy follows reccmp's entity conventions:

- `// FUNCTION:` must sit immediately above its C++ declaration.
- `// TEMPLATE:` must be followed immediately by the specialization-symbol comment and then the
  template definition.
- `// LIBRARY:` is address-only and has no owned declaration adjacency requirement.

Put explanatory prose above the marker sequence.

## Validation

Choose the narrowest lane while iterating and run the complete lane before publication:

| Change | Required validation |
| --- | --- |
| Python, docs, evidence, marker, or source inventory | `just check` |
| Ordinary local work | `just test` |
| Ported/recovered body | `just build WIZ8_GAMEPLAY_BOUNDARIES`, `just wiz8 verify-boundaries`, then `just test` |
| Reviewed Ghidra change | representative `just context` checks, `uv run wiz8 ghidra index`, then an intentional checkpoint refresh |
| Complete product/integration gate | `just verify` |

Relocation-masked boundary verification is the exact-body authority. Linked-image `compare` is
diagnostic because relocated globals can reduce its score even for exact bodies. Preserve
`/OPT:NOREF` comparison and `/OPT:REF` runtime modes.

Use `just compare ADDRESS...` or `just compare --file SOURCE...` for one batch comparison and
`just triage` for reccmp's structured first divergence. `just vtable`, `just datacmp`, and `just
addr` expose reccmp's owned vtable/data/pairing analyses; they do not supersede boundary proof.

Use `just runtime-test` for the separate in-process semantic product. Its input is queued from the
test driver but consumed by the real screen handler on the UI thread; forward/reverse normalized
observations are the determinism criterion. Do not add coordinate automation or test branches to
matching bodies.

`just check` owns formatting, lint, typing, source inventory, fixture-based unit tests, one
checked-tree repository lane, marker policy, repository hygiene, and reccmp decomplint. Do not put
live counts or generated reports into tests or Markdown.

## Workspace and publication

Give every checkout its own absolute `WIZ8_WORK_DIR`. Never hardlink a live `ghidra/` directory.
Product builds use a per-checkout lock under `build/decomp`.

At session start, run `bd prime`, `bd dolt pull`, inspect `bd ready`, and claim or create a Bead
before substantial work. A Bead is a durable outcome, not a commit: one Bead may contain several
local jj commits and normally produces one review bookmark/PR. Push Beads immediately after a
claim and after meaningful ownership, dependency, scope, review, or status changes. Record partial
evidence, note the review bookmark/PR, and close only after the implementation is integrated.

Use Jujutsu, not raw Git, for repository history. Preserve unrelated working-copy changes, inspect
`jj status` after meaningful edits, and create local checkpoint commits whenever they improve
recoverability or review structure. Rebase the whole local stack onto `main@origin`, run the full
relevant gate on its final tip, and publish its parent through a named review bookmark/PR. Publishing
a completed integration unit is the default and needs no separate authorization; merging it does.
Ordinary work never moves or pushes `main`; direct integration requires explicit repository-owner
authorization. The exact workspace, Beads, rebase, and PR commands live in
[the contributor workflow](docs/contributor-workflow.md).
