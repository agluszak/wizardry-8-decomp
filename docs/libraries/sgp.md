# Reconstructed SGP component

Wizardry's SGP revision is the first-class source project in
[`src/sgp`](../../src/sgp/README.md). The released SFI snapshot is its historical
base, not a runtime dependency alongside recovered overrides.

Its README owns the source revision, licence, project boundary and comparison
notes. `src/sgp/CMakeLists.txt` is the sole build membership list.

`uv run wiz8 report retail-folded` reports reviewed retail folds from the canonical
claims file. A folded no-op must not acquire the released ancestor's behavior;
an ordinary fold such as `DeleteList`/`DeleteStack` still performs deallocation.
The two-argument SGP `PlayButtonSound` is distinct from Wizardry's sound helper.
