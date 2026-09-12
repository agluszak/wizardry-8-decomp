# Recovery handoff: level runtime, sight, NPC and combat batch

This note preserves the reconnaissance gathered for the second large recovery batch so the
next session does not repeat it. It is a handoff, not a specification: where a fact here is
not backed by retail evidence, treat it as a lead to confirm, not as truth.

## Resuming

- Repository state when this was written: `main` at the commit that recovers
  `Function482410` plus the cast-marker follow-up. Work continued from a clean, empty
  working copy on top of `main`.
- Use Jujutsu per `docs/contributor-workflow.md`; direct-to-main is authorized by the task
  owner. Other agents commit to `main` concurrently: always
  `jj git fetch --remote origin` and rebase before pushing.
- Load the `matching-decomp` skill first. The comparison policy for this batch is
  **structural exactness, not byte exactness**: `uv run wiz8 compare ADDRESS...` is the compile and
  sanity check, and compiler-shape differences are acceptable and expected.
- Recompiling runs both clang and the pinned VC6 product. VC6 catches things clang does not:
  `goto` across an initialization, duplicate `for (int index = ...)` names in one function,
  missing `<stdio.h>` for `sprintf`, and `wchar_t` without `<wchar.h>`.
- The cast gate checks only casts added by the current diff. A new `reinterpret_cast` needs
  `/* reinterpret-ok: reason */` **on the same source line as the cast token**, or an
  evidence-backed typed replacement.
- Disposable artifacts from this batch are in `build/`: `build/context/ADDRESS.cpp`
  (decompiles) and `build/asm-ADDRESS.txt` (retail disassembly). `build/` is disposable and
  must not be committed.

## Evidence corrections that apply broadly

- In all level/NPC bodies the decompiler's `g_status_685170.saved_level` is actually
  **`g_loaded_level_id` (0x00686A70)**, declared in `include/wiz8/location_variables.h`. The
  retail code compares against that global directly.
- `g_status_685170.unknown_000c._6348_4_` is the model's `world_clock` field; the address is
  0x686A48.
- `g_status_685170.unknown_1904._2691_4_` is `game_time_ms` (0x2387);
  `unknown_1904[0xa29]` is `value_232d` (0x232D) and `unknown_1904[0xa8b]` is `flag_238f`
  (0x238F), both already carved into `W8GlobalStatus`.
- `g_current_screen_state` comparisons use `W8_SCREEN_MAIN_GAME` (`include/wiz8/screen_state.h`),
  value 7.
- Constants already declared: `g_float_005ebb34` = 0, `g_sight_default_005ec254` = 12.0,
  `g_monster_facing_tolerance_005ec2b0` = 0.785398, `g_float_005ebc64` = 1000.0,
  `g_navigator_vertical_phase_step_005ebcc8` = 0.25, `g_movement_speed_step_005ed490` = 0.01,
  0x5EC3A0 = 25000.0, 0x5EC150 = 500.0, 0x5ED7F8 = 0.6666667.
- Function-local overloads of the shared no-op stub at 0x004023A0 are the established
  pattern for calls the decompiler shows as `NoOp` with arguments (`Bink.cpp`, `PathAI.cpp`,
  `Levels.cpp`). They cost a call-target mismatch, which is accepted for this batch.

## Names already applied (do not re-derive)

`Function42B020` -> `LoadSkyWorld0042B020`; `Function42B720` -> `GetLevelCdNumber0042B720`;
`Function42B6F0` -> `IsLevelCdMissing0042B6F0`; `unknown_69` -> `cd_number`;
`Function451020` -> `UpdateWorldMeshAfterLoad00451020`;
`Function489920` -> `ReleaseRetainedMaterials00489920`;
`Function48CBE0` -> `ResetMonsterGeneratorTimers0048CBE0`;
`Function48DB30` -> `ReleaseWorldCursorNodes0048DB30`;
`Function4909C0` -> `ReleaseWorldCursor004909C0`;
`Function5060C0` -> `ResetAndRefreshAllSight005060C0`;
`Function53BF80` -> `RefreshAllPartyTargets0053BF80`;
`Function50DB50` -> `ResetNpcBindingsForParty0050DB50`;
`Function50C270` -> `ClearPendingNpcLevelFlags0050C270`;
`Function50C2E0` -> `ReleaseNpcMonsterBindings0050C2E0`;
`Function50DA00` -> `ReleaseMarkedNpcBindings0050DA00`;
`Function50AC60` -> `RebindNpcLevelTriggers0050AC60`.
Also: `Function514DF0` -> `MeasureLevelStatusChunks00514DF0`,
`W8Chunk::SetCurrentChunkAtEnd`, `W8MaterialMapper00482010` in `Environment.h`/`Environment.cpp`.

## Remaining work, in recommended order

### 1. `Function4D6C50` (0x004D6C50), `MasterFunctionList.cpp`

- Decompile: `build/context/004d6c50.cpp`. Signature `void Function4D6C50(int level)`.
- Value: the per-level trigger/master-function setup; `LoadLevel` calls it once. It consists
  almost entirely of `FindTriggerByName` lookups (each followed by an `srAssertFail` when the
  trigger is missing) over every level plus `GetFact`/`SetFact` pairs and a set of
  master-function helpers (`FUN_004d9a70`/`FUN_004d99e0` are new/delete,
  `NormalizeMasterFunctionValue004D9700`, `Function4D9740`, `Function4DA670`,
  `Function4DB200`, `Function4DBAB0`, `Function4DBE70`, `Function4DC8D0`,
  `Function4DCB50`, `Function4DEB40`, `Function4E0510`, `Function4E06D0`,
  `Function4D9B40`, `Function4D9D30`, `Function4D9A70`, `Function521060`).
- No new modeling is expected; the helpers above are address-named unless a body is
  recovered. The existing file already has the dispatcher and several of these helpers; the
  missing producers should live in their current owner files.
- This is a long transcription. Work level by level and compare after coherent chunks.

### 2. `ResetAutomapView005817D0` (0x005817D0), `AutomapScreen.cpp`

- Decompile: `build/context/005817d0.cpp`.
- Already modeled: `g_automap_state` (0x68F268, 0xFC), `g_automap_redraw` (0x68F25C),
  `g_bits_68f288`, `g_octree_6598a4`, `g_automap_top_y` (0x68F204),
  `g_automap_position`, bounds vectors, `g_automap_layer` (0x64B918).
- Needs naming (evidence is the body itself): float at 0x64B910 (30000.0 for saved level
  0x18 or 0x1A..0x22, otherwise 10000.0), float at 0x64B914, int/byte at 0x64B91C, the
  float triples at 0x68F1C8/0x68F1D8/0x68F1F8/0x68F240, and a flag at 0x68F290.
- Helpers: `FUN_0046CE30(&min, &max)` (camera-derived world bounds),
  `FUN_004D99E0(0)` (pool release), `FUN_005853A0(float*)` returns a packed cell index,
  `BitArray::Set(g_bits_68f288, index)`, `GetCameraPosition`,
  `srVector3T<float>::method_00421650` (half-vector helper).
- Behavior: allocate/zero the 0xFC automap state; drain the automap object list (freeing a
  field at +0x0C and calling `FUN_004D99E0(0)`); raise the redraw flag; set the view scale
  from the saved level; initialize the world bounds from the octree or the camera; pack the
  camera position into the bitset via `FUN_005853A0`.

### 3. `Function452F50` (0x00452F50), `Navigator.cpp`

- Decompile: `build/context/00452f50.cpp`. Signature `void Function452F50(char param_1)`;
  `ResetSight` calls it with 0.
- Already modeled: all `W8Navigator` fields it touches (`path_ai_068`, `flag_025`,
  `unknown_0bc`, `movement_0c0.location_id_004/yaw/position_040/attachment_0ac`,
  `position_03c`, `linked_update_time_0b8`), `PathAIResetTick004A9C20`,
  `FUN_00510CC0`, `CopyPathFrom004564F0`.
- Needs naming: globals 0x659C10 (current mode byte), 0x659B34/0x659B3C (navigator count and
  array), 0x659BF8/0x659BFC/0x659C04 (a `W8GrowableVector<W8Navigator*>`'s
  capacity/count/data), 0x6081E4 (flag); helpers `FUN_00511050` and
  `W8Navigator::CollectGroupNavigators`.
- Behavior: mode 0 resets each navigator's path and group notification; mode 1 sets the
  navigator flags at +0x25/+0xF4/+0xF8/+0xFC/+0xBD.

### 4. `Function50E700` (0x0050E700)

- Decompile: `build/context/0050e700.cpp`. Calls five unrecovered `50xxxx` helpers
  (`FUN_0050EDC0`, `FUN_0050EF50`, `FUN_0050F090`, `FUN_0050F090`, `FUN_0050F090`) plus
  `FUN_00547940`, `FUN_004ED9D0`, `SetSkyNodeVisible`, `SetCameraLightIntensity00483E30`. Recover the
  helpers first or confirm their argument shapes from the call sites; then transcribe.

### 5. `Function4EA310` (0x004EA310), `Combat.cpp`

- Decompile: `build/context/004ea310.cpp`. `void Function4EA310(int mode)`; `UnloadLevel`
  passes 1.
- Nearly pure orchestration over recovered callees: `BeginFreeTurnPhase`,
  `RemoveConditionFromEveryone`/`FromParty`, `ProcessMonstersAtCombatEnd`,
  `GetMonsterGroupByListIndex`, `MonsterGroupLeaveCombat`, `CountActiveCharacters`,
  `RestoreCombatFormation`, `UpdateScreenOverlays`, `ClearSurfaceRect`, `RequestRedraw`,
  `SetFlag6081E4`, `ResetLivingMonstersAfterCombat`, `ClearLevelDataFlags5To7`,
  `RequestRedrawParty`, `SetFloat60AB48`, `ReportSaveFailed`, `ShowNotice` (declared),
  plus address-named `FUN_005a1890`, `FUN_0053ae00`, `SetTargetingMode`, `FUN_00524540`,
  `FUN_00552530`, `FUN_004eef10`, `FUN_004f0560`, `FUN_0053cd60`, `FUN_00517780`,
  `FUN_005a3470`, `FUN_0059bb70`, `FUN_0058f6b0`, `MonsterForward4531A0`,
  `FUN_00482990`, `FUN_004ee9d0`, `FUN_0056aab0`, `FUN_004ea1f0`.
- `RequestRedrawCombatBar`, `SetCountdownClock`, `DisableMainRegionSet` should be in headers
  already; check before declaring.

### 6. Combat decision core

Request context for each address with
`uv run wiz8 report context ADDRESS`, then recover in `Local Code\Combat.cpp` (or the owner
the report gives):

- `ChooseCombatAction()` 0x004E77B0 (declaration-only at HEAD);
- `CharacterCanSwitchTo` 0x004E79A0 — already declared in `combat_state.h` as
  `unsigned char (int, int, int, int)`; `Combat.cpp` still has a duplicate local extern to
  remove when its body lands;
- `Function4E7CC0()` 0x004E7CC0; `Function4E8000()` 0x004E8000;
- `SwitchCharacterTo()` 0x004ED390; `Function55EE30(int)` 0x0055EE30;
  `MonsterChooseTarget()` 0x0051AC30.
- These sit under the already-recovered `GetCharacterTurnValue` and
  `DropCharacterFromRound`; `DropCharacterFromRound` delegates its re-selection/reset through
  `Function55EE30` and `Function4E8000`, so recovering those explains it.

### 7. Dialog completions

- `W8Dialog005CD710`: finish `CreateButtons005CD8D0`, `RefreshScrollButtons005CE420`,
  `HandleInputEvent005CEC20`.
- `W8Dialog005CBB40` (type-3 "ListBox Dialog", vtable 0x005EF7C8, size 0xFC, only
  `GetDialogType` present): constructor, destructor, `CreateControls`, `DestroyControls`,
  `Draw`, `SetText`, `ProcessInput`. Use `uv run wiz8 vtable 0x005EF7C8` for the slot map.
- `W8Dialog005D97D0`: `CreateControls`, `Draw`, `ProcessInput`, `OnNumericInputChanged`
  (ctor/dtor/`DestroyControls` already present).
- Locate each class under `src/wiz8/dialog_code/`; the factory dispatch table in
  `DialogFactoryDialogs.cpp` names the types.

## Working agreements

- Keep one coherent change per Jujutsu commit; the task owner expects periodic direct pushes
  to `main`, not a single final publication.
- Rename a function as soon as its behavior is established (the list above is the precedent);
  update every call site and the owner header in the same change.
- Do not use `config/verification/unresolved-baseline.csv` as a work queue; it is stale.
  Decide whether a body is missing from current definitions plus current call sites.

## Session log

### Item 5 reconnaissance (next session can transcribe directly)

`Function4EA310` (`EndCombat004EA310`, callers include `ToggleCombatMode`, `UnloadLevel` and
`FUN_004E9F90`) reaches the monster-manager trailing storage through these already-modelled
standalone globals: `[0x1C]` = `g_in_combat_00683F94` (0x683F94), `[0x1D]/[0x1E]/[0x1F]` =
`g_flag_00683F95/96/97`, `[0x55]` = `g_flag_00683FCD` (0x683FCD), and `unknown_9C7[0x3D]` =
`g_flag_006840BC` (0x6840BC). Still unmodelled: the two `W8IList*` at 0x683FAD (`_53_4_`, the
combat monster list read by `MonsterChooseTarget`) and 0x683FB1 (`_57_4_`, the combat group
list), the byte at 0x683FCE, and the int at 0x6850B0 that receives
`SetCountdownClock(120000)`. The 1000-dword pass at 0x688291..0x689231 rewrites each `1` to
`2`; it lives inside `W8GlobalStatus::unknown_2498` (offsets 0xC89..0x1C29). The combat-state
fields used are `flag_a54`, `pending_move_kind` (0x90C) and `unknown_a55[0xC]` (0xA61);
`DAT_0068C09C + 0x8CC` supplies `ShowNotice`'s notice string and still needs a model.

### Earlier session work

- The canonical Ghidra state was regenerated from the freshly built VC6 PDB with
  `reccmp-ghidra-import` (`b8f37c59`): 1429 functions changed, 3626 entities imported,
  function count 7701 -> 7737. Recovered names, signatures and types are now visible to
  `report context` and the recovery decompiler; `FUN_` spellings in older `build/context`
  dumps are pre-import output.
- `Function452F50` was recovered as `SetNavigatorLinkMode00452F50` (`88b80ff0`) with the
  bool-returning `PositionMonsterGroupNearCamera00511050` declaration added next to
  `Function510CC0`.
- Item 2 owners: `SetFloat64B914`/`GetFloat64B914` and `g_float_64b914` (2000.0) are already
  recovered in `AutomapScreen.cpp`. Of the remaining unnamed automap globals, `0x64B910` is
  also read by `0x005809F0`, `0x64B91C` is also written by `0x00580380`, `0x00581E60` and
  `0x00584690`, and `0x68F240` is also read by `0x00580380`, `0x005807B0` and `0x00581B30`;
  the setup-only triples at `0x68F1C8`/`0x68F1D8`/`0x68F1F8` do not overlap the drawing
  bounds `g_automap_bounds_min` (0x68F210) and `g_automap_bounds_max` (0x68F1B8).
- Item 4 belongs to `Local Code\Gameplay Mods.cpp` (`0x0050E8C0` and `0x0050E980` report
  that unit), which still has no source file, so it needs a unit container rather than a
  transcription into `Magic Effects.cpp`. `Function50E700` additionally needs models for
  the effect block at `0x687453`, its source list at `0x68691F`, the `W8GlobalStatus` bytes
  at `0x22E3`..`0x232A`, and the combat-state offsets `+0x7C6`/`+0x85A` reached through
  `g_combat_state` (0x006836A8; `Missile.cpp` used the wrong name `g_dialog_state_006836a8`
  until `2f505727`). Block helper `0x0050F090` is the 0x67-byte field-wise adder.


