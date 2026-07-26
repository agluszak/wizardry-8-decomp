# Repository instructions

This is a Jujutsu (`jj`) repository for evidence-driven Wizardry 8 matching decompilation. Use
Beads (`bd`) for durable task state and `just` as the normal build, analysis, and test surface.

## Evidence and source recovery

- **Do not reverse engineer library code.** Windows and its DLLs, the MSVC runtime, SGP, zlib, IJG
  JPEG, Info-ZIP, SurRender and anything else the game links rather than authors are off limits as
  subjects of recovery. Their behavior, constants, structures and calling conventions come from the
  real headers, the vendored or pinned sources, and the published documentation - not from
  decompiling their bodies. Include `<windows.h>` and use `FILE_ATTRIBUTE_DIRECTORY`; do not read
  0x004054F0 to work out what a bit means, and do not restate a constant that a header already
  defines. Recovering first-party code that *calls* a library is the work; the library end of the
  call is a declaration. When an oracle and the image appear to disagree about a library, that is a
  fact to record and route to the owning track, not a licence to reconstruct the library.
- Ground names, types, ownership, and behavior in the checked-in evidence, original binaries,
  source oracles, cross-build comparison, and Ghidra observations. Do not guess names or ABI.
- Prefer real typed interfaces and recovered layouts over casts, raw vtable indexing, wrappers, or
  placeholders shaped only to make a test pass.
- Keep generated machine observations separate from reviewed conclusions and accepted identities.
  Regenerate owned outputs after changing their producer; unexplained generated churn is a reason
  to stop and inspect.
- Follow `docs/evidence-policy.md`: `config/` contains execution inputs, canonical observations and
  reviewed conclusions live under `evidence/`, and reproducible reports live under gitignored
  `build/`. Do not add a tracked projection, live count, or exhaustive Markdown inventory when the
  same fact already has a canonical home. Generated outputs that require unavailable proprietary
  inputs need the policy's explicit snapshot exception.
- Use the single SGP project flag hypothesis in `config/sgp.yml`. Do not infer
  per-translation-unit flags from sparse matches; reconsider the project profile only against a
  larger recovered corpus.
- SGP is vendored under `third_party/sfi-sgp` under the non-commercial SFI-SCLA. Preserve its
  verbatim license. Do not remove upstream notices; prominently mark modified licensed files with
  the change and date. Prefer adapting this source over independently reconstructing identical SGP
  code from the binary.
- Never commit game binaries, extracted trees, live Ghidra projects, or build products. They belong
  in `WIZ8_WORK_DIR` or the gitignored `build/` directory. The single canonical Wiz8 GZF declared by
  `vendor/ghidra/exports/manifest.json` is the explicit exception: it is a tracked, validated,
  disposable materialization seed, never the authority for an accepted fact.

## Commands and validation

- Prefer repository commands: `just test`, `just build <target>`, `just compare`, `just wiz8 ...`,
  and `just ghidra ...`.
- The CMake build directory is `build/decomp` **inside the checkout**, so each working copy builds
  and compares in isolation and cannot overwrite another's CMake cache or linked `Wiz8.exe`/
  `Wiz8.pdb`. `WIZ8_WORK_DIR` holds only large generated data - extracted installers, variants,
  FID sources, oracles, Ghidra projects - and may be shared between checkouts. `just configure` and
  `just compare` still refuse to run when a build directory's cache names a different checkout,
  which catches a moved or copied working copy; `just wiz8 check-build-dir` reports it on demand.
- **Keep the `// FUNCTION:` marker immediately above its declaration** - no blank line and no
  comment between them. reccmp binds a marker to whatever declaration follows it, so prose in the
  gap detaches the marker from its function rather than merely reading untidily. Put the prose above
  the marker. `just check-markers` gates this, and also that no address is claimed twice within a
  module. `// LIBRARY:` markers are exempt from the adjacency rule, having no owned definition to
  sit against.
- **Verify a ported body with `just verify-boundaries`.** It masks relocated operands and compares
  against the original image, which is the criterion `relocation_masked_sha256` records, and it
  reports `regressed` when a reviewed-exact body stops matching and `promotable` when a near miss
  starts matching.
- **Do not read `just compare <target>` as a matching criterion.** reccmp diffs the linked image,
  where our globals sit at different addresses than the original, so every relocated operand counts
  as a difference and byte-exact bodies score well under 100% — `AddLinesToMessageBox` is
  byte-identical and reports 75%. Run it for link-level faults reccmp alone can see (wrong import
  names, unreachable functions, stale links) and for whole-image progress, never to choose between
  two candidate bodies. Delete `Wiz8.exe`/`Wiz8.pdb` and rebuild before reading any reccmp number,
  because a successful `just build` does not guarantee the link step reran.
- **Set `WIZ8_GHIDRA_AGENT_ID` in every checkout's `.env`.** It separates this checkout's Ghidra
  project from every other agent's, and it has no default: two checkouts that leave it unset share
  one per-agent project root and collide on its lock, which fails as `LockException: Unable to lock
  project`. Anything unique works; the checkout's directory name is the obvious choice.
- Use `just ghidra query <program> ...` directly. On first use, it automatically restores the
  validated canonical GZF, replays current reviewed evidence, validates it, and gives the agent an
  isolated content-addressed project. It then starts and reuses that agent's persistent read-only
  daemon. Do not manually manage the cache or daemon in normal workflows; one-shot Ghidra is only
  an automatic fallback when the daemon cannot be used. Use `just ghidra rebuild <program>` only as
  the slow fresh-import parity gate, and `just ghidra cache build` after accepting a refreshed seed.
- **Wizardry 8 is a C++ program: recovered first-party units are `.cpp`, not `.c`.** Every original
  translation unit the assertions name is a `.cpp` file - `GameplayDatabase.cpp`, `IList.cpp`,
  `PC Item.cpp`, `Targeting.cpp` - so owned sources under `src/wiz8/` must be C++ too. C compiles the
  same bodies for simple code, which makes the mistake easy to miss and wrong anyway: it models the
  original as something it is not, and it cannot express a `__thiscall` virtual call at all. Only
  genuine C library code, such as the zlib wrappers, stays `.c`.
- **Start every port from `just ghidra query <program> decompile 0x<addr>`, not from disassembly.**
  Ghidra already carries the applied types, global names, and callee identities, so its output names
  `g_fact_values`, `FileWrite` and `W8NpcDatabaseRecord` where a raw listing shows only addresses.
  Reading instructions by hand to work out what a function *does* re-derives, badly and slowly, what
  the project has already recorded. Disassembly answers a different and narrower question — why two
  encodings differ — and `just wiz8 diff-boundary <symbol>` is the tool for that, aligned against the
  original with relocations and moved branch displacements already discounted.
  Pass the full program selector (`wiz8` alone is ambiguous across 21 programs; use
  `wiz8--gog-base--wiz8--18a74ff61c65` for canonical retail). Query results are emitted as ordinary
  JSON rather than through Rich, so they remain parseable and unwrapped regardless of terminal width.
- Run the narrowest relevant checks while iterating, then the complete relevant gate before
  publishing. A successful build alone does not prove identity or behavior.
- Use `just wiz8 report status` for current identity, match, ownership, and source-unit counts; do
  not copy live totals into Markdown.
- Preserve unrelated changes in the shared workspace. Inspect `jj status` before and after work,
  and stage nothing implicitly because Jujutsu snapshots the whole working copy. In a shared
  checkout, run `jj status` immediately after a meaningful edit batch so a concurrent workspace
  operation cannot replace unsnapshotted files.

## Beads workflow

Beads is the durable task tracker and has its own Dolt history and remote; it is not a generated
CSV/JSONL side channel in this repository.

At the start of a session or after context loss:

```sh
bd prime
bd dolt pull
bd ready
bd show <id>
bd update <id> --claim
```

- Find or create a Bead before substantial implementation. Use `bd create` with a concrete
  description and acceptance criteria; link dependencies rather than duplicating work.
- Record decisions and partial evidence with `bd note <id> ...`. Keep a Bead open when only part of
  its acceptance criteria is proven.
- Close with `bd close <id> --reason="..."` only after the implementation and required validation
  are complete. Then run `bd dolt push` so tracker state is not stranded locally.
- Do not recreate or track `.beads/interactions.jsonl`; interaction auditing is intentionally
  excluded.

## Jujutsu integration and publication

Use `jj`, not raw Git, for repository history, bookmarks, synchronization, and publication. Raw
Git is acceptable only inside explicitly external source-oracle directories where a command
requires it.

Starting or resuming work:

```sh
jj workspace update-stale       # only when jj reports a stale shared workspace
jj git fetch --remote origin
jj status
jj log -r '@ | main | main@origin'
jj new main                     # start an empty change on the integrated local main
```

If `@` is already the intended empty child of `main`, do not create another. If another agent moves
local `main` while work is in progress, inspect the new commits and integrate them before
publication with `jj rebase -s @ -d main`. Resolve conflicts, regenerate owned outputs when
necessary, and rerun affected gates. Never erase or rewrite another agent's work to simplify
integration.

Completing each coherent unit of work:

```sh
jj diff --stat
jj diff
just test                       # plus target-specific build/compare/gates
jj describe -m "Accurate imperative summary"
jj git fetch --remote origin
jj rebase -s @ -d main          # when main advanced since this change began
jj bookmark set main -r @
jj git push --remote origin --bookmark main
bd dolt push
jj new main
jj status
```

- Every completed coherent unit must be described, committed by advancing `main`, and pushed.
- Fetch immediately before moving `main`; Jujutsu's push lease protects against an unseen remote
  update, but local concurrent changes still require inspection and integration.
- Do not push an unnamed or empty-description change. Do not use `--allow-empty-description`.
- After pushing, leave a clean empty working-copy change on `main` for the next task and verify both
  the repository and Beads remotes succeeded.
