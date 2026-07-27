# Data-segmentation snapshot

`unit-data-intervals.csv` holds the fitted per-translation-unit address
intervals for each data storage class of the canonical retail program, keyed
`program,storage_class,unit,lower,upper,baseline_globals`.

Producer: `uv run wiz8 report data-segmentation --update-snapshot`. The fit
joins the per-reference globals report (which needs the proprietary binaries in
`WIZ8_WORK_DIR`) with the assertion-anchored `.text` interval map; the snapshot
exists so the candidate replay can attribute globals to units from tracked
inputs alone. Every run without `--update-snapshot` regenerates the table under
`build/reports/data-segmentation/` and fails if it differs from this snapshot.

The intervals are an order-constrained fit, not reviewed identity: attribution
derived from them is bounded evidence, and the per-global table plus outlier
and exclusion detail stay under `build/reports/data-segmentation/`.
