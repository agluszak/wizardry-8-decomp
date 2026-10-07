# Wizardry 8 Standard Gaming Platform

This is our reconstruction of the Wizardry fork of Sir-Tech's source-available
SGP project. It is the only buildable SGP implementation. `WIZ8_SGP` is one
static library shared by the comparison, runtime, and runtime-test executables.

`src/sgp` is editable recovered source, like `src/wiz8`, with unusually strong
ancestry evidence. It is **not** the source oracle.

## Released baseline (the oracle)

The immutable ancestor oracle is the `sgp/` directory of
`https://github.com/ja2-stracciatella/ja2-stracciatella.git`, commit
`5ac0a9d56d27e8a7e2c4a7b48ed8932ae7f64033` (Initial Import, 2004-09-06), tree
`52766c4237e63d7a3d619796784947ed1681f24e`, `sgp/` subtree
`5cf5916502ee719fe4966b7f15ec643379ecb390`.

A byte-identical copy (every blob matches the upstream subtree) is in this
repository's history at commit `72697ddaac1c`, path `third_party/sfi-sgp/sgp/`:

```sh
git show '72697ddaac1c:third_party/sfi-sgp/sgp/<file>'
```

The released prebuilt `SMACKW32.LIB`, `ddraw.lib`, and `mss32.lib` were never
imported. The later move commit `cb593aff` already carried a Wizardry
`english.h`; it is not a pristine reference. Claims that cite
`released-baseline:fdff790b` refer to that import tree (pristine apart from
`english.h`).

Released SGP is shared JA2/Wizardry/utility source. It is an excellent ancestor
for names, types and bodies, but Wizardry's SGP was a fork. Released bodies
are evidence only where binary/object evidence shows Wizardry retained them.

## Ownership versus provenance

- A `FUNCTION`/`GLOBAL` marker in `src/sgp` establishes current **ownership**:
  the address belongs to the SGP component and must not be recovered in
  `src/wiz8`. It is not evidence that the name or body is original.
- `sgp-source` provenance comes only from an explicit reviewed claim in
  `evidence/reviewed/wiz8/claims.csv`, backed by the released baseline and an
  object/binary comparison. The `source-oracle` gate reports SGP owner markers
  that lack such a claim as unproven ownership
  (`build/reports/source-oracle.json`, `"proven": false`).
- Names introduced here without a released counterpart (for example
  `OpenLibraryStream` at `0x00412f10`) are our names until evidence says
  otherwise.

## Licence

The accompanying `SFI Source Code license agreement.txt` is retained verbatim
(upstream blob `b66aabc6f7affb4fca0b6cc2d6288f3225ecd0b1`). Preserve it and all
upstream notices. Changed files carry dated modification notices. This component
is not offered under commercial-use or broader terms.

## Build and diagnostics

`CMakeLists.txt` owns archive membership. All products use the same `WIZ8_SGP`
archive and owning headers; the historical SGP API retains C linkage.

Retained DirectDraw diagnostics use `ATTEMPT_AT` with retail line constants.
Debug-only diagnostics use current-source coordinates. Compile whole translation
units so `/Ob2` sees the actual helpers, globals and headers.
