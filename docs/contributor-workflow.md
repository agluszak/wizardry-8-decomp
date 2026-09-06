# Contributor workspace and publication workflow

The root instructions contain policy; this page contains the repeatable machine procedure.

## Checkout setup

Use the provided checkout. Do not create a worktree, Jujutsu workspace, clone, or baseline checkout
unless the task explicitly requires one. Copy `.env.example` to `.env` when configuring a new
provided checkout. If multiple checkouts already exist, each needs a unique absolute `WIZ8_WORK_DIR`.

The live Ghidra project lives inside the checkout at `ghidra-project/` (gitignored), so
every workspace owns its project by construction. `WIZ8_GHIDRA_PROJECT_DIR` relocates one
checkout's project; it must never point two checkouts at the same directory, and the
tooling refuses a project restored by a different checkout. Never hardlink or copy a live
project; Ghidra writes it in place. A checkout that previously kept its project at
`$WIZ8_WORK_DIR/ghidra` should `mv` that directory to `<checkout>/ghidra-project` (or set
`WIZ8_GHIDRA_PROJECT_DIR` to the old path) to keep analysis state past the reviewed
checkpoint.

The product CMake directory is `build/decomp` inside each checkout. `wiz8 build` configures it
automatically and refuses a cache owned by another checkout.
Product configure/link operations also take a non-blocking per-checkout lock and report the
holder when another build is active.

## Beads

```sh
bd prime
bd dolt pull
bd ready
bd show <id>
bd update <id> --claim
```

Create a concrete Bead before substantial implementation and link dependencies instead of
duplicating scope. Record accepted facts, consequential rejected hypotheses, and unresolved blockers.
Batch related experiments into one concise note; include commands and detailed results only when a
non-obvious conclusion needs to be reproduced. When the task acceptance criteria pass:

```sh
bd close <id> --reason="<verified outcome>"
bd dolt push
```

## Start a task stack

Fetch once, inspect the shared bookmark state, and create a unique task bookmark from integrated
`main`. Include the Bead ID so concurrent agents cannot choose the same name.

```sh
jj workspace update-stale       # only if Jujutsu reports a stale workspace
jj git fetch --remote origin
jj status
jj log -r '@ | main | main@origin'
jj new main                     # only if @ is not already the intended empty child
jj bookmark create agent/<bead>-<topic> -r @
jj describe -m "<task summary>"
```

Keep one mutable working-copy commit for the task by default. Split it only when separate commits
materially improve review or recovery. A task may contain several coherent commits, but all of them
remain under the task bookmark until the whole Bead is complete. Do not move `main` after each
commit.

When creating another commit in the same stack, move the task bookmark to the new stack tip before
pushing it:

```sh
jj new
jj describe -m "<next coherent change>"
# edit and validate
jj bookmark set agent/<bead>-<topic> -r @
```

## Local completion and checkpoint publication

Use the change-specific verification policy in `AGENTS.md`. A completed result remains valid until a
relevant input changes. Review the final diff before publication.

```sh
jj status
jj diff --stat
jj diff
```

Push the task bookmark when a remote checkpoint or review is useful. This is not integration and
must not move `main`:

```sh
jj describe -m "Accurate imperative summary"
jj bookmark set agent/<bead>-<topic> -r @
jj git push --remote origin --bookmark agent/<bead>-<topic>
bd dolt push
```

Other agents may advance `main` repeatedly while the task is in progress. Do not chase each movement
with a rebase. Fetch and rebase when upstream evidence is needed, when a real conflict must be
resolved, and immediately before final integration.

To refresh the whole task stack, not merely its tip:

```sh
jj git fetch --remote origin
jj log -r 'main | main@origin | agent/<bead>-<topic>'
jj rebase -s 'roots(main..agent/<bead>-<topic>)' -d main
jj bookmark set agent/<bead>-<topic> -r @
```

Inspect the incoming commits before resolving conflicts. Never force or overwrite a concurrent
bookmark movement.

## Integration preparation

Local completion, checkpoint publication, PR publication, and integration are separate operations.
Do not integrate merely because implementation is complete. Before requested integration, fetch and
rebase the task stack, inspect what changed, and run `just verify` once on the rebased tip. Add only
focused comparisons that `just verify` does not provide.

```sh
jj git fetch --remote origin
jj rebase -s 'roots(main..agent/<bead>-<topic>)' -d main
jj bookmark set agent/<bead>-<topic> -r @
jj diff --stat
jj diff
just verify
```

For a failing broad gate, use an existing baseline only when its base commit and relevant inputs are
known. Otherwise report the failure as unclassified. Do not create another checkout merely to classify
it, and do not call it pre-existing without evidence. Do not integrate with an unresolved blocking
failure; a local handoff may still contain completed work and a clearly stated blocker.

If `main` advances after this rebase, inspect the new commits and rebase again. Rerun only checks whose
inputs or affected scope changed.

## Integrate once

For authorized direct integration, move `main` only after integration verification passes. Do not
repeat unchanged constituent checks during the publication sequence:

```sh
jj git fetch --remote origin
jj rebase -s 'roots(main..agent/<bead>-<topic>)' -d main
jj bookmark set agent/<bead>-<topic> -r @
jj bookmark set main -r agent/<bead>-<topic>
jj git push --remote origin --bookmark main
bd dolt push
```

If the push is rejected or local `main` changes during the sequence, stop. Fetch, inspect the other
workspace's work, rebase again, and rerun the affected gates. Never force-push `main`.

After a successful push, prove publication rather than trusting the push message:

```sh
jj git fetch --remote origin
jj log -r 'main | main@origin | agent/<bead>-<topic>'
jj diff --from main@origin --to agent/<bead>-<topic> --stat
jj new main
jj status
```

Local `main` and `main@origin` must resolve to the published commit, and the task bookmark must have
an empty tree diff against it.

## Pull-request publication

When a pull request is explicitly requested, fetch and rebase the stack, run the verification needed
for review, then push the task bookmark and open the PR:

```sh
jj bookmark set agent/<bead>-<topic> -r @
jj git push --remote origin --bookmark agent/<bead>-<topic>
gh pr create --base main --head agent/<bead>-<topic> ...
```

Merge only when authorized. After GitHub merges, fetch into Jujutsu and prove the reviewed task is
an ancestor and tree-equivalent:

```sh
jj git fetch --remote origin
jj log -r 'agent/<bead>-<topic>::main'
jj diff --from agent/<bead>-<topic> --to main --stat
jj new main
jj status
```
