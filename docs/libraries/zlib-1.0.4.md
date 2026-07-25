# zlib 1.0.4 in `Wiz8.exe`

The canonical `Wiz8.exe` contains zlib 1.0.4. The embedded strings include `1.0.4`,
`deflate 1.0.4 Copyright 1995-1996 Jean-loup Gailly`, and
`inflate 1.0.4 Copyright 1995-1996 Mark Adler`. Its decoder error strings match the pinned 1.0.4
source archive whose SHA-256 is
`e5c260cd3db1370fb3e0c193e9cbd9f127a9bd055d622b3fb55b82747f6e5b24`.

## Recovered ownership

The reviewed map in `config/analysis/functions/wiz8-zlib.csv` accounts for 49 entries:

| Ownership | Entries | Interpretation |
| --- | ---: | --- |
| Sir-Tech adapter | 3 | Game-facing allocation, streaming, and cleanup wrappers |
| zlib 1.0.4 | 46 | Inflate, retained deflate helpers, compression trees, Adler-32, and allocators |

The zlib-owned range begins at `0x00415910` with `inflateReset` and ends at `0x0041A7ED` with
`inflate_fast`. The next defined function at `0x0041A7F0` is game code, and the larger function at
`0x0041AB40` references `C:\Projects\Wizardry 8\Engine Code\GameData.cpp`. The three functions at
`0x00415850`-`0x004158F0` are Sir-Tech wrappers, not zlib source.

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
