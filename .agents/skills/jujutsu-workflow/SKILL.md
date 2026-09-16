---
name: jujutsu-workflow
description: Manage Wizardry 8 repository state with Jujutsu, including starting/resuming work, rebasing, conflict resolution, existing PR branches, and publication.
---

# Jujutsu workflow

Use this skill whenever the task changes repository state: starting a task, switching to an existing
change/PR, rebasing, resolving conflicts, squashing, setting bookmarks, or publishing. Recovery and
verification policy remains in `AGENTS.md`; this skill owns the VCS mechanics.

## Checkout ownership

Use the provided checkout. Do not create another worktree, Jujutsu workspace, clone, sibling checkout,
or baseline copy unless the task explicitly requires one. One agent owns revision switching, rebasing,
bookmark movement, and publication for a shared checkout.

Use Jujutsu for history mutation. In a colocated repository, `git show`, `git diff`, and other read-only
Git inspection are fine when they are the cheaper way to inspect immutable historical commits; do not
use Git commands to mutate the working copy/history behind Jujutsu.

## Start or resume once

Inspect state before doing anything destructive:

```sh
jj status
jj log -r '@ | @- | main@origin' --no-pager
```

If `@` already contains the current task, resume it. A conversation restart or context compaction is not
a reason to create a new change, fetch again, or restart the publication workflow.

For a genuinely new task:

```sh
jj git fetch --remote origin
jj new main@origin -m "Describe the coherent task"
uv run wiz8 doctor
```

`doctor` is the recovery preflight after adopting a new base. It checks the local toolchain, repository
safety, checkout-owned Ghidra project ownership, and whether the live Wiz8 analysis is known to come
from the reviewed GZF tracked by this revision. Do not begin retail/Ghidra recovery when it reports a
stale, untracked, or freshness-unknown live project.

Keep one mutable change per coherent task by default. Do not create empty children to imitate Git
commits. Use `jj describe` when the task description needs correction.

## Existing pull request or remote branch

Treat remote-bookmark commits as immutable input. Do **not** weaken `immutable_heads`, rewrite the
remote PR stack in place, or force Jujutsu to make remote history mutable just to rebase/squash it.

1. Inspect the PR head/base (`gh pr view ...`) and fetch origin.
2. Inspect the exact PR stack with `jj log`/read-only `git show` or `git diff`.
3. Duplicate the PR revisions into mutable local revisions with `jj duplicate`; for a stack, duplicate
   the explicit stack together so its parent relationships are preserved.
4. Rebase/squash only the mutable duplicates onto `main@origin`.
5. Resolve conflicts against the intended architecture and review the resulting diff; do not choose a
   conflict side merely because it is newer.
6. Run the invalidated validation and merge-preservation checks before publication.

If the task is only to inspect a PR, do none of the mutation steps.

When updating the same remote PR branch, track its remote bookmark before moving the local bookmark:

```sh
jj bookmark track PR_BRANCH@origin
jj bookmark set PR_BRANCH -r NEW_HEAD
jj git push --remote origin --bookmark PR_BRANCH
```

If the bookmark is already tracked, skip the first command. Do not create a competing local bookmark
and then track the remote one; that manufactures a bookmark conflict. Do not invent Git-style push
flags such as `--allow-backwards`; inspect `jj git push --help` when an unusual push is actually needed.

## Rebase and conflicts

Fetch only when upstream state matters, normally before integration:

```sh
jj git fetch --remote origin
jj rebase --destination main@origin
```

After a rebase, inspect the conflict set at the rebased revision, not merely at whatever unrelated
working-copy revision happened to be current. Resolve coherent groups, then review the complete diff.
Do not repeatedly rerun successful checks whose inputs did not change.

If the rebase changes `vendor/ghidra/exports/manifest.json` or a tracked GZF, run `uv run wiz8 doctor`
before any further Ghidra-derived recovery. A matching program name or retail binary hash does not
prove that the live analysis contains the current reviewed checkpoint.

For recovery changes, run the marker-preservation audit after the final rebase/merge:

```sh
uv run wiz8 report merge-preservation --base origin/main
```

Every allowed loss/duplicate/demotion needs its actual evidence-backed reason; conflict resolution by
itself is not a reason.

## Publish

For a pull request, complete the required validation boundary first:

```sh
uv run wiz8 pr-check
jj git push --remote origin --change @
```

Use the bookmark name reported by Jujutsu to open the PR. For an existing PR, use the tracked-bookmark
flow above instead of creating a new publication bookmark.

Direct publication to `main` requires explicit authorization and follows the short recipe in
`docs/contributor-workflow.md`. A successful push ends publication; do not fetch, revalidate, create an
empty child, or perform tree-equivalence proofs merely to reassure yourself after a successful push.
