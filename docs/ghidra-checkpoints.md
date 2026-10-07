# Reviewed Ghidra checkpoints

Reviewed GZF checkpoints exist to share reviewed analysis state between checkouts. Ordinary Ghidra edits are saved in the live checkout-owned ProgramDB; they do not require checkpoint refreshes.

## Freshness

After moving to a revision whose reviewed checkpoint may have changed, run:

```sh
uv run wiz8 doctor
```

The checkout records which reviewed GZF initialized the live project. If the tracked checkpoint changes, `doctor` reports the live project as stale. Legacy or otherwise unproven live state is reported as unknown/untracked. Matching binary hashes, function counts or timestamps are not proof that two analyses are identical.

`doctor` never overwrites Ghidra state.

## Restore

When no live project exists, the canonical opener restores the reviewed seed. For an explicit restore:

```sh
uv run wiz8 ghidra restore --program wiz8
```

Restore does not overwrite an existing live program. If stale/unknown live work matters, reconcile it first. Otherwise replace it only on explicit instruction; remove the live program files while keeping checkout ownership metadata, then restore the current seed. Do not create backup copies of live projects.

## Refresh

After a coherent reviewed analysis batch:

```sh
uv run wiz8 analyze decompiler-quality
uv run wiz8 ghidra seed refresh wiz8
```

`seed refresh` requires a successful decompiler-quality report tied to the current ProgramDB fingerprint. It is a sharing operation, not a per-function step and not a substitute for saving the live program.

## Reconcile divergent checkpoints

Do not choose one reviewed archive wholesale merely because it is newer. Use the common reviewed ancestor as `BASE` and the other reviewed archive as `INCOMING`:

```sh
uv run python tools/ghidra-scripts/merge_checkpoint_functions.py BASE INCOMING
```

Review the selected and pending changes, then apply only after review:

```sh
uv run python tools/ghidra-scripts/merge_checkpoint_functions.py BASE INCOMING --apply
uv run wiz8 ghidra seed refresh wiz8
```

`BASE` and `INCOMING` are opened immutably and must describe the same binary as the live program. The merge handles non-overlapping function-analysis changes, entry symbols and tags. It deliberately refuses overlapping local edits, incoming type changes, non-function symbol changes and unsupported analysis state.

Resolve refusals explicitly with native Ghidra and retail/source evidence. Do not build a general merge engine, signature ledger, replay log or per-edit archive system around this.
