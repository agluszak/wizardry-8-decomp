# Reviewed Ghidra checkpoints

Checkpoint operations are for sharing reviewed analysis state between checkouts. They are not part of
the ordinary inspect/edit loop; a normal native edit is persisted with `program.save()`.

## Freshness preflight

After moving a checkout to a new task base, run:

```sh
uv run wiz8 doctor
```

The checkout-owned project records the reviewed GZF hash that initialized each newly restored program.
If the tracked checkpoint later changes, doctor reports the live project as stale. A legacy live project
without that provenance reports `unknown`/`untracked`; do not treat matching binary hashes, function
counts, or timestamps as proof that its analysis equals the reviewed checkpoint.

Doctor never overwrites Ghidra state. If a stale/unknown live project contains work that must survive,
reconcile it with the current reviewed checkpoint as described below. If it contains nothing worth
preserving, replacing the checkout-owned project is still an explicit state-management/destructive
operation: preserve/authorize it first, then let the canonical opener restore the current seed.

## Restore

The canonical opener restores the seed when the local project does not exist. If an explicit restore
is required:

```sh
uv run wiz8 ghidra restore --program wiz8
```

A new restore records reviewed-seed provenance for future doctor runs. Restore does not overwrite an
existing live program. Do not restore over ordinary live work, copy a live project, or open the same
project concurrently.

## Refresh a reviewed checkpoint

After a coherent reviewed analysis batch, close the program session and export accepted state:

```sh
uv run wiz8 analyze decompiler-quality
uv run wiz8 ghidra seed refresh wiz8
```

`seed refresh` updates the tracked reviewed GZF checkpoint. It requires a successful
`decompiler-quality` report tied to the current ProgramDB fingerprint. It is a sharing
operation, not a per-function or per-signature step and not a substitute for saving the
live program.

Other checkouts whose live project was restored from the previous reviewed GZF will then fail the
freshness preflight until they explicitly reconcile or replace that live state.

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
