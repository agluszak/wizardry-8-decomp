# Reviewed Ghidra checkpoints

Checkpoint operations are for sharing reviewed analysis state between checkouts. They are not part of
the ordinary inspect/edit loop; a normal native edit is persisted with `program.save()`.

## Restore

The canonical opener restores the seed when the local project does not exist. If an explicit restore
is required:

```sh
uv run wiz8 ghidra restore --program wiz8
```

Do not restore over ordinary live work, copy a live project, or open the same project concurrently.

## Refresh a reviewed checkpoint

After a coherent reviewed analysis batch, close the program session and export accepted state:

```sh
uv run wiz8 ghidra seed refresh wiz8
```

This updates the tracked reviewed GZF checkpoint. It is a sharing operation, not a per-function or
per-signature step and not a substitute for saving the live program.

## Reconcile divergent GZF checkpoints

Never choose one reviewed archive wholesale merely because it is newer. Use the common reviewed
ancestor as `BASE` and the other reviewed archive as `INCOMING`. Preview first:

```sh
uv run python tools/ghidra-scripts/merge_checkpoint_functions.py BASE INCOMING
```

Review the selected and pending function-analysis changes. Apply only after that review:

```sh
uv run python tools/ghidra-scripts/merge_checkpoint_functions.py BASE INCOMING --apply
uv run wiz8 ghidra seed refresh wiz8
```

`BASE` and `INCOMING` open immutably and must describe the same binary as the live program. The merge
handles non-overlapping function-analysis changes, function-entry symbols and tags. It deliberately
refuses overlapping local edits, incoming type changes, non-function symbol changes and unsupported
analysis state.

Resolve refusals explicitly with native Ghidra and retail/source evidence. Do not build a general
merge engine, signature ledger, replay log or per-edit archive system to avoid that judgment.
