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

Claim and update a concrete Bead at meaningful task boundaries. Record accepted facts,
consequential rejected hypotheses, and unresolved blockers. Bead synchronization is tracker
maintenance, not a step attached to every checkpoint, rebase, or push.

## Validate the change

Review the change and run checks whose scope can detect its failures. Typical examples are:

- recovered bodies: focused `just compare ADDRESS...` or `just compare --file SOURCE`;
- layout, inheritance, virtual, or lifecycle changes: the affected comparison bundle and relevant
  vtable or ABI checks;
- Python/tooling changes: relevant tests plus applicable lint and type checks;
- prose-only changes: review the diff.

`just verify` remains available for deliberate repository-wide auditing and shared validation
changes. It is not an integration or publication prerequisite.

Reuse successful validation while its relevant inputs remain unchanged. A new change ID, description
edit, bookmark move, or successful push does not invalidate it. After a rebase, rerun only checks
affected by incoming changes or conflict resolution. Investigate an unexplained relevant failure;
do not label it pre-existing without evidence. Reuse known baseline evidence when available, and
report uncertainty instead of creating a clean sibling solely to classify a failure.

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
