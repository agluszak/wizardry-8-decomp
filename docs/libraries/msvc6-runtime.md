# VC6 runtime helpers in `Wiz8.exe`

The exact pinned `libcmt.lib` snapshots identify eight compiler/runtime helpers in the canonical
executable. Each target receives the same symbol from RTM, SP3, SP4, SP5, and SP6. These matches
prove function identity but do not select a service pack because the relevant bodies are identical
across all five snapshots.

| Address | Size | Symbol | Role |
| --- | ---: | --- | --- |
| `0x005E1C30` | 104 | `__aulldiv` | unsigned 64-bit division |
| `0x005E1CA0` | 52 | `__allmul` | 64-bit multiplication |
| `0x005E1CF0` | 170 | `__alldiv` | signed 64-bit division |
| `0x005E1DA0` | 47 | `__alloca_probe` | stack allocation/probing |
| `0x005E1DD0` | 31 | `__aullshr` | unsigned 64-bit right shift |
| `0x005E1DEF` | 106 | `??_L@YGXPAXIHP6EX0@Z1@Z` | vector-constructor iterator |
| `0x005E1E71` | 104 | `??_M@YGXPAXIHP6EX0@Z@Z` | vector-destructor iterator |
| `0x005E1EF1` | 81 | `?__ArrayUnwind@@YGXPAXIHP6EX0@Z@Z` | array unwind after construction failure |

Ghidra already recognizes `__aulldiv`, `__allmul`, and `__aullshr`. The reviewed map in
`config/analysis/functions/wiz8-vc6-runtime.csv` supplies the five missing names with their original
decorated spelling. Keeping these functions classified as compiler support prevents them from
inflating Wizardry source-recovery counts.

The agreement across snapshots is deliberately not described as evidence for one VC6 service
pack. Compiler selection still has to use discriminating code bodies, Rich records, and eventual
rebuild comparison.
