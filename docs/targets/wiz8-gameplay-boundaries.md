# Initial `Wiz8.exe` gameplay boundaries

The first manually owned executable source unit deliberately starts with deterministic
primitives rather than a large parser or gameplay state machine. Their canonical bodies are small,
fully typed, and used broadly enough to validate the compiler configuration before it shapes larger
recovery work.

| Address | Function | Canonical size | Evidence |
| --- | --- | ---: | --- |
| `0x00421680` | `VectorFromThreeFloats` | 25 | Converts three scalar doubles into a three-float vector and returns `this`; its verified CFAgent name is used by 149 canonical callers. |
| `0x00517970` | `RollDice` | 53 | Reads packed `W8Dice`; performs `count` independent `GetRandomNumber(sides) + 1` rolls after the signed base. |
| `0x005179b0` | `IntegerPower` | 24 | Returns one for exponent zero; otherwise multiplies by the base exactly `exponent` times. |
| `0x005179d0` | `ClampInteger` | 25 | Stores the maximum above range, the minimum below range, and otherwise leaves the value unchanged. |
| `0x004ac9d0` | `GetSpellTargetType` | 47 | Reads target type at offset `0x137` in the packed `0x1bf`-byte spell record and optionally normalizes single-target type 1 to type 0. |
| `0x004aca60` | `CanSpellBackfire` | 102 | Encodes the complete spell-ID exception sets for target groups 0-2, 3-7, and 10. |
| `0x004acb40` | `MinimumCasterLevelForSpellLevel` | 61 | Maps spell levels 2-7 to caster levels 3, 5, 8, 11, 14, and 18, with level 1 as the default. |
| `0x004acba0` | `GetMinimumCasterLevelForSpell` | 85 | Reads the spell level at offset `0x56` and applies the same mapping inline. |
| `0x00535ad0` | `GetFactionDisposition` | 100 | Enforces the signed faction domain `0..20`, reads the first byte of each 14-byte faction record, and classifies scores as hostile below 34, neutral below 67, or friendly. |
| `0x00517ea0` | `StripMonsterNameSuffix` | 26 | Finds the first wide `#` marker in any of the monster record's four display-name buffers and truncates the suffix in place. |
| `0x00517ec0` | `CharacterPointerToPartySlot` | 105 | Validates the character's `in_party` byte and converts its pointer to an index in the eight-element, `0x1862`-stride party array. Its reviewed name originated in the CFAgent hook oracle; the implementation is original Wizardry code. |
| `0x004ff3b0` | `GetProfessionCasterLevel` | 82 | Resolves profession `-1` through the character's current profession at `+0x69`, adds the corresponding unaligned profession level at `+0x8d`, and preserves the `-255` no-magic sentinel. |
| `0x00506280` | `GetFact` | 132 | Delegates derived-state evaluation to `EvaluateFact`, optionally logs the `FACT_*` symbolic name from the `0x1d8`-byte definition record, and preserves the original upper-ID guard. Its name comes from the verified CFAgent oracle. |
| `0x005061a0` | `SetFact` | 219 | Writes the mutable fact byte, records actual transitions through the journal/notification path at `0x005588f0`, dispatches `HandleFactChange` unless suppressed, and optionally logs the definition's `FACT_*` name. Its name comes from the verified CFAgent oracle. |
| `0x0040efa0` | `GetRandomNumber` | 50 | Returns zero for a zero bound; otherwise scales VC6 `rand()` by `RAND_MAX` before wrapping the inclusive upper endpoint. Its name comes from the verified CFAgent oracle and it has 365 canonical callers. |
| `0x0042b580` | `GetLoadedLevelID` | 6 | Returns the current level identifier used by triggers, monster code, save handling, and utility paths. Its name comes from the verified CFAgent oracle. |
| `0x00451280` | `GetWorld` | 6 | Returns the authoritative world object pointer used by 83 canonical callers. Its name comes from the verified CFAgent oracle. |
| `0x0051b9e0` | `GetItemInHand` | 19 | Returns `-1` when the item-in-hand validity byte is clear, otherwise the item ID at offset zero of the packed 12-byte instance. Its name comes from the verified CFAgent oracle. |
| `0x00518150` | `GetRandomCharacter` | 218 | Rolls a random skip count, then scans the eight party slots circularly, skipping empty rows, an excluded slot, a matching faction, and slots failing two eligibility tiers, until the skip count is exhausted. On failure it relaxes each requirement from 1 to 2 in turn and re-rolls. Its name comes from the verified CFAgent oracle. |
| `0x0042b410` | `GetLocationIDFromCode` | 231 | Case-insensitively searches the 47-entry table by its three-letter location code (record offset `0x64`), rejects a row whose folder name or level name is empty, and returns the index only if `0x0042A370` accepts it. Its name comes from the verified CFAgent oracle. |
| `0x0042b500` | `LevelGetLocationCodeByID` | 76 | Copies a row's three-letter location code out to a caller buffer. Bounds-checks only the upper end, so a negative `level_id` reads before the table. Descriptive name. |
| `0x0042b550` | `LevelGetFolderNameByID` | 33 | Bounds-checks the 47-entry level metadata table and returns its 50-byte folder-name field. The packed `0x6b` record also carries a 50-byte level name and a three-letter location code. Its name comes from the verified CFAgent oracle. |
| `0x00444170` | `GetLocationVarIDByName` | 109 | Case-insensitively scans the location-variable name table and accepts only an entry whose parallel level-ID slot equals the currently loaded level. Its verified CFAgent name is used by 97 canonical callers. |
| `0x00446110` | `Copy3DVector` | 25 | Converts a packed three-double source into a three-float destination and returns `this`; its name comes from the verified CFAgent oracle. |
| `0x0048bdc0` | `FindMonGenByName` | 100 | Traverses the world object's monster-generator pointer vector and compares the complete 32-byte generator-name field. Its verified CFAgent name is used by seven canonical callers. |
| `0x004f6b90` | `CreateWorldItem` | 179 | Allocates and zeroes the `0xad`-byte runtime item, initializes its packed item instance at `+0x09`, copies the position to `+0x15`, and optionally inserts it into the world-item list. The descriptive name is provisional; the complete implementation and layout are exact. |
| `0x004f6c50` | `SpawnItem` | 103 | Materializes a packed item instance unless the item ID is `-1`, passes it with a three-component position to the world-item allocator, and preserves the original `ItemManager.cpp:398` assertion. Its name comes from the verified CFAgent oracle. |
| `0x0050b830` | `GetNPCItemListByID` | 53 | Searches the global NPC item-list vector for an entry whose NPC record's `record_id` (offset `0x58`, independently corroborated by `config/types/wiz8/npc_database.h`) equals the requested ID, using the same odd `index < count` in-loop guard as `FindMonGenByName`. Its name comes from the verified CFAgent oracle and it has 73 canonical callers. |
| `0x00510b60` | `FindFirstMonsterByID` | 130 | Walks the global species `PList`, then the encounter `PList`, for a `W8MonsterGroup` whose `monster_id` field (offset `0x18`) matches; the species list is queried through `GetMonsterGroupByID` (`0x005101B0`) rather than the raw `PListGetAt` accessor the encounter list uses. Its name comes from the verified CFAgent oracle. |
| `0x00510bf0` | `FindNextExistingMonsterByID` | 206 | Continues a `FindFirstMonsterByID` search after a given `previous` result: locates `previous` in the species `PList` via `PListIndexOf` (`0x005E2890`) and resumes there, spills into the encounter `PList` once the species list is exhausted or `previous` was never in it, and starts the encounter `PList` fresh at index `0` unless `previous` was found there directly. Its name comes from the verified CFAgent oracle. |
| `0x005222d0` | `GetOriginOfCharacterItem` | 201 | Reports where an item pointer lives for a given party character: the 8-slot equipped array at `+0x1029`, the 12-slot carried array at `+0xf5d`, or a fixed-address, non-per-character shared item pool, each `0xc` bytes per slot. Its name comes from the verified CFAgent oracle. |
| `0x0046ded0` | `WorldUpdateProps` | 117 | Iterates the world's prop `PList`, invoking three `__thiscall` methods on each entry. Ported specifically as the layout proof for `W8World::plsProps`: the member's name and `PList` type come from the canonical assertion at `Engine Code\3d.cpp:344`, and byte-exactness fixes its offset at `0x08`. Descriptive name. |
| `0x0042e440` | `Octree::AddLoadedProp` | 118 | Appends to the octree's prop array and NUL-terminates it. Layout proof for seven `Octree` fields. |
| `0x0042e4c0` | `Octree::AddLoadedParticle` | 118 | The particle sibling of the above. |
| `0x0053bea0` | `TargetSourceIsCharacter` | 110 | Resolves whether a targeting source names a party character, accepting the either-type case only when a backfire or reflection flag is set. Layout proof for `W8TargetSource`. |
| `0x0053bf10` | `TargetSourceIsMonster` | 110 | The monster sibling of the above. |
| `0x004a8460` | `GrCycle::SetBehaviour` | 57 | Validates a behaviour against `BEHAVIOUR_FIRST`/`BEHAVIOUR_LAST` and stores it on the object returned by primary vtable slot 9. Proves the slot-9 vtable offset; establishes no `GrCycle` data member. |
| `0x004bfab0` | `Monster::GetNumSubsPerCycle` | 70 | Returns a cycle's sub count, substituting the current cycle for the `-1` sentinel. Its assertion message supplies the method name; layout proof for `Monster`'s `0xAC` cycle array. |
| `0x004e57c0` | `GetMonsterDataByID` | 115 | Lazily populates a 1000-slot cache of `0x297`-byte runtime `MONSTERS.DBS` records, allocating and loading through `LoadMonsterDatabaseRecord` (`0x0054A8A0`) on a miss and freeing again if the load fails. Its canonical assertion names the parameter `uiMonsterSpecies` and the bound `MAX_MONSTERS_IN_DATABASE`, and assigns the function to `Local Code\MonsterManager.cpp:1523`. Its name comes from the verified CFAgent oracle. |
| `0x004e5620` | `MonsterGetScriptPartByLocationIndex` | 245 | Bounds-checked accessor over `gXStatus.plsMonsterList`/`plsUnbornMonsterList`, dispatching on whether `monster_list_index` is `< 10000`, in `[10000, 20000)`, or `>= 20000` (the first and third cases share the same first-list path). Its name comes from the verified CFAgent oracle. |
| `0x004e5550` | `MonsterGetIndexByLocationID` | 199 | Linearly searches the same two `PList`s for a `W8MonsterInfo` whose `location_id` (offset `0x00`) matches, returning the encounter-list index offset by `10000`, or asserting/`-1` when `assert_on_failure` is set. Its name comes from the verified CFAgent oracle. |
| `0x00528a80` | `AddLinesToMessageBox` | 199 | Allocates and zeroes a `0x24`-byte message-line node, stamps its type/text/extra fields and a running sequence counter, and appends the node's pointer to a global grow-by-exactly-one pointer array. Its name comes from the verified CFAgent oracle. |

The owned definitions live in `src/wiz8/character_items.c`, `src/wiz8/gameplay_boundaries.c`,
`src/wiz8/random_number.c`,
`src/wiz8/location_variables.c`, `src/wiz8/spell_backfire.cpp`,
`src/wiz8/state_getters.c`, `src/wiz8/monster_generators.cpp`, `src/wiz8/monster_location.c`,
`src/wiz8/monster_lookup.c`,
`src/wiz8/npc_item_lists.c`,
`src/wiz8/item_spawning.cpp`, and `src/wiz8/vector_conversions.cpp`, and retain explicit `FUNCTION`
markers. `WIZ8_GAMEPLAY_BOUNDARIES` is a real VC6 CMake object target built by
`just build WIZ8_GAMEPLAY_BOUNDARIES`; it uses the pinned SP5 `/O2 /G6 /MD` environment alongside the
already exact compression and plug-in targets. The review map is
`config/analysis/reccmp/wiz8-gameplay-boundaries.csv`.

The target adds `/G6`, which is matching-relevant for this translation unit: it changes VC6's
instruction scheduling to the canonical order. With `/O2 /G6 /MD`, twenty-two bodies match exactly after
masking COFF relocations where needed:

| Function | Result | Relocation-normalized SHA-256 |
| --- | --- | --- |
| `VectorFromThreeFloats` | exact, 25/25 bytes | `8c7237c51fbcb1cbc3cd5e637a53cd257c04edf6f47958caf465882cf3af655f` |
| `RollDice` | exact, 53/53 bytes | `9e88bdc5744063e0d522ea20c95810352faed566df73a58552566ce96017ab63` |
| `IntegerPower` | exact, 24/24 bytes | `4454e70e52316ed73ef32bf813e14e16145c25fa3f402079afd9de08ecc375e8` |
| `ClampInteger` | exact, 25/25 bytes | `13755985809557446cbd5b7e269ef859eeb4fbaab4313ec284ad3d8232cf7b17` |
| `GetSpellTargetType` | exact, 47/47 bytes | `c801093b8ae5b3030e9db8e3d0fd1e4e6ad874ba8ea20eb0882f8802bce55dc5` |
| `MinimumCasterLevelForSpellLevel` | exact, 61/61 bytes | `649880237ae6e55d4df2f0ccfcc79f459e41a6bdb57711253ae74e907fa35228` |
| `GetMinimumCasterLevelForSpell` | exact, 85/85 bytes | `bb490ccad29b8f851d5c0f3e8ee00b2c3cc7f778aa3f671f2a620e4974094758` |
| `GetFactionDisposition` | exact, 100/100 bytes | `8166d9a0a4cfcb3ed073f966e33115d60f4512f670bcb6785a5300e114a113f4` |
| `StripMonsterNameSuffix` | exact, 26/26 bytes | `fcc9db3bf744139df99cc283507aff3d58c1deb75cf8bedbb4a3beef5e5698cf` |
| `CharacterPointerToPartySlot` | exact, 105/105 bytes | `ba51c3d9a068fa8b79e4fbe5fb88e160d773ee10dbbe561891b0a9186aaff725` |
| `GetProfessionCasterLevel` | exact, 82/82 bytes | `a9d9c20d53f78c3eaf03ef10a24468a2cb2a63254148cefe7bf67ca06526a75e` |
| `GetFact` | exact, 132/132 bytes | `bd49cf8863d372e24a72e6ad01c04fd9772d3bb8726df41cf5aa5d1a9870db33` |
| `SetFact` | exact, 219/219 bytes | `929c978d5088d599d032ddca2689f8d01b8adb0526aaa15a3de0621f77f760cf` |
| `GetRandomNumber` | exact, 50/50 bytes | `438ef441f48956b35998a4c1aa63c4da1d0a0fc45436883376b4a7bc849897bb` |
| `GetLoadedLevelID` | exact, 6/6 bytes | `76811197299fd7215ff45276752d25eaa8889353ee70ada1fd839c8a55d34ffc` |
| `GetWorld` | exact, 6/6 bytes | `76811197299fd7215ff45276752d25eaa8889353ee70ada1fd839c8a55d34ffc` |
| `GetItemInHand` | exact, 19/19 bytes | `b9095c14c2ad5c66b33be97c13da5f36816cee38f6b3d5faff013d7909fd2c14` |
| `LevelGetFolderNameByID` | exact, 33/33 bytes | `0e02b69da5480ffc3bd971ad5330ef56843d43cae98b4f89dc8c42f212cc25d7` |
| `GetLocationVarIDByName` | exact, 109/109 bytes | `d0e75111187a799a90b2c25b05469b9acd6a91b11984a5582c4bca7f35b800d9` |
| `Copy3DVector` | exact, 25/25 bytes | `d2c6b969f7e840e9b66f67645f81ca73443a3440a6172050a5bb642d96fd1f9c` |
| `CreateWorldItem` | exact, 179/179 bytes | `9f728370ebab904577b40d6b5b75d5ce4a963ad1eb8f57f05ddb4eef98280c94` |
| `LevelGetLocationCodeByID` | exact, 76/76 bytes | `0a8de1a7b2ca5e8d67f1522337732bcb5fe4dc3fc33cde6051de5f9ed864240a` |
| `GetLocationIDFromCode` | exact, 231/231 bytes | `432b1896821b8cbe7a262da430417dc336cc59f13a23bf043f9e493bed8bda2b` |
| `GetMonsterDataByID` | exact, 115/115 bytes | `5976911974dc4ad7e3a908e1f90024b0ecf8f83c6fa7d6e835833f54fc4fab4b` |
| `GetRandomCharacter` | exact, 218/218 bytes | `f881acd9b355a6e4aac0b6dd77781a1e5d01e4e6e7872ba6e0c5026f7f8f0908` |
| `SpawnItem` | exact, 103/103 bytes | `b2b9177917fde0f54d3033f2cf8deaf6ddc44fa7546707e7ecbd912f5ec9e120` |
| `GetNPCItemListByID` | exact, 53/53 bytes | `e293f0561b674646f81354fe8f56c2a05f4ec43fa755416032def319cdfff793` |

`GetRandomNumber` is the first proven exception to the unit's `/G6` scheduling. Its isolated
`random_number.c` object is exact with `/G5`; `/G6` preserves the semantics but moves the multiply
constant load and zeroing instruction. This is kept as a per-source CMake option rather than
weakening the exact `/G6` evidence for the other twenty-one bodies.

`CanSpellBackfire` is retained because the typed semantics and all exception sets are grounded in
the canonical body, but it is intentionally classified as `structurally-strong`, not exact. The
current VC6 output emits a separate true-return block for one nested switch. That unresolved
code-layout difference is recorded instead of being hidden behind a misleading hash.

`FindMonGenByName` is likewise retained as `structurally-strong`. Its typed world and vector layout,
loop bounds, fallback element selection, and fixed-width `strncmp` all agree with the canonical body,
but VC6 schedules the invariant name/import loads ahead of the initial count comparison and places the
success epilogue differently. No exact hash is claimed for that layout mismatch.

`GetNPCItemListByID` shares `FindMonGenByName`'s odd `index < count` in-loop element guard, but its
body is small and simple enough that this recovery is byte-exact once the vector pointer and the
element count are cached in the same registers the original used (`ESI`/`EDX`); declaring the count
local before the vector-pointer local was what flipped VC6's register choice to match. The exact hash
was independently confirmed against the original bytes read straight out of the imported `Wiz8.exe`
Ghidra program (`wiz8 ghidra query <program> read-data <address> <size>`), not just the disassembly
text. That query command had a latent bug: it filled a plain Python `bytearray`, which JPype copies
rather than shares across the Java call boundary, so `Memory.getBytes` always returned all zeros; it
now uses a `jpype.JArray(jpype.JByte)` so raw-byte reads work for future comparisons too.

`FindFirstMonsterByID` is retained as `structurally-strong`. Its two `PList` traversals, the
species-list dispatch through `GetMonsterGroupByID` versus the encounter list's direct
`PListGetAt`, and the `monster_id` comparison at `+0x18` all agree with the canonical body
instruction-for-instruction — including the exact `JBE` fallthrough once `PListGetCount` is typed
`unsigned int` to match the original's unsigned entry check. The one remaining difference is loop
peeling: the current VC6 output duplicates each loop's first iteration into its own code block
instead of reusing the original's single body through a backward branch. Unlike `GetNPCItemListByID`,
swapping which local occupies which register did not change this; it appears to be a loop-shape
heuristic rather than something reachable through simple reordering.

`FindNextExistingMonsterByID` shares that same limitation and is also `structurally-strong`. Its
`PListIndexOf` callee (`0x005E2890`) is a useful case study in cross-checking Ghidra evidence:
decompiled at its own address it appears to take three parameters, but decompiled at this function's
call sites it only ever passes two; the standalone signature is a Ghidra recovery artifact from the
callee's `srAssertFail` prologue, and the correct two-argument `(list, target)` signature is confirmed
by manually walking the raw disassembly's `ESP`-relative operand offsets against the actual push
count at each read. Every branch in the recovered body — both directions of the `position < count - 1`
check, the `position == -1` fallback into the encounter `PList`, and the shared `search_species`/
`search_encounter` labels reached by both fallthrough and `goto` — agrees with the canonical
control flow; only the loop-peeling and parameter-to-register assignment differ, the same open gap
as `FindFirstMonsterByID`.

`GetOriginOfCharacterItem` is `structurally-strong` for the same reason, but its recovery also
needed its own Ghidra cross-check: decompiled directly, Ghidra reports five parameters and multiplies
the *second* one (the item pointer) by `0x1862`, which cannot be right for a pointer being compared
by identity a few lines later. Walking the raw disassembly's `ESP`-relative reads against the actual
push count at each point shows the truth is four parameters, with the *first* (`character_index`)
driving the `0x1862` (`W8Character` stride) strength-reduction sequence and the second (`item`) used
only for pointer comparisons — the `pPCItem != NULL` assertion text and `PC Item.cpp` source path
back this reading up directly. With that resolved, and `g_shared_item_pool_count` typed `unsigned int`
to get the original's `JBE` bounds check instead of a signed `JLE`, the recovered body matches
instruction-for-instruction: the multiply sequence, the `+0x1029`/`+0xf5d`/shared-pool slot bases,
and all three linear scans. Only the same character_index/item register-role swap and loop peeling
already seen on the monster-lookup functions keep it from being byte-exact. The equipped/carried slot
counts (8 and 12) and the shared item pool's exact scope are read directly from the bytes; their
descriptive names are provisional.

`MonsterGetScriptPartByLocationIndex` and `MonsterGetIndexByLocationID` are a second `PList`-backed
pair alongside the monster-lookup functions, this time over `gXStatus.plsMonsterList` and
`plsUnbornMonsterList` (named from the assertion text embedded at their call sites, e.g.
`uiMonsterListIndex < (UINT32) PLLength(gXStatus.plsMonsterList)` and `MonsterManager.cpp`) rather
than the species/encounter group lists. The first version of `MonsterGetScriptPartByLocationIndex`
used a plain `index < 10000` / `else` split and was a genuine correctness bug, not just a byte
mismatch: the original's raw disassembly does two range checks (`CMP ESI,0x2710` / `CMP ESI,0x4e20`)
and routes indices `>= 20000` back through the *first* list's path, identical to how it treats
indices `< 10000`. `PListGetAt`/`PListGetCount` were also widened from returning `W8MonsterGroup*` to
a generic `void*`, since the same two accessor addresses (`0x005E2870`/`0x005E2C70`) are called
against both this pair's lists and the earlier monster-group lists — this is a genuinely
type-erased `PList`, not a per-element-type template instantiation. Both functions are
`structurally-strong` for the same register-role and loop-peeling reasons as the rest of this
section.

`AddLinesToMessageBox` is the first function in this file outside the `PList`-backed lookup family,
and it is byte-exact. Its parameters are recovered from its 8 call sites (e.g. `0x004EEFFD`, which
builds a `swprintf`-formatted wide string via `operator_new(0x400)` and a small 8-byte extra-data
node before calling in with a literal type constant): `type`, a wide `text` pointer, and an `extra`
payload pointer. The body allocates a `0x24`-byte node with `new`, zero-initializes it, stamps four
fields plus a running `g_message_sequence` counter, then appends the node's pointer into a global
`W8MessageBoxLine**` array that grows to exactly `count + 1` (never doubled) whenever the existing
capacity is too small. Matching the original exactly needed two adjustments past a first working
draft: comparing `new_capacity > g_message_box_line_capacity` instead of the mathematically
equivalent `g_message_box_line_capacity < new_capacity` (VC6 preserves the source's operand order in
the emitted `cmp`, so the flipped comparison produced `cmp eax,edx`/`jge` instead of the original's
`cmp edx,eax`/`jle`), and reading the old array pointer only inside the grow branch instead of
caching it before the capacity check (matching the original's lazy load at `0x00528ACF`, which
Ghidra's own decompile pseudocode does not preserve since it reorders reads that have no
control-flow dependency). With both fixes, a standalone `WIZ8_GAMEPLAY_BOUNDARIES` build's object
code is byte-identical to the canonical function after masking its 17 global/import-call
relocations, confirmed by extracting the compiled bytes directly from the `.obj`'s `.text` section
and comparing against the original bytes read out of the imported Ghidra program.

The two Ghidra auto-analysis signature artifacts found above — `PListIndexOf` (`0x005E2890`) and
`GetOriginOfCharacterItem` (`0x005222D0`) — are corrected in the Ghidra project itself, not just in
this document's prose, by:

```sh
just ghidra apply-wiz8-signature-fixes wiz8--gog-base--wiz8--18a74ff61c65
```

`tools/wiz8decomp/ghidra/apply_wiz8_signature_fixes.py` applies both corrected signatures via
`ApplyFunctionSignatureCmd`, so a fresh analysis session (or a re-import) sees the true parameter
counts and types instead of the standalone-decompile artifacts described above.

## What `GetLocationIDFromCode` cost, and why it is worth recording

The body converged only after two source-shape constraints, both of which are evidence about the
original rather than compiler trivia:

* The lookup loop has to be a separate `__inline` helper that returns `-1` on exhaustion. Writing it
  as an inline `for` with a post-loop bound test adds a redundant `index >= 47` block; writing it
  with a `goto` instead lets VC6 range-propagate the result and *delete* the original's retained
  `level_id == -1` and `level_id >= 57` guards. Only the inlined-helper shape reproduces both. Those
  two dead guards are therefore a fossil of a lookup helper Sir-Tech had factored out, and the
  57-vs-47 mismatch says its caller was written against a larger location-ID space than the table.
* The two emptiness checks have to be one short-circuit `||`. As separate `if`s the compiler
  duplicates the `return -1` epilogue; the original shares a single one.

Intermediate attempts measured 261, 162 and 247 bytes against the canonical 231, and the register
allocation (`ebx` index, `esi` cursor, `edi` callee pointer) fell out on its own once the control
flow was right. That is worth noting against the seven `structurally-strong` entries whose remaining
gap is described as a register-role swap: at least in this case the register roles were a *symptom*
of the control-flow shape, not an independent problem to solve.

Both new functions are absent from the demo at any address, as is the already-mapped
`LevelGetFolderNameByID`. The demo's level-table accessors are a different shape, which is
consistent with it shipping a smaller table, and is recorded rather than treated as a failed match.
