# SGP harness snapshot

`harness.csv` is the reviewed cross-build snapshot for the settled SGP project profile
`/O2 /Ob2 /G5 /MD`. It is tracked because reproducing it requires the proprietary Wizardry
executables named and hash-pinned in each row.

The SGP-specific comparator and its active unit/findings inventories were retired
when SGP became one reconstructed source component. This snapshot preserves the
historical investigation and is not refreshed by routine gates. Accepted source
identities were moved to the ordinary `evidence/reviewed/wiz8/claims.csv` records.

Old labels such as `compiled-empty` describe that scanner's external-text view,
not complete object ownership. Likewise a near match did not establish modified
source. Use current reccmp object/function/data comparisons for current results.

The CSV records the source function's relocation-masked hash and every target module SHA-256, so
its source and binary inputs are independently identifiable.
