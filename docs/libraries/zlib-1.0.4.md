# zlib 1.0.4 in `Wiz8.exe`

The canonical `Wiz8.exe` contains zlib 1.0.4. The embedded strings include `1.0.4`,
`deflate 1.0.4 Copyright 1995-1996 Jean-loup Gailly`, and
`inflate 1.0.4 Copyright 1995-1996 Mark Adler`. Its decoder error strings match the pinned 1.0.4
source archive whose SHA-256 is
`e5c260cd3db1370fb3e0c193e9cbd9f127a9bd055d622b3fb55b82747f6e5b24`.

## Recovered ownership

The reviewed map in `config/analysis/functions/wiz8-zlib.csv` accounts for 51 entries:

| Ownership | Entries | Interpretation |
| --- | ---: | --- |
| Sir-Tech adapter | 5 | Game-owned allocation callbacks plus create, streaming, and cleanup wrappers |
| zlib 1.0.4 | 46 | Inflate, retained deflate helpers, compression trees, Adler-32, and allocators |

The zlib-owned range begins at `0x00415910` with `inflateReset` and ends at `0x0041A7ED` with
`inflate_fast`. The next defined function at `0x0041A7F0` is game code, and the larger function at
`0x0041AB40` references `C:\Projects\Wizardry 8\Engine Code\GameData.cpp`. The three functions at
`0x00415850`-`0x004158F0` are Sir-Tech wrappers, not zlib source. The configured stream callbacks
at `0x00415820` and `0x00415840` are also Wizardry-owned: unlike zlib's later `zcalloc`/`zcfree`,
they call the executable's imported `malloc` and `free` directly.

The linked corpus is broader than the wrapper API suggests. Alongside five public inflate
functions, the executable retains seven deflate helpers and seventeen functions from `trees.c`.
The deflate helpers call the same tree machinery used by zlib's compressor. They should be
classified as library code even though the game-facing wrapper shown here drives decompression.

## Cross-build boundary recovery

Ghidra initially discovers only 20 of the 46 zlib functions in the canonical program. The 1.28
executable contains the same block shifted by `+0xC0` and exposes all but three boundaries through
ordinary references. Corresponding discovered functions have identical address-insensitive
instruction fingerprints. Subtracting the uniform offset recovers 23 additional canonical
boundaries without treating proximity as a name oracle.

Three entries remain absent from both Ghidra function lists and were recovered from aligned
disassembly:

| Canonical address | 1.28 address | Function | Size |
| --- | --- | --- | ---: |
| `0x00415F60` | `0x00416020` | `deflate_stored` | 265 |
| `0x004165C0` | `0x00416680` | `deflate_slow` | 752 |
| `0x00417960` | `0x00417A20` | `zcfree` | 14 |

Their names are grounded in source control flow and data access: the two deflate loops call the
already identified `fill_window`, `longest_match`, `_tr_tally`, `_tr_flush_block`, and
`flush_pending` functions; `zcfree` is the exact configured deallocator wrapper.

## Function groups

- `0x00415910`-`0x00415AEF`: `inflateReset`, `inflateEnd`, `inflateInit2_`, `inflateInit_`, and
  `inflate`.
- `0x00415F10`-`0x004168AF`: retained deflate helpers including `deflate_stored`, `deflate_fast`,
  and `deflate_slow`.
- `0x004168B0`-`0x004177FF`: the inflate block state machine.
- `0x00417810`: `adler32`; `0x00417940` and `0x00417960`: zlib's default allocator wrappers.
- `0x00417970`-`0x00419229`: the compression-tree implementation from `trees.c`.
- `0x00419230`-`0x0041A7ED`: inflate Huffman trees, code decoder, flush routine, and fast path.

The source-built FID matrix does not reproduce the large target bodies with the available complete
VC6 compiler snapshots. The names here therefore come from exact version strings, unique source
semantics, the recovered call graph, and cross-build boundaries—not from weak FID scores. This is
consistent with the executable's mixed 8447/8168/9044 Rich records and the unavailable complete
8447-era optimizer combination.

## Recovered Wizardry boundary

The five game-owned functions are now compilable C in `src/wiz8/zlib_wrappers.c`. The pinned SGP
initial import contains this same boundary in `sgp/Compression.c`, so these are source-backed
original names rather than descriptive placeholders:

| Address | Recovered function | Behavior |
| --- | --- | --- |
| `0x00415820` | `ZAlloc` | multiply `items * size`, call imported `malloc` |
| `0x00415840` | `ZFree` | ignore the opaque context, call imported `free` |
| `0x00415850` | `DecompressInit` | allocate a `0x38`-byte `z_stream`, install callbacks, call `inflateInit_`, and retain the caller's input span |
| `0x004158B0` | `Decompress` | set the output span, call `inflate(..., Z_PARTIAL_FLUSH)`, and return bytes produced |
| `0x004158F0` | `DecompressFini` | call `inflateEnd` and free the stream |

`just build WIZ8_ZLIB_WRAPPERS` compiles this source with the pinned VC6 SP5 `/O2 /MD` toolchain
against the pristine zlib 1.0.4 headers. Masking only COFF relocation fields, every resulting body
is byte-exact against the canonical executable:

| Function | Size | Relocation-normalized SHA-256 |
| --- | ---: | --- |
| `ZAlloc` | 20 | `700a399b5e19bf990286dbd979f67666bb7c07081a2d2a680c6ef64208752ea0` |
| `ZFree` | 13 | `63e4417e0067e692fa73a952f0d05010d0cb3de2aa0a70158e4b77ab2ede7f80` |
| `DecompressInit` | 92 | `f014577ffe8c54f5ee596331f0d946e2744bf4c06cd9e49f334686879dc7a84e` |
| `Decompress` | 52 | `08c5436c2c1a757aa99d8640d853437ba2676511d87433c084e9de962812c516` |
| `DecompressFini` | 23 | `1a71a7db964a267e97b227a93fb0c56369eecb9619d248ffbc31d9ecf4400234` |

## Applied Ghidra model

`uv run wiz8 ghidra apply-functions ... --map config/analysis/functions/wiz8-zlib.csv` creates the
28 starts missed by the initial canonical analysis and applies all 51 reviewed identities. The two
additional starts beyond the earlier census are the Wizardry allocator callbacks.

`uv run wiz8 ghidra apply-zlib-model` installs the exact 32-bit release-1.0.4 layouts for
`z_stream`, the `0x18`-byte inflate state, the `0x3C`-byte block state, the `0x1C`-byte code state,
the eight-byte `inflate_huft`, the four-byte `ct_data`, and the 12-byte `tree_desc`. It applies
source-derived `__cdecl` prototypes to all 46 zlib functions and all five Wizardry boundary
functions. As a result, the decompiler now renders `stream->next_in`, `avail_in`, `next_out`,
`avail_out`, allocator callbacks, and private inflate-state fields directly rather than anonymous
word offsets.
