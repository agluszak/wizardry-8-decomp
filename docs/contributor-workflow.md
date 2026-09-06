# Contributor workflow

Use Jujutsu in the provided checkout. This page is a short command reference; repository policy is
in `AGENTS.md`.

## Environment ownership

Do not create another worktree, Jujutsu workspace, clone, sibling, or baseline checkout unless the
task explicitly requests one. If multiple checkouts already exist, each needs a unique absolute
`WIZ8_WORK_DIR`. Each checkout's live Ghidra project belongs at `ghidra-project/` by default and must
never be shared, copied, or hardlinked. Product builds live under that checkout's `build/decomp`.

Only one agent owns repository-state operations in a shared checkout. Other agents may inspect or do
independent work, but must not independently switch revisions, rebase, or publish that working copy.

## Start or resume

Inspect the checkout once:

```sh
jj status
```

Continue `@` when it already contains this task. Do not restart the VCS workflow when a conversation
resumes. For a genuinely new task, first preserve or account for existing work, then start from the
last observed remote main:

```sh
jj git fetch --remote origin
jj new main@origin -m "Recover W8DialogInterface"
```

Keep one mutable change per coherent task by default. Bookmarks are unnecessary during ordinary
work. Split changes only when it materially improves review or recovery. Give the completed change
an accurate description when needed:

```sh
jj describe -m "Recover W8DialogInterface"
```

Do not create an empty child merely to imitate `git commit`.

## Validate the change

Follow the verification policy in `AGENTS.md`. After a rebase, rerun only checks affected by incoming
changes or conflict resolution.

## Publish directly to main

Direct integration requires explicit authorization and completed work:

```sh
jj git fetch --remote origin
jj rebase --destination main@origin

# Resolve conflicts, review the completed change, and run missing or invalidated checks.

jj bookmark set main -r @
jj git push --remote origin --bookmark main
```

The ordinary rebase selects the current branch. Never rewrite remote `main`. If the push is rejected,
fetch and inspect the actual competing change, integrate it, rerun affected checks, and retry.

A successful push completes publication. Do not fetch again, recreate an empty child, repeat
validation, or perform routine bookmark/tree-equivalence proofs.

## Publish for pull request or checkpoint

Leave `main` alone and let jj create a publication bookmark:

```sh
jj git push --remote origin --change @
```

Use the bookmark name reported by jj when opening the pull request. A checkpoint or draft PR does not
require an integration rebase or broad validation merely to upload work. Merge only when authorized.
