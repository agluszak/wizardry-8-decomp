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

Ghidra already recognizes `__aulldiv`, `__allmul`, and `__aullshr`. `LIBRARY` source markers supply
the reviewed names with their original decorated spelling. Keeping these functions classified as compiler support prevents them from
inflating Wizardry source-recovery counts. `uv run wiz8 check`'s `source-oracle` gate treats the
documented CRT helper cluster, CRT startup range, and these `LIBRARY` / `fid-variants` claims as
oracle-owned: a Wizardry `FUNCTION` body in that space is a gate failure.

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
the compiler-generated destructor sites that call it. Both callable identities are source-marked;
the IAT jumps carry `SYNTHETIC` markers because the linker emits them, while the
local delete wrapper retains its `LIBRARY` marker. The IAT identities and their ownership meaning
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

## Narrow CRT string/memory intrinsics

Under the first-party `/O2 /G6 /MD` profile, `/O2` implies `/Oi` and VC6 expands these narrow
operations inline instead of emitting MSVCRT calls: `strlen`, `strcpy`, `strcat`, `strcmp`,
`memcmp`, `memcpy`, `memset`, `_strset`. The wide-character twins (`wcslen`, `wcscpy`, `wcscmp`,
`wcscat`, ...) are not intrinsics and always appear as CRT calls. An anonymous narrow
scan/copy/compare loop in retail code is therefore most likely an intrinsic expansion, not an
authored loop — check the fingerprint before writing source. The reproducible fixture is
`docker/msvc600/probes/intrinsics_probe.cpp`.

| Source op | Instruction fingerprint |
| --- | --- |
| `strlen(s)` | `or ecx,-1; xor eax,eax; repnz scasb` over `s`, then `not ecx; dec ecx` |
| `strcpy(d,s)` | `strlen` sequence over `s`; `sub edi,ecx; mov esi,edi; mov edi,d; mov edx,edi; shr ecx,2; rep movsd; mov ecx,eax; and ecx,3; rep movsb` |
| `strcat(d,s)` | `strlen` sequence over `s`, then `repnz scasb` over `d`, `dec edi`, then the `rep movsd`/`rep movsb` copy tail |
| `strcmp(a,b)` | two-byte-at-a-time walk: `mov dl,[a]; mov bl,[b]; cmp; jne; test cl,cl; je; mov dl,[a+1]; mov bl,[b+1]; add a,2; add b,2; jne head`; result `sbb eax,eax; sbb eax,-1` |
| `memcmp(a,b,n)` | `repz cmpsb` then `je`/`sbb eax,eax; sbb eax,-1` |
| `memcpy(d,s,n)` | `shr ecx,2; rep movsd; mov ecx,n; and ecx,3; rep movsb` |
| `memset(d,c,n)` | byte splat (`bl`,`bh`, `shl 16`, `mov ax,bx`), `shr ecx,2; rep stosd; and ecx,3; rep stosb` |
| `_strset(s,c)` | `repnz scasb` length pass, `not ecx; dec ecx`, byte splat, `rep stosd`/`rep stosb` |

## Toolchain identification evidence

The VC6 family is confirmed; the exact compiler build remains unresolved.

- The canonical executable reports LINK 6.0 and imports MSVCP60.dll and MSVCRT.dll.
- SurRender PDB paths contain an explicit msvc6 directory component.
- The canonical Rich header contains product/build records 10/8447, 11/8447, 11/8168, and 48/9044.
- The SP5 repository contains LINK.EXE 6.0.8447.0, while its compiler probe emits C/C++ records different from the canonical binary's dominant 8447 records.
- Product 48/build 9044 is consistent with the separately tested Processor Pack optimizer candidate.

Limitations:

- No single complete archaic-msvc repository reproduces every Rich record in the canonical executable.
- The msvc600_sp3 and msvc600_sp4 repositories contain libraries and headers but no compiler executable, so they must be tested as overlays rather than standalone toolchains.
- Rich records describe all linked objects and do not by themselves prove which compiler built each statically linked library.

The MSVC static archives were support-code oracles; they do not establish that
the product linked the static CRT.

## Exported trivial copies

The pinned SP5 compiler exports an implicit assignment operator, but no standalone implicit
copy constructor, for a class-level `dllexport` record containing an array of four-byte,
trivially copied class members. A second record with a user-defined destructor has the same
behavior. Probes under `/O2 /G6 /MD /GX`, `/Od`, `/O2 /Ob0`, and `/GX-` retain
this distinction. Exporting the member class also does not produce the containing copy
constructor. A counterfactual probe with an explicit member copy constructor does produce it;
that establishes a compiler sensitivity, not an original-source declaration.

The retail `srModeler::Triangle` and `Polygon` copy exports at `0x10037C10` and `0x10037CF0`
copy the texture-pointer region, four shader words, and the remaining members without extra
behavior. `Polygon` copies its vertex pointer shallowly; `Triangle` block-copies its vertex
region. The integrated shader model retains a header-visible nontrivial value copy. The
compiler emits both containing implicit copy constructors with this model, resolving the
missing definitions. This supports the member-copy model without establishing its exact
historical spelling. See [provider export evidence](surrender-import-visibility.md#compiler-owned-provider-exports).

The probe object records compiler product/build `11/8966`; retail `sr.dll` records `11/8447`
for 113 linked objects. This difference keeps exact compiler selection unresolved. It does not
establish that changing compiler build would reproduce these exports.

`wiz8 build SURRENDER` inspects the actual VC6 provider objects before linking. It reports
missing definitions required by `sr.def` and compiler export directives absent from the reviewed
retail ABI together. The objects are not rewritten, and no substitute bodies are generated.
These failures leave the declaration/compiler questions explicit.
