# Recovery handoff: spell visuals, sound events, dirty tiles, shader walks

State at `14528d78` (`Recover the dirty-tile recursion and the scene shader
walkers`). `just check` is green. This is the continuation of the change stack

| Change | Title |
| --- | --- |
| `c7b40f65` | Resolve the pointer-vector placeholders and receiver-only functions |
| `c9533ef1` | Name the navigator bodies and recover the environment colour application |
| `b7d50afd` | Name the frustum helpers and recover the plane and projection tests |
| `f01a8f23` | Recover the spell-visual spawn path |
| `14528d78` | Recover the dirty-tile recursion and the scene shader walkers |

Upstream also landed `4bd8d909` (Video2 tail helpers, split headers folded) and
`2f4de1ea` (shared `g_double_005ebf60`) inside that range.

Decompilation dumps made during this work are disposable and live under
`build/cleanup/recovery2/`, `recovery4/`, `recovery5/`, `recovery6/`, plus the
older `build/context/`. Focused comparison JSONs are in `build/cleanup/`.

## What is already done

Recovered bodies (compare score at handoff):

| Address | Identity | Status |
| --- | --- | --- |
| `0x0044EBE0` | `W8Prop::GetAnimationState0044EBE0() const` | exact |
| `0x00452630` | `W8Navigator::ConfigureMovementToPosition00452630` | exact |
| `0x00452C90`/`0x004526C0` | `ResetMovementAndGroupState00452C90`, `SetMovementTargetToNavigator004526C0` | 0.746 |
| `0x00453CA0` | `W8Navigator::SetPitchRollEnabled00453CA0` | exact |
| `0x0046D7D0` | `SetOctreeGameData0046D7D0` | exact |
| `0x0046E860`/`0x0046E880` | `ForwardThroughMember3C_*` | 0.875 (call-target layout) |
| `0x0046E640`/`0x0046E750` | scene shader walks | 0.381 / 0.347 |
| `0x004259B0` | `InvalidateDirtyTile004259B0` | 0.383 (was 0.071) |
| `0x00483D70`/`0x00483BA0` | `ScaleColourAndSaturate00483D70`, `ApplyEnvironmentColour00483BA0` | 0.618 / 0.380 |
| `0x004301C0`/`0x004302E0` | `W8Octree::MarkVisibleRegions004301C0`, `BuildFrustumPlanes004302E0` | 0.352 / 0.322 |
| `0x0046D880`/`0x0046D660` | `PointInsideFrustum0046D880`, `BuildPlaneFromPoints0046D660` | 0.407 / 0.103 (see TODO) |
| `0x004AD430`/`0x004AC530` | `SpawnSpellEffect`, `W8SpellVisual::FindSupportedCycle004AC530` | 0.27 / 0.349 |
| `0x004D5770`/`0x004D57A0` | `W8SoundEvent` destructor/factory | ~0.95 / ~0.99 |
| vectors | `W8SpellVisual*`, `W8Missile*`, `W8SoundEvent*`, `W8TriggerEvent*`, `W8World*`, `W8EncounterTableRuntime*`, `W8GrowableVector<W8GrCycle*>*`, `stSound3D*`, `W8SpellEffectEntry*`, `W8Searchable*`, `W8MonsterGroup*` | vtable exact |

`W8SpellVisual`'s vtable became exact when
`FindSupportedCycle004AC530` filled the slot after `StartIfHostActive`; treat
that as the reference example for "missing virtual slot" triage.

## Open item 1: `LoadSpellVisualResource004AB580`

- Callers: `SpawnSpellEffect` (`0x4AD430`) twice, `0x4FB4C0` four times.
- Decomp: `build/cleanup/recovery5/004ab580.c` (258 lines).
- What it does: finds a GrCycle by the resource name, allocates one 0x1f8
  `W8SpellVisual`, constructs it, loads/parses the named bitmap/cycle data,
  builds the `W8SpellEmitterHost`, registers the cycle, and wires the sound /
  shake hooks.
- Observed dependencies: `FindFirstGrCycleByName`, `RegisterGrCycle`,
  `ReadCycleData004AB340` (recovered), `W8GrObject::AddSoundEvent`,
  `AddShakeEffect004A8530`, `FileOpen`/`FileClose`, `sscanf`/`sprintf`,
  `IncrementValue60DFAC`, `PauseSharedGameTimers00439BC0` /
  `ResumeSharedGameTimers00439CA0`, and the still-open `FUN_004CEE40`,
  `FUN_004A67E0`.
- Next step: read the full decomp and transcribe; declare the loader's own
  seams in `Spells.h` (its owner) with address-only names. `SpawnSpellEffect`
  already carries the recovered load/fallback flow, so this function completes
  that caller chain.

## Open item 2: `UpdateSoundEvents004D5890` and player `0x4D5A10`

- `UpdateSoundEvents004D5890` is declared in `SoundEvent.h` and called from
  `GrCycle.cpp` (`TickAnimation`, `ApplyPendingCycle`). Decomp:
  `build/cleanup/recovery5/004d5890.c` (88 lines).
- Candidate-vector globals: object at `0x683408`, count `0x68340C`, capacity
  `0x683410`, data `0x683414`; selected index `0x683418`; last-selected index
  `0x61095C`. Model the object as `W8GrowableVector<W8SoundEvent*>` (its `Grow`
  emission folded to the int specialization).
- Selection logic recovered: for each bit of `event_mask`; scan every event and
  require its kind dword (`W8SoundEvent::value_000`) to match the bit and the
  mask; kind 1/`0x100` matches when
  `(value_004 == -1 || value_004 == cycle) && value_008 == frame &&
  value_00c == subcycle`; kind 2 matches `value_004 == cycle &&
  value_00c == subcycle`; append matches to the candidate vector; pick
  `Random(count)` avoiding an immediate repeat of `0x61095C` while count >= 2;
  call the player; store the chosen index on success; shift the bit; return 1.
- Player `0x4D5A10`: decomp `build/cleanup/recovery5/004d5a10.c` (209 lines).
  **ABI is the blocker**: the Ghidra prototype shows three stack arguments
  (`__thiscall(int* event, undefined4, float* position, int)`), but the retail
  call site at `0x4D59DC` pushes five stack values plus ECX. Correct the Ghidra
  prototype from the call site before transcribing; do not guess the extra
  arguments.
- Player dependencies: `Sound3DPlay`, `TrackSoundHandle004CA6E0`,
  `BuildFootpathPath0047A540`, `FUN_00420CA0`, `GetFlag6850F6`, `Chance`,
  `IsAmbientSoundMuted`, `GetMonsterByLocationID`, `GetCameraPosition`,
  `GetCameraYawRadians`, and `W8CameraMatrixRow004D6930`.

## Open item 3: `ReleaseMeshModelInternal004729F0`

- Declared in `stMeshModel.h` as `void ReleaseMeshModelInternal004729F0(
  stMeshModel*)`; called by the `ReleaseMeshModel` forwarder at `0x473180`.
- Decomp: `build/cleanup/recovery2/004729f0.c` (270 lines).
- **Signature blocker**: Ghidra reports
  `__thiscall(srMeshModel* param_1, int param_2, undefined4 param_3)` with two
  stack arguments, while the existing declaration and forwarder pass one.
  Disassemble `0x473180` first and reconcile the real argument list.
- Body outline: allocates a polygon remap through
  `getPolyVertex` / `srHeap::allocate(polygon_count_230 * 0xc)`, rebuilds the
  vertex remap via helper `FUN_00471930`, and rewrites the polygon/vertex
  tables under `param_1[1].unknown_1c_ + 0x20`. Several `stMeshModel` table
  fields are only partly modelled, so type those before transcription.

## Backlog (lower priority)

- Unresolved vector element types: `0x005ECA5C`, `0x005ECAD0` (ctors/dtors
  recorded, no element users), `0x005ED2C8` (`W8MonsterRep` field at `+0xAC`),
  `0x005EF08C` (OptionsScreen factory vector). Facts live in
  `src/wiz8/vector.cpp` prose and
  `evidence/observations/wiz8/ptr-vector-instantiations.csv`.
- `BuildPlaneFromPoints0046D660` carries a `// TODO` in `src/wiz8/engine_code/3d.cpp`:
  the `srVector3T` API form (`Length`/`DotProduct`/`Set`) matches retail at
  ~0.10 because the original keeps compiler-lowered counted loops; restore the
  loop form if byte fidelity there matters.
- Intentionally address-named, body known-faithful but not exact:
  `Function421100`/`Function421150` (GDCamera wrappers), `Function443A50`,
  `Function44D760`, `Function44E830`, `W8Prop::Function44DEA0`,
  `Function479030`, `Function4836A0`, `Function48F280`, `Function4B5780`,
  `Function4D9080`, `Function5588E0` remain deliberately unchanged.
- Verification debt: `wiz8 verify` is broad and not run; the unresolved
  baseline (`config/verification/unresolved-baseline.csv`) is stale, and the
  global vtable gate has ~20 pre-existing mismatches in classes untouched by
  this work.

## Working notes

- Recovery loop and verification live in `AGENTS.md` and
  `.agents/skills/matching-decomp/SKILL.md`. Use `just compare ADDRESS...` for
  focused checks, `just check` for the fast lane, and
  `uv run wiz8 report context ADDRESS...` for TU placement.
- Native binary inspection uses
  `wiz8decomp.ghidra.env.open_program(settings, "wiz8")`; the helper serialises
  project access per checkout.
- Correct a wrong Ghidra prototype before writing a body (see item 2); the
  parser and native `Function.updateFunction` paths are in the skill's
  `references/pyghidra.md`.
- `InvalidateDirtyTile004259B0` and the `0x46E6xx` walkers now live in
  `src/wiz8/engine_code/Video2.cpp` and `3d.cpp` after the upstream Video2
  consolidation, not in the old `dirty_tiles.cpp`/`stMeshModel.cpp` locations.
