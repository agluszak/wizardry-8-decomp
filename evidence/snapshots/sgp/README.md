# SGP harness snapshot

`harness.csv` is the reviewed snapshot of the full SGP compiler-matrix report. It is tracked because
reproducing it requires the proprietary Wizardry executables named and hash-pinned in each row.

The producer is `wiz8decomp.sgp_oracle`, configured by `config/sgp.yml`. Normal sweeps write to
`build/reports/sgp/harness.csv` and do not modify this snapshot. Refresh it only after reviewing a
complete sweep:

```sh
uv run wiz8 sgp sweep --update-snapshot
```

Partial `--unit` sweeps use this snapshot as the baseline when no generated report exists, but may
not replace it. The CSV records the source function's relocation-masked hash and every target
module SHA-256, so its source and binary inputs are independently identifiable.
