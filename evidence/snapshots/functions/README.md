# Function-candidate and call-graph snapshot

Candidate function starts and the static call graph for every first-party Wizardry executable whose
code is readable. Tracked because reproduction needs the proprietary binaries.

The producer is `wiz8decomp.function_census`. Normal runs write the same CSVs under
`build/reports/functions/` and fail when they differ from this snapshot:

```sh
uv run wiz8 function-census                  # verify against the snapshot
uv run wiz8 function-census --update-snapshot
```

These are candidates, not identities. Promoting a start to a canonical function is a reviewed
decision and belongs in `evidence/reviewed/wiz8/functions.csv`; regenerating this snapshot cannot
overwrite one, because nothing here writes there.

`sources` lists which independent things attest the address, and they are not equal. `eh-record` is
read from the exception tables through the handler thunk and is exact. `call-target` and
`data-pointer` are strong when the address also begins a prologue. `padding` is by far the noisiest,
because alignment padding also appears inside functions and after data, so a padding-only candidate
is only kept when it both lands on an instruction boundary and begins a prologue.

`aligned` records whether the address is an instruction boundary in a resynchronising linear decode.
A candidate that is not is data or a misread and is rejected whatever else attests it, short of an
exception record.

`verdict` is `exact`, `strong`, `padding-prologue`, `called-no-prologue`, `referenced-no-prologue`
or `rejected`. The two `no-prologue` verdicts are deliberately not accepted and deliberately not
discarded: a tail-merged or hand-written body reached by a real call is exactly what they look like,
and so is a jump-table entry.

`size` is the distance to the next accepted start, so it is an upper bound rather than the body
length.

`calls.csv` is the deduplicated call graph: one row per `(caller, callee)` with the number of sites.
`caller` is the accepted function containing the call site, so an edge is only as good as the
boundary beneath it - which is the reason this producer emits both tables rather than either alone.
