# Repository instructions

This is a Jujutsu repository for evidence-driven Wizardry 8 matching decompilation. Use Beads for
durable task state and `just` as the normal command surface.

## Authority and ownership

- Do not reverse engineer implementations whose source or declarations are available. Windows,
  MSVC runtime, SGP, zlib, IJG JPEG, and Info-ZIP behavior comes from the pinned source/header
  oracle. Recover first-party callers instead. SurRender is closed middleware and remains a valid
  recovery subject.
- Search before declaring anything. A function, global, class, field, vtable, import, or constant
  has one canonical declaration and one evidence identity. Extend that owner; do not add a cast,
  raw vtable call, duplicate extern, wrapper, guessed name, or parallel inventory.
- Recovered Wizardry translation units are C++ and use `.cpp`. Preserve proven original TU
  ownership and the explicit source/link order in `src/wiz8/sources.cmake`. Keep address-qualified
  template emissions separate until ownership is proved.
- Prefer typed layouts and calling conventions. Names, ABI, ownership, and behavior must be grounded
  in reviewed evidence, original images, source oracles, cross-build facts, or Ghidra observations.
  Preserve partial knowledge with positional/address-qualified names.
- Neutral machine observations and reviewed conclusions are different authorities. Canonical
  execution inputs live in `config/`, observations and reviewed facts in `evidence/`, and
  disposable reports in `build/`. Follow [the evidence policy](docs/evidence-policy.md).
- Never commit game binaries, extracted trees, live Ghidra projects, or build products. The
  validated canonical GZF seed declared by `vendor/ghidra/exports/manifest.json` is the sole
  tracked cache exception and is never fact authority.
- Preserve the SFI-SCLA and upstream notices under `third_party/sfi-sgp`. Mark modifications as
  required by the license and prefer adapting the vendored source to reconstructing it.

## Recovery loop

Start a port with the canonical program selector and the joined context packet:

```sh
CANON=wiz8--gog-base--wiz8--18a74ff61c65
just wiz8 report context 0x<address> --program "$CANON"
just ghidra query "$CANON" \
  -q 'facts-at 0x<address>' \
  -q 'decompile 0x<address>' \
  -q 'high-function 0x<address>' \
  -q 'field-accesses 0x<address> this' \
  -q 'type-variables 0x<address> this' \
  -q 'pcode 0x<address> normalize'
```

Use decompile for behavior, high-function for ABI/storage, field accesses for layout, type variables
for anonymous ownership, and normalized P-code for receiver identity and indirect flow. Use a listing
only to explain a remaining encoding/compiler-order difference.

Candidate conclusions belong in a disposable overlay:

```sh
just ghidra overlay analyze "$CANON" config/ghidra/hypotheses/<plan>.json
just ghidra overlay inspect "$CANON" <overlay-id> 0x<address>
just ghidra overlay discard "$CANON" <overlay-id>
```

Promotion goes through canonical reviewed evidence and a rebuilt baseline. The detailed lifecycle,
class, and reconstructed-transfer procedure is in
[the class recovery guide](docs/wiz8-class-recovery-procedure.md).

Keep every `// FUNCTION:` or `// TEMPLATE:` marker immediately above its declaration, with no
blank line or prose between them. Put explanatory prose above the marker. `// LIBRARY:` has no
owned definition and is exempt.

## Validation

Choose the narrowest lane while iterating and run the complete lane before publication:

| Change | Required validation |
| --- | --- |
| Python, docs, evidence, or source inventory | `just check` |
| Ordinary local work | `just test` (unit + repository invariants) |
| Ported/recovered body | `just build WIZ8_GAMEPLAY_BOUNDARIES`, `just wiz8 verify-boundaries`, then `just test` |
| Reviewed replay/evidence change | `just ghidra rebuild <program>` plus representative `facts-at`/caller queries |
| Complete local product/integration gate | `just verify` |

Relocation-masked boundary verification is the body criterion. Linked-image `compare` is diagnostic:
relocated globals reduce its score even for exact bodies. Preserve `/OPT:NOREF` comparison and
`/OPT:REF` runtime modes.

`just check` owns formatting, lint, types, unit tests, repository evidence validation, and marker
validation. Do not copy live counts into tests or Markdown; use `just wiz8 report status`.

## Workspace and publication

Give each checkout its own `WIZ8_WORK_DIR` and `WIZ8_GHIDRA_AGENT_ID`. Never hardlink a live
`ghidra/` directory. Ghidra query automatically restores, replays, validates, and reuses the
checkout-isolated project; do not manually manage daemon/cache internals.

At session start, run `bd prime`, `bd dolt pull`, inspect `bd ready`, and claim or create a Bead
before substantial work. Record partial evidence; close only after acceptance and push Beads state.

Use Jujutsu, not raw Git, for repository history. Preserve unrelated working-copy changes, inspect
`jj status` after meaningful edits, fetch/rebase before publication, advance and push `main`, then
leave a clean empty child. Pull requests remain Jujutsu-first. The exact workspace, Beads, rebase,
publication, and PR commands live in [the contributor workflow](docs/contributor-workflow.md).
