# SGP harness snapshot

`harness.csv` is the reviewed cross-build snapshot for the settled SGP project profile
`/O2 /Ob2 /G5 /MD`. It is tracked because reproducing it requires the proprietary Wizardry
executables named and hash-pinned in each row.

The optional `wiz8decomp.sgp_oracle` archaeology command compiles the normal root-project SGP
targets and writes `build/reports/sgp/harness.csv`. This snapshot preserves the completed
historical investigation and is not refreshed by routine gates:

```sh
uv run wiz8 sgp sweep
```

The CSV records the source function's relocation-masked hash and every target module SHA-256, so
its source and binary inputs are independently identifiable.
