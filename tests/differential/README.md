# SurRender differential runner

`sr_difftest.exe` is a VC6 console program linked against the SurRender
provider import library. It runs the same cases in separate Wine processes
against different `sr.dll` builds. Retail is the reference. The trace prints
only values and logical identities, never process addresses, so traces from
different DLLs can be diffed line by line.

```sh
# build SURRENDER + runner, then compare retail (reference) with the rebuilt DLL
uv run wiz8 sr-difftest --build
# any number of candidates; the first --dll is the reference
uv run wiz8 sr-difftest \
  --dll retail=$WIZ8_WORK_DIR/variants/gog-base/sr.dll \
  --dll rebuilt=build/decomp/sr.dll \
  --dll other=/path/to/other/sr.dll
# subsets: exact names or trailing-* prefixes
uv run wiz8 sr-difftest 'fixed.transform3*' 'probe.*'
```

Output goes to `build/differential/sr-vp/`: one directory per variant, each
holding its staged DLL set and `trace.txt`, plus `report.json`. The command
exits 1 on any divergence. Each variant directory contains only the runner,
that variant's `sr.dll`, and the retail `MSVCRT.DLL`/`MSVCP60.DLL`, loaded
native-first through `WINEDLLOVERRIDES`. Every container runs with
`--network none`.

Runner options: `--seed N` (default 1), `--generated K` rounds,
`--window FIRST COUNT` (rerun only those elements; the driver uses this for
minimized reproducers), `--list`.

## Cases

* `fixed.transform3.swapxy.v123.{separate,inplace}`: the X/Y swap matrix
  applied to (1,2,3). Expected (2,1,3) in both modes.
* `fixed.<transform op>.<matrix>.n4`, `fixed.<op>.general.n0/n1`: these cover
  transform3, transform4, transform3to4, transformOrtho and
  transformPerspective, each with the identity, swapxy, translate, rotz90,
  rotx30, persp, ortho and general matrices. The transforms go through the
  public `srVectorProcessor` wrappers.
* `fixed.transformIndexed*`: repeated indices and the n0 case.
* `gen.<op>.<separate|inplace>.n<count>.r<round>`: deterministic xorshift
  inputs for every op in the table in `sr_difftest.cpp`, at counts
  0/1/2/3/5/8/17/64. The per-case seed comes from the case name plus `--seed`.
  Outputs have guard bytes around them, so underruns and overruns get
  reported. Each op runs under SEH.
* `probe.registry.grow-shrink`: an `srRegistry` with 40 named instances.
  After each insert and each removal, the probe checks that every live name is
  still found. Capacity starts at 4, so the 5th insert grows the table.
* `probe.gerd.driver-lists`: a runner-owned `srDD` (`ProbeDD`) reports 3
  texture formats and 2 display modes to a real `srGERD::createContext`.
* `probe.gerd.default-texture`: `openWindow` builds the 64x64, 7-level
  default texture through `createNewTexture`'s MultiRequest. The probe
  prints each mip level's size and FNV checksum as `ProbeDD::bindTexture`
  sees it.
* `probe.camera.*`: `srCamera::processPush` and the MODELVIEW matrix for
  identity, translated, rotated and translated+rotated cameras.

## Aliasing scope

In-place mode (`dest == source0`) runs only for ops whose in-place use
appears in retail code. The list below was found by scanning retail Wiz8.exe
and sr.dll. A call site counts when it pushes the same register twice for
dest and source, with no write in between, and then makes an srVP vtable
call.

| op | retail call site |
| --- | --- |
| transform3 (+0x224) | Wiz8 0x46EC5C |
| transform4 (+0x220), transformPerspective (+0x240), transformOrtho (+0x23c) | sr 0x10025B52 / 0x10025B5A / 0x10025B68 |
| add3K (+0xd4) | Wiz8 0x46ECAD |
| reverse | sr 0x1005767D |
| subFF / mulFF / clampMin / clampUnit / specularPow | sr 0x1004D497 / 0x10033F50 / 0x1004D4F9 / 0x1002B5A3 / 0x1004D51F |
| axpy4VKFF / axpy4VKF | sr 0x1004D39C / 0x1002BD1B |
| mulIndexed4V / mulIndexedF | sr 0x1002B5D7 / 0x1002B439 |
| normalize3 | sr 0x1003E676, Wiz8 0x472F62 |
| addFF / mul3F | Wiz8 0x47474E / 0x47239C |

The scan is heuristic. It can miss aliasing that runs through different
registers or memory, and other aliasing patterns (partial overlap,
`dest == source1`) are not exercised.

The runner prints each wrapper's vtable slot offset by decoding its
compiled vcall thunk. These offsets match the retail call sites above.

## Caveats

* `ProbeDD` follows `include/surrender/srDD.h`. The probes can only check
  the GERD side of the DD interface; no retail driver DLL is involved.
* The retail list layout `{pointer @0, count @4}` was confirmed from the
  retail `srGERD::initTextureFormats` (0x10018F30) and `initDisplayModeList`
  (0x10018FE0) instructions. Retail provider DLLs were not checked.
* Every variant shares the same Wine build and the same retail CRT, so CRT
  and loader behaviour cannot differ between variants.
* Clean traces show agreement only on these inputs. They do not prove
  equivalence.
