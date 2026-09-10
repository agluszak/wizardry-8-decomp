# Accepted source oracles

Before decompiling, determine whether accepted original source exists. If it does, use it as the
source oracle and recover the product delta/ownership rather than independently reverse-engineering
the same implementation. Match constants by numeric values in owning headers, not inferred Ghidra
enum names. Before retaining a function, check its oracle TU for inline assembly: an assembly-containing
unit requires per-function code-generation comparison; similarity to nearby C is insufficient provenance.

## SGP

Search `src/sgp` before recovering or declaring an SGP API elsewhere.

- `src/sgp` is the reconstructed Wizardry SGP revision and the only buildable SGP implementation.
  `WIZ8_SGP` is one static archive shared by comparison, runtime, and runtime-test products.
- Released SGP supplies ancestry/baseline evidence. Wizardry deltas belong in the original SGP units;
  there is no parallel pristine runtime implementation plus override layer.
- Include real owning product headers where recovered. `include/wiz8/sgp-compat` is only for genuinely
  unresolved original interfaces. Do not duplicate SGP functions into Wizardry C++ to work around
  archive extraction, folding, or other linker behavior.
- Preserve SFI-SCLA, upstream notices, and dated modification notices. History, licensing, source
  membership, and build specifics live in [src/sgp/README.md](../../../../src/sgp/README.md).

Compile the owning TU so `/Ob2` sees its real statics, globals, headers, and inline helpers. The
[COFF contribution comparison](comparison.md#coff-contribution-versus-original) can then validate
selected code/data independently of archive extraction and final linking. Whole-TU compilation and
object evidence are particularly useful here; masked equality still does not prove target identity.

## Other accepted boundaries

| Boundary | Source/evidence owner |
| --- | --- |
| Windows/MSVC runtime | Pinned toolchain source/headers; [MSVC6 runtime](../../../../docs/libraries/msvc6-runtime.md) records reviewed matches. |
| zlib 1.0.4 | Pinned source and [zlib boundary](../../../../docs/libraries/zlib-1.0.4.md). |
| IJG JPEG release 6 | Accepted codec source; recover the product adapter/delta described in [JPEG importer](../../../../docs/targets/srext-jpegimporter.md). |
| Info-ZIP UnZip 5.4 | Accepted library source; [ZIP extension](../../../../docs/targets/srext-unzip.md) owns adapter evidence. |

SurRender remains recoverable; exports, ABI observations, and header evidence do not make its
implementation ordinary available source. See [SurRender ABI](../../../../docs/libraries/surrender-abi.md)
when working at that boundary.
