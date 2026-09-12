# Recovery backlog

Remaining source-model work on top of the current tree. The lists below come
from `wiz8 analyze unresolved` and a marker scan for `FUNCTION` definitions
whose name is still `Function<address>`; refresh them when this file goes
stale. The Clang source model and type gate are described in
[`wiz8-source-model.md`](wiz8-source-model.md).

## Understood functions that still carry address-only names

These definitions already have a behavior comment but no semantic name. Rename
the definition, its declaration, every call site and any handoff record
together. The `// FUNCTION: WIZ8 0x...` marker is the address identity; do not
keep an address suffix in the C++ name once the behavior is understood.


### `src/wiz8/character_skills.cpp`


### `src/wiz8/engine_code/3d.cpp`

- `void Function46E640` — Walk one scene subtree and toggle shader bit 3 on every mesh model of every model instance; the instance's flags_3a0 bit zero selects the off state.
- `void Function46E750` — The sibling walker that leaves shader bit 2 clear and writes the low three bits: seven when the argument is zero, otherwise three.

### `src/wiz8/engine_code/Environment.cpp`


### `src/wiz8/engine_code/GDCamera.cpp`

- `void Function421100` — Two thin GDCamera wrappers over GetForwardPoint, placed here because their GameData.cpp ownership was never evidence-backed; they keep their address names until body-leve

### `src/wiz8/engine_code/Prop.cpp`


### `src/wiz8/engine_code/Video2.cpp`


### `src/wiz8/engine_code/stParticle.cpp`

- `void stParticle::Function4994D0` — One particle system's complete submission.

### `src/wiz8/local_code/CharGeneration.cpp`

- `void Function557AE0` — Every point the edited character has already committed is spent from the pool in one burst.

### `src/wiz8/local_code/Combat.cpp`

- `void Function4E8000` — Record the chosen in-combat action on the slot row, copy its detail block, then aim and validate that choice for a still-active character.

### `src/wiz8/local_code/GameplayCode.cpp`


### `src/wiz8/local_code/GameplayDatabase.cpp`


### `src/wiz8/local_code/Health Stamina Mana.cpp`

- `void Function52A3E0` — Recompute the stamina ceiling from the three physical attributes and the level, subtract any outstanding penalty, carry the difference into the current pool, and derive t
- `void Function52A500` — The resistance bonus skill (36) is derived only for the professions whose bodies can learn spells; a few fixed professions keep it at zero.

### `src/wiz8/local_code/Magic Effects.cpp`


### `src/wiz8/local_code/MonsterGroup.cpp`

- `void Function5115B0` — On a level's first visit, rebind every group monster's script from the name of the script it already carries, so reloaded monsters resume their level-local script objects.

### `src/wiz8/local_code/NPC Manager.cpp`


### `src/wiz8/local_code/PC Item.cpp`

- `unsigned int Function520C70` — The binding difficulty of a character's worn items: the worst identify difficulty among binds-on-equip pieces whose binding has not been announced yet, in thirds rounded 
- `void Function520D10` — Reconcile the character and combat state after one equipment record has been emptied.
- `void Function5227D0` — Deliver the one-time character reaction associated with exceptional items.

### `src/wiz8/local_code/Targeting.cpp`


### `src/wiz8/local_code/character_events.cpp`

- `int Function52E750` — Advance the eight character portrait/voice records.

### `src/wiz8/local_code/formation_state.cpp`

- `void Function554580` — This initializer lies in the reviewed attribution gap between Magic Effects.cpp and Formation & Facing.cpp.

### `src/wiz8/local_code/party_encumbrance.cpp`


### `src/wiz8/local_screens/AutomapScreen.cpp`


### `src/wiz8/local_screens/CharacterScreen.cpp`

- `void Function5B1AF0` — Skill-availability hooks raised by RefreshCharacterSkillAvailability00553CD0 while this screen is current.

### `src/wiz8/local_screens/MainGameScreen.cpp`

- `void Function56C590` — Forward a monster-script notice to the targeting layer unless the screen is busy or this NPC kind suppresses it.
- `unsigned int Function568950` — The panel flags and the modal-dialog frame hooks.

### `src/wiz8/local_screens/PartySelectionScreen.cpp`

- `void W8State5OptionPanel005EF4AC::Function5C05F0` — Replace the option panel's complete detail composition.

### `src/wiz8/local_screens/RCSCommon.cpp`


### `src/wiz8/local_screens/ReviewCharacterScreen.cpp`

- `void Function5B7230` — The camp screen's remaining six regions, three across and two down.

### `src/wiz8/local_screens/Screens.cpp`

- `bool W8Controls005EE920::Function55EBB0` — Run the first screen command predicate and reset this target through its second virtual slot when command zero succeeds.
- `bool W8Controls005EE920::Function55EBE0` — The parallel path using the second command predicate.

### `src/wiz8/startup_world.cpp`

- `void Function4B5780` — The two caller-provided values override the original 16 MiB and 1 MiB defaults only when positive.

## Single-function link gaps to port next

Each unit below is missing only the listed first-party symbol(s); everything
else in the TU already links. Port the body, place it in the owning header by
the retail address order, then run `uv run wiz8 compare ADDRESS`.
WorldRemoveLight (0x0046E250) from this list was recovered and is no longer a
gap.

| Candidate | Address | Unit(s) waiting | Notes |
| --- | --- | --- | --- |
| `Function420CA0` | 0x00420CA0 | `SoundEvent.cpp` | Ground surface/material lookup, 152 bytes; uses `g_octree_game_data_00652db0`, `surfaces_38` stride 0x4c and `value_54`; placement is a GameData/Video2 gap. |
| `Function497690` | 0x00497690 | `OctBuildPreTree.cpp`, `OctBuildTree.cpp` | Eight-case octree build-report logger; one port completes both units. |
| `Function55EE70` | 0x0055EE70 | `Text_Input.cpp` | Mouse-cursor video-object state setter; reads `g_status_685170` UI state. |
| `Function502010` | 0x00502010 | `Environment.cpp` | Also wants `UpdateEnvironmentLighting00484300` recovered. |
| `W8CharacterPage005EF778::OnPrimary` | — | `CGSStatsPage.cpp` | Virtual override; the class vtable is already recovered. |
| `CalcXPGoal` | — | `GameplayCode.cpp` | Along with `CompatiblePartnerItems(int, int)`. |
| Dialog destructor pairs | — | `ProfRaceInfoDialog.cpp`, `StatInfoDialogs.cpp` | Deleting-destructor glue once the bases are confirmed. |
| Two dozen other units | — | see `wiz8 analyze unresolved` | Most are one first-party symbol short; the ranking names them. |

## Remaining structural cleanup candidates

- `ReadMesh.cpp` still uses the nested lvalue ternary
  `(corner == 0 ? poly.x : ...) = vertex`. Keep it local until SurRender
  evidence proves an `srVector3i` accessor; do not invent an operator.
- `ReadVectorArray`'s `srVector3i` to float-vector cast is a proven 12-byte
  disk-reader body and stays.
- Genuine boundaries to leave alone: button userdata pointer/`INT32`,
  dynamic-library symbol to function pointer, DirectDraw/pixel buffers, SGP
  APIs declared with `UINT8*`, packed-colour byte access.
- Do not merge `W8ControlsRect` and `W8ScreenRect`: identical four-int layout
  is not a source-level bridge.
- Leave `src/sgp` stylistically alone; it is retained C under the vendor lint
  profile.
- Raw offset accesses that still lack a recovered producer or consumer:
  `W8PortraitAnimationState` +0x5c, `W8LevelRuntimeBlock` +0xfc,
  `g_status_685170` fact-0x14c pieces, and `dialogue_owner` +0x2d. Keep the
  marked casts until those fields have both sides.
- `W8MonsterShakeCallbackBase` is ABI emission for the derived destructor's
  vptr swap, not an authored polymorphic interface. Do not collapse it into
  handwritten vptr stores.
- First-party `malloc`/`free` sites in PList, IList, GDFileIO, DialogBase,
  stMessageDialog, 3dapi, OctBuildPreTree, Octree, Text_Input, and OctPath
  stay CRT-backed; none of those recovered bodies has evidence to switch to
  `operator new`.
- The cast, void-vector and identity validators in `uv run wiz8 check` stay
  until the Clang lane is proven to catch their injected bad cases. The
  source-index cross-TU gate (`validate_cross_tu_declarations`) now runs,
  including variable declarations and `extern T[]` vs `T[N]` compatibility.

