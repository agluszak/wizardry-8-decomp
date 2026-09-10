# Wizardry 8 Standard Gaming Platform

This is the reconstructed Wizardry revision of Sir-Tech's source-available SGP
project. It is the only buildable SGP implementation. `WIZ8_SGP` is one static
library shared by the comparison, runtime, and runtime-test executables.

## Released baseline and licence

The historical base is the `sgp/` directory from
`https://github.com/ja2-stracciatella/ja2-stracciatella.git`, commit
`5ac0a9d56d27e8a7e2c4a7b48ed8932ae7f64033` (Initial Import, 2004-09-06),
tree `52766c4237e63d7a3d619796784947ed1681f24e`.
The import/move commit in this repository is `cb593aff`.
The released prebuilt `SMACKW32.LIB`, `ddraw.lib`, and `mss32.lib` are omitted.

The accompanying `SFI Source Code license agreement.txt` is retained verbatim
(upstream blob `b66aabc6f7affb4fca0b6cc2d6288f3225ecd0b1`). Preserve it and all
upstream notices. Changed files carry dated modification notices. This component
is not offered under commercial-use or broader terms.

`git diff cb593aff -- src/sgp` is the Wizardry-versus-release patch. There is no
parallel pristine implementation or override layer. Most source bodies remain
inherited; a difference in generated code does not by itself prove changed source.

## Project boundary

`CMakeLists.txt` records the reconstructed object membership directly. The
released JA2 `.dsp` is historical evidence, not a second build specification.
The non-JA2 cursor entry is called by retail mouse routing, and the generic
button-sound scheme writer is called at `0x0040d5c2` and `0x0040d831`.
The bootstrap review did not identify a retained DbMan, Install, or WinFont
contribution; the relevant WinFont and Mutex imports are absent. Mutex
initialization/shutdown and exception reporting are disabled in the Wizardry
branches. The released `video.c` depends on JA2 rendering; Wizardry supplies
`Video2.cpp`. These unused ancestor files remain available for historical diffs,
but have no build or analysis-only target.

SGP's product calls use the real `GameData.h`, `Video2.h`, and `gameloop.h` C
boundaries. Only unresolved original configuration headers remain in
`include/wiz8/sgp-compat`. `WIZ8_PRECOMPILED_HEADERS` is not defined.
`tools/lint/include` contains host-only filename-capitalization adapters.

The recovered deltas live in their original units, including the 252-character
font table, startup and input behavior, sound-cache revision, and SLF mapping
and patch precedence. The retail English input table is **512 words**, not the
released 1,024: its two character banks occupy `0x005ffc3c..0x0060003b`.
Library initialization records are `0x103` bytes and library records are `0x28`
bytes; the latter include the patch flag and mapping fields. `WizLibs.c` owns the
product library configuration: 50 records, six initially populated. Retail
allocates 56 open-library records separately; the two capacities are not the
same. The patch loop is preserved, including this original capacity discrepancy.

JA2 Utils `Text_Input` is not an SGP unit. Its source ancestor remains under
`third_party/sfi-ja2-utils`, and the Wizardry derivative remains product code.
Miles startup/exit support belongs to the Miles import boundary, not to SGP.

## Comparison and provenance

Compile and compare whole translation units so `/Ob2` sees the actual helpers,
globals and headers. reccmp's COFF object view retains functions, static symbols,
data, common/BSS storage, section attributes and relocation targets. An exact
selected-contribution comparison masks relocatable operands at an independently
known original extent; it does not establish relocation-target identity or
authored source syntax. It requires no retained function in a recompiled PE.

`/OPT:NOREF` remains the comparison-image mode and `/OPT:REF` the runtime mode;
both link this same archive. Extra retained comparison-image functions are a
linker diagnostic, not a reason to build a different platform implementation.

Ghidra owns live identities and types. Accepted source identities are projected
from the ordinary provenance records into disposable reccmp data. The old
cross-build observations remain in `evidence/snapshots/sgp/harness.csv`; they are
historical results, not an active comparator, unit inventory, or current score.
