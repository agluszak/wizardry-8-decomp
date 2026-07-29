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
duplicating scope. Record decisions with `bd note`. When all acceptance criteria and gates pass:

```sh
bd close <id> --reason="<verified outcome>"
bd dolt push
```

## Jujutsu publication

Start or resume from integrated main:

```sh
jj workspace update-stale       # only if Jujutsu reports a stale workspace
jj git fetch --remote origin
jj status
jj log -r '@ | main | main@origin'
jj new main                     # only if @ is not already the intended empty child
```

Complete one coherent unit:

```sh
jj diff --stat
jj diff
just check
just test                       # plus the change-specific gate
just lint                       # for C++ class or inheritance changes
jj describe -m "Accurate imperative summary"
jj git fetch --remote origin
jj rebase -s @ -d main
jj bookmark set main -r @
jj git push --remote origin --bookmark main
bd dolt push
jj new main
jj status
```

If `main` advanced, inspect and integrate it; never erase another contributor's work. Do not push
an unnamed change or use `--allow-empty-description`.

For an explicitly requested pull request, describe/fetch/rebase first, then create and push a review
bookmark:

```sh
jj bookmark create agent/<topic> -r @
jj git push --remote origin --bookmark agent/<topic>
gh pr create --base main --head agent/<topic> ...
```

Merge only when authorized. After GitHub merges, fetch into Jujutsu and prove the reviewed commit is
an ancestor and tree-equivalent:

```sh
jj git fetch --remote origin
jj log -r '@::main'
jj diff --from @ --to main --stat
jj new main
jj status
```
