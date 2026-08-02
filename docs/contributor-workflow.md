# Contributor workspace and publication workflow

The root instructions contain policy; this page contains the repeatable machine procedure.

## Isolated workspace setup

Copy `.env.example` to `.env`. Every checkout needs a unique absolute `WIZ8_WORK_DIR`.
A private work directory may use `cp -al` for immutable `extracted`, `fid`, `variants`,
`oracles`, and `sgp` trees.

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
duplicating scope. Record decisions and negative compiler experiments with `bd note`. A useful
negative note names the symbol/address, hypothesis, source shape, comparison command, before/after
result, and conclusion. When all acceptance criteria and gates pass:

```sh
bd close <id> --reason="<verified outcome>"
bd dolt push
```

## Start an isolated task stack

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

## Recovering C++ from the reviewed project

`uv run wiz8 recover explain 0x<address>` reports structured recovery facts, while
`uv run wiz8 recover regress 0x<address>...` measures how much of an already-recovered body the
headless recovery engine regenerates with zero manual edits. Recovery opens the reviewed Ghidra
project read-only and receives source-index facts transiently; it does not generate class,
translation-unit, or global-data source products.
`just recover 0x<address>` drafts a new function into its owning translation unit: source-owner
lookup and address-order insertion from the source index, duplicate refusal, a focused build with
`compare`/`triage`, one structurally recovered definition, and diagnostics
with include suggestions on failure. Constructor placement alternatives use the source index's
actual field inventory; other structured differences decline until reccmp identifies the affected
source declaration or region.
The default previews and restores the tree; `--apply` writes
it. All of these need the live Ghidra project; the recover/regress loops also need the pinned VC6
toolchain.

## Work and checkpoint publication

Use the narrow change-specific gates while iterating. Inspect every meaningful source change and
record compiler-falsified hypotheses rather than silently forgetting them.

```sh
jj status
jj diff --stat
jj diff
just test                       # plus the narrow change-specific gate
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

## Final validation

Once the Bead scope is complete, perform one final fetch and stack rebase. Run the complete
applicable lane on the rebased tip, not on a pre-rebase commit:

```sh
jj git fetch --remote origin
jj rebase -s 'roots(main..agent/<bead>-<topic>)' -d main
jj bookmark set agent/<bead>-<topic> -r @
jj diff --stat
jj diff
just check
just test                       # plus focused compare/triage and other change-specific gates
just lint                       # for C++ class or inheritance changes
just verify                     # integration gate
```

When `just verify` fails, do not merely label the output pre-existing. Reproduce the same command in
a clean sibling workspace at the exact `main` commit used for the rebase. All narrower required
gates must pass, and the topic may proceed only when the baseline and topic failure identities have
an empty delta. Record the base commit, both failure sets, and the delta in the Bead and handoff.

If `main` advances after this rebase, inspect the new commits, rebase the whole task stack again, and
rerun every gate that the rebase could affect.

## Integrate once

For authorized direct integration, move `main` only after the complete task has passed final
validation. Treat fetch, rebase, validation, bookmark movement, and push as one controlled sequence:

```sh
jj git fetch --remote origin
jj rebase -s 'roots(main..agent/<bead>-<topic>)' -d main
jj bookmark set agent/<bead>-<topic> -r @
# rerun the final applicable gates here
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

When a pull request is explicitly requested, perform the same final fetch, whole-stack rebase, and
validation first. Then push the task bookmark and open the PR:

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
