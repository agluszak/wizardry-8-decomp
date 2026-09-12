# Recovery backlog

Remaining source-model work on top of the current tree. The lists below come
from `wiz8 analyze unresolved` and a marker scan for `FUNCTION` definitions
whose name is still `Function<address>`; refresh them when this file goes
stale. The Clang source model and type gate are described in
[`wiz8-source-model.md`](wiz8-source-model.md).

## Understood functions that still carry address-only names

These definitions already have a behavior comment but no semantic name. Rename
the definition, its declaration, every call site and any handoff record
together. Addresses belong in `// FUNCTION`, `// GLOBAL`, and `// VTABLE`
markers, not in the C++ identifier. Where the original spelling is unknown,
use a behavior-descriptive name rather than an address-qualified placeholder.


### `src/wiz8/character_skills.cpp`


### `src/wiz8/engine_code/3d.cpp`

- `void Function46E640` — Walk one scene subtree and toggle shader bit 3 on every mesh model of every model instance; the instance's flags_3a0 bit zero selects the off state.
- `void Function46E750` — The sibling walker that leaves shader bit 2 clear and writes the low three bits: seven when the argument is zero, otherwise three.

### `src/wiz8/engine_code/Environment.cpp`


### `src/wiz8/engine_code/GDCamera.cpp`

- `void Function421100` — Two thin GDCamera wrappers over GetForwardPoint, placed here because their GameData.cpp ownership was never evidence-backed; they keep their address names until body-leve

### `src/wiz8/engine_code/Prop.cpp`

- `int W8Prop::Function44DEA0` — Build or refresh the pathing representation for a collidable Prop.

### `src/wiz8/engine_code/Video2.cpp`

- `unsigned char Function422800` — Applies the configured window style, asks SurRender for the matching display mode in fullscreen operation, and opens the renderer output window.

### `src/wiz8/engine_code/stParticle.cpp`

- `void stParticle::Function4994D0` — One particle system's complete submission.

### `src/wiz8/local_code/CharGeneration.cpp`

- `void Function557F90` — Refund every spent skill point through the per-skill commit helper.
- `int Function557FD0` — Sums the realm skill costs the edited character still owes: skills the original holds but the edited build lacks, or the original's own set when it has no caster level to
- `void Function556DC0` — A fresh creation: zero the character and the editing state, install the sentinel values, and hand out the level-one pools.
- `void Function556CC0` — One level gained: raise the level and profession counter, reset the editing state, and re-derive every pool the character's new level entitles them to.
- `void Function557C90` — Reset one skill's contribution to the editing state: drop its flag, refund its spent points, and let a realm skill that was just opened up reach its minimum five.
- `void Function557D20` — Refund every point spent on one skill and leave its level at the base value the assignment gave it.
- `void Function557AE0` — Every point the edited character has already committed is spent from the pool in one burst.
- `void Function557730` — Attribute limits and the step ceiling for the current pool, then the attribute-point reconciliations.
- `void Function557800` — Clamp the allocated attribute values to their limits, then take points back round-robin while the character is over budget.
- `void Function557890` — Top the character's attributes up to the profession's minimums, or hand the shortfall to 0x00557200 when the pool cannot cover it.
- `void Function557200` — Draw the level-up attribute debt back down one point at a time, preferring the largest remaining deficit, until the pool or the debt runs out.
- `void Function5579E0` — Apply one attribute adjustment through the same clamps 0x005579E0 uses for the creation flow: never below zero, never above the pool or the limit.
- `void Function557B20` — Skill limits from the pool plus the base levels the attribute pairs give every skill.
- `void Function557EB0` — Clamp the committed skill points to their limits, refund round-robin while over budget, and report whether every skill still has room.
- `void Function558070` — Finalize the spell-point pool the level-up flow presents: settle the realm skills against the spent spell points, then trim any learned spell the character can no longer 
- `int Function558180` — Re-checks every spell against the character's current realm skills and reports how many points still remain to be allocated.
- `void Function557D80` — Reset every skill to the attribute-derived base, hand the profession's skills and bonus skill their share of the step pool, then add the points already committed.
- `int Function558330` — Compute how many spell points the level-up summary can award from the character's six realm pools, saving and restoring the learned-spell flags around the trial assignmen
- `void Function557060` — Recompute the level-up pools after a profession change, refunding every baseline the previous profession's assignment had granted.
- `void Function556EB0` — Apply the race and profession tables once race, profession and faction are all set, rebuilding every derived pool on the way.
- `void Function557BC0` — The level-up attribute editor's pool is fixed by the profession minimums, so the deficit sweep runs before the skill pass.

### `src/wiz8/local_code/Combat.cpp`

- `void Function4E8000` — Record the chosen in-combat action on the slot row, copy its detail block, then aim and validate that choice for a still-active character.

### `src/wiz8/local_code/GameplayCode.cpp`


### `src/wiz8/local_code/GameplayDatabase.cpp`

- `void Function54B250` — Raises three flags, optionally hands the caller's target to 0x005A9E70, then runs a fixed opening sequence.
- `void Function54AF30` — Optionally releases the global status block's two buffers, then clears the whole block - which zeroes those pointers as a side effect, since they live inside it - and all
- `unsigned char Function54A760` — Reads MONSTERS.DBS whole: the count into gXStatus, then - only when the caller wants them - every record into one allocation handed back through the out-parameter.
- `unsigned char Function54A9A0` — The range sibling of LoadMonsterDatabaseRecord, named by its own assertion at GameplayDatabase.cpp line 378.
- `void Function54B100` — The new-game reset. It repeats Function54AF30's status-block cycle inline rather than calling it, clears the item in hand and the carried pool, then grants the starting i
- `void Function54B560` — Clears the settings block and writes its defaults.
- `void Function54B300` — Resets one 0x118-byte slot.

### `src/wiz8/local_code/Health Stamina Mana.cpp`

- `void Function52A3E0` — Recompute the stamina ceiling from the three physical attributes and the level, subtract any outstanding penalty, carry the difference into the current pool, and derive t
- `void Function52A500` — The resistance bonus skill (36) is derived only for the professions whose bodies can learn spells; a few fixed professions keep it at zero.

### `src/wiz8/local_code/Magic Effects.cpp`

- `unsigned char Function554540` — Whether anything holds the screen busy: combat, a modal, the trigger flag, or a current state past the idle slot all answer yes; otherwise the idle check decides.

### `src/wiz8/local_code/MonsterGroup.cpp`

- `void Function5115B0` — On a level's first visit, rebind every group monster's script from the name of the script it already carries, so reloaded monsters resume their level-local script objects.

### `src/wiz8/local_code/NPC Manager.cpp`

- `unsigned char Function50B8F0` — Whether the NPC attached to either of the first two party rows carries the given name style with an unreleased binding, while that row's lead stays under level fifteen.
- `void Function509EA0` — Release the NPC binding held at the given index: clear its monster link and handle, then hand the handle to the owned item-list teardown.
- `W8NpcState* Function50A440` — Hand back the NPC binding selected by a monster-list index, or null when the monster carries no matching enchantment mark or the binding is not released.

### `src/wiz8/local_code/PC Item.cpp`

- `void Function51FB40` — Normalize a stack and split every full overflow stack into the party pool.
- `unsigned char Function51F900` — Merge as much of one stack as fits in another.
- `unsigned int Function520C70` — The binding difficulty of a character's worn items: the worst identify difficulty among binds-on-equip pieces whose binding has not been announced yet, in thirds rounded 
- `void Function520D10` — Reconcile the character and combat state after one equipment record has been emptied.
- `void Function520070` — Empty one live item record.
- `void Function520310` — Empty every item record a character carries.
- `void Function5227D0` — Deliver the one-time character reaction associated with exceptional items.

### `src/wiz8/local_code/Targeting.cpp`

- `void Function53A2C0` — Replace a monster's current combat target with one monster id.
- `unsigned char Function53A300` — Probe whether a monster's current combat target is one of the kinds its chosen hostile spell accepts.
- `unsigned int Function53A3D0` — Map the current screen state to the targeting context used by this path.
- `unsigned char Function53A700` — Whether the pending spell in one party row needs an explicit target.
- `unsigned int Function53A8D0` — Return the spell-like id carried by a chosen action: the fixed attack id, a spell's detail word, or the spell attached to an item use.
- `void Function53A320` — Select the cursor and renderer-side targeting mode for one targeting state, then clear the cached world point so the following refresh recomputes it.
- `void Function53AEB0` — Remove one party slot's highlight bit from every live monster that carries it, notifying the render-side highlight owner for each changed monster.
- `void Function53B160` — Clear the target marker and request the party-display refresh that consumes the change.
- `void Function53B170` — Recompute the target point and hand it to the marker only when it differs from the cached three-float position.
- `unsigned char Function53C270` — A party slot can participate only while occupied, alive, and below the terminal character-state threshold.

### `src/wiz8/local_code/character_events.cpp`

- `void Function52F060` — A character at zero percent hit points may start one of the three recovered incapacitation events.
- `int Function52E750` — Advance the eight character portrait/voice records.

### `src/wiz8/local_code/formation_state.cpp`

- `void Function554580` — This initializer lies in the reviewed attribution gap between Magic Effects.cpp and Formation & Facing.cpp.

### `src/wiz8/local_code/party_encumbrance.cpp`

- `bool Function4EDC60` — Sum the stack weights of everything the character carries.

### `src/wiz8/local_screens/AutomapScreen.cpp`

- `void Function57FD90` — Select which automap buttons are enabled for the update mode, then dirty and redraw both and remember the mode.

### `src/wiz8/local_screens/CharacterScreen.cpp`

- `void Function5B1AF0` — Skill-availability hooks raised by RefreshCharacterSkillAvailability00553CD0 while this screen is current.

### `src/wiz8/local_screens/MainGameScreen.cpp`

- `void Function56C520` — Reset the screen state block: zero its 0x268 bytes, write its reset values, clear the keyword status byte, and reload the keyword lists.
- `void Function56C590` — Forward a monster-script notice to the targeting layer unless the screen is busy or this NPC kind suppresses it.
- `unsigned int Function568950` — The panel flags and the modal-dialog frame hooks.

### `src/wiz8/local_screens/PartySelectionScreen.cpp`

- `void W8State5OptionPanel005EF4AC::Function5C05F0` — Replace the option panel's complete detail composition.
- `void W8State5Controller005EF4CC::Function5C1F40` — Draw the state-5 composition in owner order.
- `void W8State5Controller005EF4CC::Function5C26C0` — Apply the result of a state-5 confirmation dialog.
- `void W8State5Controller005EF4CC::Function5C2C60` — Load the selected imported-party file, report its two failure classes, and refresh every control whose state depends on the resulting six party slots.
- `void W8State5Controller005EF4CC::Function5C2970` — Toggle the selected loose character into the active party, or detach an active member back into an owned loose record.

### `src/wiz8/local_screens/RCSCommon.cpp`

- `void Function5B1C00` — Release the three level-runtime dialogue owners through the shared teardown, then clear the slots.

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
- The cast, void-vector and identity validators in `uv run wiz8 check` stay
  until the Clang lane is proven to catch their injected bad cases. The
  index-based cross-TU check (`validate_cross_tu_declarations`) now covers
  marked functions and external variable declarations, including the
  `extern T[]` vs `T[N]` completion idiom.
- `src/sgp` is retained C. Do not restyle it to recovered Wizardry C++.
- Do not merge `W8ControlsRect` and `W8ScreenRect` without a direct
  cross-API type flow or a common original header/oracle. Matching four-int
  layouts are not enough.
- Do not DRY duplicated retail reset bodies, assertion fall-through after
  `srAssertFail`, permissive PList null handling, shared static formatting
  buffers, or the combat-condition scan that walks past `effect_slots_tail`.
  Those are contracts. Use `// retail:` only where a maintainer would
  otherwise "fix" them.

