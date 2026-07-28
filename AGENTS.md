# Repository instructions

This is a Jujutsu (`jj`) repository for evidence-driven Wizardry 8 matching decompilation. Use
Beads (`bd`) for durable task state and `just` as the normal build, analysis, and test surface.

## Evidence and source recovery

- **Do not reverse engineer code we already have.** Windows and its DLLs, the MSVC runtime, SGP,
  zlib, IJG JPEG and Info-ZIP are off limits as subjects of recovery, because their sources or
  headers are already available to us - vendored, pinned, or published. Their behavior, constants,
  structures and calling conventions come from those, not from decompiling their bodies. Include
  `<windows.h>` and use `FILE_ATTRIBUTE_DIRECTORY`; do not read 0x004054F0 to work out what a bit
  means, and do not restate a constant that a header already defines. Recovering first-party code
  that *calls* one of these is the work; that end of the call is a declaration. When an oracle and
  the image appear to disagree about one, that is a fact to record and route to the owning track,
  not a licence to reconstruct it.
- **SurRender is not in that list.** It is closed middleware with no source and no published
  headers, so it is a legitimate subject of recovery like any first-party code, and
  `include/surrender/` is reconstructed rather than vendored. The same evidence rules apply: names
  and layouts come from the export table, the assertions, and the image, and anything unproven says
  so. Its inline bodies land in whatever links them, which is why some are owned under
  `surrender-template` in the Wiz8 boundary map rather than in a SurRender target of their own.
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
  `Wiz8.pdb`. `just configure` and `just compare` still refuse to run when a build directory's cache
  names a different checkout, which catches a moved or copied working copy; `just wiz8
  check-build-dir` reports it on demand.
- **Give every concurrently active checkout its own `WIZ8_WORK_DIR`.** Sharing one is safe for the
  build, which is isolated by the rule above, but not for Ghidra. Agents materializing at once
  serialize on that directory's `materialize.lock`, and because the project is content-addressed
  over the reviewed evidence, every evidence commit another agent lands invalidates the cached
  project and forces a fresh GZF restore. With several agents landing evidence a few minutes apart
  the effect is not a slow query but one that never returns. `WIZ8_GHIDRA_AGENT_ID` separates the
  project directories; it does not separate the lock or the cache key.
- Build a private work directory the way `.env.example` describes, with `cp -al` of the immutable
  input trees only - `extracted`, `fid`, `variants`, `oracles`, `sgp`. Those are hardlinks, so the
  copy costs no disk. **Do not hardlink `ghidra/`**: it is a live Ghidra project that Ghidra writes
  in place, and sharing its inodes corrupts the original. Let it, `ghidra-agents/` and `daemon/` be
  rebuilt - restoring the canonical GZF is what the tooling already does on first use.
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
- The materialization key covers the reviewed evidence and the replay modules only. Editing the
  daemon, the cache or another transport no longer invalidates it; editing an `apply_*` module or an
  evidence CSV still does, and should. An unclassified module under `tools/wiz8decomp/ghidra/` feeds
  the key by default, so the cost of overlooking one is a needless rebuild rather than a stale
  program.
- Materialized Ghidra projects are disposable and are pruned to the newest three per agent
  (`WIZ8_GHIDRA_KEEP_MATERIALIZATIONS`). The materialization key covers the reviewed evidence, so
  recording an identity or a layout invalidates the cache and rebuilds a 51MB project; without
  eviction one session left 2.4GB. `just ghidra cache prune` clears the backlog for this agent only -
  another agent's projects are not ours to remove and one may be open.
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
- **Start every port from `just wiz8 report context 0x<addr> --program <program>`.** The context
  packet joins reviewed provenance, HighFunction variables, rooted field accesses, anonymous type
  variables, normalized P-code, indirect call sites, reconstructed signatures, cross-build
  candidates and object-map candidates with the older assertion/EH/global channels. Follow it with
  one batched semantic query when a relation needs closer inspection:
  `facts-at`, `decompile`, `high-function`, `field-accesses`, `type-variables`, `pcode ... normalize`
  and `callsite` answer different questions and should not be substituted for one another. Ghidra
  already carries the applied types, global names, and callee identities, so its output names
  `g_fact_values`, `FileWrite` and `W8NpcDatabaseRecord` where a raw listing shows only addresses.
  Reading instructions by hand to work out what a function *does* re-derives, badly and slowly, what
  the project has already recorded. Disassembly answers a different and narrower question — why two
  encodings differ — and `just wiz8 diff-boundary <symbol>` is the tool for that, aligned against the
  original with relocations and moved branch displacements already discounted.
  Pass the full program selector (`wiz8` alone is ambiguous across 21 programs; use
  `wiz8--gog-base--wiz8--18a74ff61c65` for canonical retail). Query results are emitted as ordinary
  JSON rather than through Rich, so they remain parseable and unwrapped regardless of terminal width.
- Candidate conclusions belong in disposable Ghidra overlays, not tracked projections. Put the
  hypothesis scope in a JSON file under `config/ghidra/hypotheses/`, then run
  `just ghidra overlay analyze <program> <plan>`. The driver clones the reviewed materialization,
  applies requested typed vtables, reconstructed information and aggregates, materializes anonymous
  structures and owner fields, adds computed indirect references, expands one ProgramDB-derived
  dependency graph, and repeats until no type or edge changes. Its semantic verdict distinguishes
  presentation, prototype, field, target-set, unification, contradiction and no-change results.
  Inspect candidate provenance with `just ghidra overlay facts-at <program> <hypothesis> <anchor>`;
  discard the overlay after review. Promotion still goes through reviewed evidence and a baseline
  rebuild.

### Recommended class-port workflow

Use the inference tooling as a loop, not as a collection of terminal reports. For canonical retail,
keep the full selector visible in the shell and start from the owner or lifecycle address:

```sh
CANON=wiz8--gog-base--wiz8--18a74ff61c65
just wiz8 report context 0x004b6e00 --program "$CANON"
just ghidra query "$CANON" \
  -q 'facts-at 0x004b6e00' \
  -q 'decompile 0x004b6e00' \
  -q 'high-function 0x004b6e00' \
  -q 'field-accesses 0x004b6e00 this' \
  -q 'type-variables 0x004b6e00 this' \
  -q 'pcode 0x004b6e00 normalize'
```

- Prefer a coherent lifecycle slice: allocation site, constructor, one behavior method and
  destructor. Constructors usually establish size, argument widths and most field offsets at once;
  destructors establish ownership and destructor kind. Follow actual Ghidra containment and P-code
  data flow when a snapshot attributes a writer or caller differently.
- Read `decompile` for behavior, `high-function` for the current ABI and storage, `field-accesses`
  for layout, `type-variables` for owned anonymous pointers, and normalized P-code for receiver
  identity, aliasing, branches and indirect calls. Use `listing` only after the semantic model has
  answered what the code does and an encoding or compiler-order question remains.
- Preserve partial knowledge. Give unresolved classes and members address-qualified positional
  names; do not convert a shape match or a large candidate pool into an original identity. Promote
  semantic names only from assertions, exports, source/debug evidence or another independent
  witness.
- Model the ABI directly in C++ before chasing bytes: real `__thiscall` methods, receiver types,
  parameter widths, base/subobject layout and ownership. Do not replace them with casts, raw vtable
  indexing or nullary placeholders. A relocation-masked exact compile then proves that typed model;
  it does not retroactively prove descriptive parameter or member names.
- Exercise the hypothesis in a disposable overlay before reviewing it. `analyze` consumes the plan
  path and iterates type constraints and indirect edges to a fixpoint; its result supplies the
  generated hypothesis name used by the other actions:

  ```sh
  just ghidra overlay analyze "$CANON" config/ghidra/hypotheses/<plan>.json
  just ghidra overlay facts-at "$CANON" <hypothesis> 0x004b6e00
  just ghidra overlay decompile "$CANON" <hypothesis> 0x0044dea0
  just ghidra overlay discard "$CANON" <hypothesis>
  ```

  Inspect semantic changes, not merely changed C text: a useful result resolves a field, improves a
  prototype, unifies a type variable or narrows/adds an indirect edge without contradiction. Record
  the baseline fingerprint before creation and confirm it after discard when changing overlay code.
- Once the source is typed, iterate with `just build WIZ8_GAMEPLAY_BOUNDARIES` and
  `just wiz8 diff-boundary <symbol>`. Promote only after `just verify-boundaries` reports the body
  exact and the recorded digest is fresh. Update the canonical class, field, function, signature and
  function-evidence rows that the new proof actually supports, rebuild the materialization, and end
  with `facts-at` plus representative caller decompilations. Run `just test` before publication.
- Recommended priorities are, in order: close a decisive anonymous owner into a reviewed class;
  recover its lifecycle; type call receivers and vtable slots from the strongest available
  prototype; then expand to consumers through the ProgramDB dependency graph. Avoid adding another
  census when the next class method can exercise and improve the closure loop instead.

- Reconstructed transfers have a hard body gate. Run `just build WIZ8_GAMEPLAY_BOUNDARIES` and
  `just verify-boundaries` first; `reconstructed-transfer` also recomputes current relocation-masked
  object digests and keeps an unverified exact row overlay-only. Exact bytes establish calling
  convention and stack argument shape. Reconstructed parameter/return spellings remain candidate
  semantic types, and reconstructed parameter names are source labels rather than original-name
  evidence.
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

**The workflow is Jujutsu-first even when the user explicitly requests a pull request.** Finish and
describe the change in `jj`, fetch and rebase it onto current `main`, create its review bookmark with
`jj bookmark`, and push that bookmark with `jj git push` before invoking any GitHub PR operation.
GitHub is then only the review/merge layer over an already coherent Jujutsu commit. Do not begin by
creating a Git branch, and do not use `git status`, `git add`, `git commit`, `git checkout`, or
`git push` in this repository.

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

When a PR is explicitly requested, replace the direct `main` bookmark push above with this ordering:

```sh
jj describe -m "Accurate imperative summary"
jj git fetch --remote origin
jj rebase -s @ -d main                 # only when main advanced
jj bookmark create agent/<topic> -r @  # use `set` when resuming an existing bookmark
jj git push --remote origin --bookmark agent/<topic>
gh pr create --base main --head agent/<topic> ...
gh pr merge <number> ...               # only when the user also authorized merge
jj git fetch --remote origin
jj log -r '@::main'                    # the reviewed commit must be an ancestor
jj diff --from @ --to main --stat      # the merge must not alter its tree
jj new main
jj status
```

Do not open the PR before the `jj` commit, integration check, bookmark, and push exist. After GitHub
merges it, treat the remote result as external history: fetch it into Jujutsu, verify ancestry and an
empty tree diff, then move to a clean empty child of the updated `main`. Do not recreate or amend the
merged change through GitHub tooling.

- Every completed coherent unit must be described, committed by advancing `main`, and pushed.
- Fetch immediately before moving `main`; Jujutsu's push lease protects against an unseen remote
  update, but local concurrent changes still require inspection and integration.
- Do not push an unnamed or empty-description change. Do not use `--allow-empty-description`.
- After pushing, leave a clean empty working-copy change on `main` for the next task and verify both
  the repository and Beads remotes succeeded.
