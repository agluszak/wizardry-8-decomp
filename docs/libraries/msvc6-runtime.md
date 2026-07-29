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
`evidence/reviewed/wiz8/function-provenance.csv` supplies the missing names with their original
decorated spelling. Keeping these functions classified as compiler support prevents them from
inflating Wizardry source-recovery counts.

The agreement across snapshots is deliberately not described as evidence for one VC6 service
pack. Compiler selection still has to use discriminating code bodies, Rich records, and eventual
rebuild comparison.

## Allocation ABI

The canonical executable's global C++ allocation boundary is asymmetric:

| Address | Shape | Reviewed identity |
| --- | --- | --- |
| `0x005E1CE0` | `jmp dword ptr [0x005EB1BC]` | six-byte import thunk to MSVCRT `operator new` |
| `0x005E1C10` | push argument, call `0x005E1C1D`, pop, return | local `operator delete` wrapper |
| `0x005E1C1D` | `jmp dword ptr [0x005EB224]` | six-byte import thunk to MSVCRT `free` |

Calling both outer entries "import thunks" hides a real distinction. The `operator new` identity is
read directly from the imported decorated export and is therefore ABI-backed. The delete wrapper
is not imported under that name: its descriptive identity comes from its exact forwarding body and
the compiler-generated destructor sites that call it. Both callable identities are reviewed in
`evidence/reviewed/wiz8/function-provenance.csv`; the IAT identities and their ownership meaning
are reviewed separately in `evidence/reviewed/wiz8/allocator-layers.csv` because an IAT slot is data,
not a function.

This is the old VC6 Microsoft-extension allocation contract, not standard throwing `new`. The
pinned VC6 CRT source implements that `operator new(unsigned int)` as `_nh_malloc(size, 1)` and
returns its result. `Wiz8.exe` imports neither a new-handler setter nor `_CxxThrowException`, and its
generated code treats allocation failure as an ordinary null result. For example, the pointer-array
constructor at `0x00509890` leaves the global null if object allocation fails and stores capacity
zero if backing allocation fails; `MonsterDBFromSpecies` likewise frees a failed record load and
returns null. Ported code must preserve these explicit null paths, for example with a nothrow
allocation boundary, rather than assuming modern throwing `new` semantics.
