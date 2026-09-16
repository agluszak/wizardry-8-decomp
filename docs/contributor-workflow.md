# Contributor workflow

Use Jujutsu in the provided checkout. Repository policy is in `AGENTS.md`; detailed repository-state
recipes, especially existing-PR rebases, live in the agent
[`jujutsu-workflow`](../.agents/skills/jujutsu-workflow/SKILL.md) skill.

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
uv run wiz8 doctor
```

`doctor` is the recovery preflight for the adopted revision. In addition to machine/tooling checks, it
verifies checkout-local Ghidra ownership and whether the live canonical Wiz8 program is provably based
on the reviewed GZF tracked by this revision. `not-restored` is safe; `stale`, `untracked`, or `unknown`
means retail-derived recovery must stop until Ghidra state is explicitly reconciled/refreshed. Doctor
never repairs or overwrites the live project.

If a later rebase/merge changes the reviewed Ghidra manifest/checkpoint, rerun doctor before using
Ghidra again.

Keep one mutable change per coherent task by default. Bookmarks are unnecessary during ordinary
work. Split changes only when it materially improves review or recovery. Give the completed change
an accurate description when needed:

```sh
jj describe -m "Recover W8DialogInterface"
```

Do not create an empty child merely to imitate `git commit`.

## Existing pull requests

Remote PR commits are normally immutable Jujutsu history. Do not override `immutable_heads` merely to
squash or rebase them. Use the `jujutsu-workflow` skill: duplicate the PR stack into mutable local
revisions, integrate those, and only move/track the remote bookmark at publication. Read-only `git show`/`git diff` is fine for
inspecting colocated immutable commits.

## Validate the change

Follow the verification policy in `AGENTS.md`. After a rebase, rerun only checks affected by incoming
changes or conflict resolution.

After any rebase or merge of a recovery branch, compare marker identities by retail address against
the base before publishing:

```sh
uv run wiz8 report merge-preservation --base origin/main
```

The default result is the current Jujutsu change (snapshotted to its commit ID), or the current
Git working tree. `--head COMMIT` explicitly selects revision-only mode and excludes working edits.
Git refs (`origin/main`, a commit hash) work with both colocated and non-colocated Jujutsu checkouts.
The report records resolved commit/tree IDs and flags identical source selections; that is not a
two-input integration audit.

It fails on removed or duplicated `FUNCTION`/`GLOBAL`/`VTABLE` addresses and lists changed identities
and removed functions whose names are still referenced. Explain each intentional loss with
`--allow WIZ8:FUNCTION:0xADDRESS:loss=reason`; the transition is `loss`, `duplicate`, or `demotion`.
Exceptions cover only that target, kind, address, and transition. A conflict resolution is not a
reason, and a simultaneous FUNCTION/STUB claim cannot be waived as a withdrawal.

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
uv run wiz8 pr-check
```

This is the required PR validation boundary. It always runs the fast repository lane and cannot omit
the clang-cl/tidy lane when the change includes C/C++ source or headers.

```sh
jj git push --remote origin --change @
```

Use the bookmark name reported by jj when opening the pull request. A checkpoint or draft PR does not
require an integration rebase or broad validation merely to upload work. Merge only when authorized.
