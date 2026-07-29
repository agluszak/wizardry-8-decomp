# Contributor workspace and publication workflow

The root instructions contain policy; this page contains the repeatable machine procedure.

## Isolated workspace setup

Copy `.env.example` to `.env`. Every checkout needs a unique absolute `WIZ8_WORK_DIR`.
A private work directory may use `cp -al` for immutable `extracted`, `fid`, `variants`,
`oracles`, and `sgp` trees. Never hardlink `ghidra/`; Ghidra writes the canonical local
project in place.

The product CMake directory is `build/decomp` inside each checkout. `wiz8 build` configures it
automatically and refuses a cache owned by another checkout.
Product configure/link operations also take a non-blocking per-checkout lock and report the
holder when another build is active.

## Beads

A Bead is the durable outcome, ownership, dependency, and acceptance-criteria unit. It is
independent of Jujutsu history: one Bead may contain several local commits and normally produces
one review bookmark/PR. A large outcome may instead have independently landable child Beads and
several PRs.

```sh
bd prime
bd dolt pull
bd ready
bd show <id>
bd update <id> --claim
bd dolt push                 # publish ownership immediately
```

Create a concrete Bead before substantial implementation and link dependencies instead of
duplicating scope. Do not create another Bead for incidental work required by the existing
acceptance criteria. Record decisions with `bd note` and push after meaningful dependency,
ownership, scope, review, or status changes. A local commit alone is not a Beads synchronization
event.

When a review is published, keep the Bead claimed and record it:

```sh
bd note <id> "Review: PR #<number>; required local gates pass"
bd dolt push
```

Close it only after the change is integrated into `main` and its acceptance criteria remain
satisfied:

```sh
bd close <id> --reason="Merged in PR #<number>; acceptance criteria verified"
bd dolt push
```

## Jujutsu checkpoints and review publication

Start a new task from the unambiguous remote integration point:

```sh
jj workspace update-stale       # only if Jujutsu reports a stale workspace
jj git fetch --remote origin
jj status
jj new main@origin
```

During implementation, use narrow gates and make local commits whenever they improve recoverability
or review structure. `jj commit` describes the current change and creates a new empty working-copy
commit, so a Bead can accumulate a clean stack:

```sh
just <narrow-relevant-gate>
jj diff --stat
jj diff
jj commit -m "Recover registry identity accessors"

# Continue the same Bead in the new empty working-copy commit.
just <narrow-relevant-gate>
jj commit -m "Type the associated registry globals"
```

Checkpoint commits may be squashed, split, reordered, or rewritten before review. They do not move
`main`, do not require a Beads push, and do not each require the complete publication gate.

Before review, fetch and rebase the whole branch containing `@`. The explicit `-b @` form includes
the stack's ancestors that are not ancestors of `main@origin`; `-s @` would select only `@` and its
descendants and is therefore wrong for a multi-commit stack:

```sh
jj git fetch --remote origin
jj rebase -b @ -o main@origin
jj status
jj log -r 'main@origin..@'
jj diff --from main@origin --to @
```

Inspect conflicts and every upstream change incorporated by the rebase; never erase another
contributor's work. Run the complete relevant gate on the final review tip:

```sh
just check
just test                       # plus build, boundary, runtime, Ghidra, or integration gates
```

Require an empty `@`, then publish its parent as the review tip. The bookmark, not `main`, is the
ordinary publication target:

```sh
jj status                       # must report an empty working-copy commit
jj bookmark create agent/<bead-id>-<topic> -r @-
jj git push --remote origin --bookmark agent/<bead-id>-<topic>
gh pr create --base main --head agent/<bead-id>-<topic> --title "..." --body "..."
```

Do not push an unnamed or empty-description change or use `--allow-empty-description`. For later
review updates, rebase the whole stack again, rerun affected gates, and move the existing bookmark:

```sh
jj git fetch --remote origin
jj rebase -b @ -o main@origin
jj bookmark move agent/<bead-id>-<topic> --to @-
jj git push --remote origin --bookmark agent/<bead-id>-<topic>
```

Merge only when explicitly authorized. After GitHub merges, fetch into Jujutsu, prove the reviewed
tip is an ancestor of remote main and tree-equivalent, then start the next empty change from the
remote integration point:

```sh
jj git fetch --remote origin
jj log -r '@-::main@origin'
jj diff --from @- --to main@origin --stat
jj new main@origin
jj status
```

Ordinary agent work must not move or push `main`. Direct integration is an exception requiring
explicit repository-owner authorization, such as an urgent repair of broken `main`. GitHub should
also enforce this with a ruleset requiring PRs and public checks for `main`, disallowing force pushes
and deletion, and retaining an owner bypass for exceptional integration. Ruleset administration is
external owner state, not part of the ordinary contributor procedure.
