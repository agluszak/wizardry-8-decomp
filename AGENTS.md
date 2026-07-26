# Repository instructions

This is a Jujutsu (`jj`) repository for evidence-driven Wizardry 8 matching decompilation. Use
Beads (`bd`) for durable task state and `just` as the normal build, analysis, and test surface.

## Evidence and source recovery

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
- Never commit game binaries, extracted trees, Ghidra projects, or build products. They belong in
  `WIZ8_WORK_DIR` or the gitignored `build/` directory.

## Commands and validation

- Prefer repository commands: `just test`, `just build <target>`, `just compare`, `just wiz8 ...`,
  and `just ghidra ...`.
- Use `just ghidra query <program> ...` directly. The query command automatically starts and reuses
  the persistent read-only Ghidra daemon, switches it when the requested program changes, and
  recovers it after mutating Ghidra commands stop it. Do not manually manage the daemon in normal
  agent workflows; a one-shot Ghidra launch is only an automatic fallback when the daemon cannot
  be used.
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
