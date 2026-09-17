# Cosmic Forge semantic evidence

Cosmic Forge is a third-party semantic and patching oracle, not original Sir-Tech source-name evidence. Findings in this document therefore use the repository's `cosmic-forge` / `external-semantic` authority unless an independent retail consumer establishes the same meaning.

This pass analyzed the supplied Cosmic Forge 4.38 artifacts statically. They are distinct from the separately reviewed `cfagent1.28.dll` and must not share address or loader assumptions.

| Artifact | SHA-256 |
| --- | --- |
| uploaded `cf.zip` | `9f9b0eb46b0d8564ff690d041868cdc0b83173b02e3c94a8e3c548fa9288addd` |
| `CFAgent.dll` | `47aea7af79348dfa4862ee95ebced0a1636b78dcc8f5dbd2ea3fae57fb445b75` |
| `CosmicForgeU.exe` | `404aa27cac6bd9270cfaa8fca5958971079303f652d9bee5b6d423ce5c807c2a` |

## Accepted semantic corrections

### Monster database fields

The Monster Editor provides paired control-to-record mappings for a larger set of fields than the first pass used. The reviewed map is kept in `evidence/reviewed/cosmic-forge-438/monster-fields.csv`, including control IDs and editor instruction sites. Retail consumers independently corroborate several of the highest-value names.

The source now uses the following external-semantic names where the mapping is strong enough to improve the model:

- `+0x0c0` `can_open_doors_0c0`: retail propagates it to the monster movement flag used by door traversal;
- `+0x0ce` `stamina_regeneration_0ce`: the editor labels the signed byte `ST Regen`;
- `+0x0e3` `special_attack_kind_0e3`: the editor labels it `Special attack`, while retail uses it to index the GroupAttacks effect, condition, realm, summon and display-name tables;
- `+0x0e4` `initiative_0e4`;
- `+0x15c` `special_attack_cooldown_15c` and `+0x15d` `evasion_ac_15d`;
- `+0x15e` `constitution_15e`: retail uses the selected value as the body-specific hit-location label row;
- `+0x1b9` `prefer_ranged_actions_1b9`: retail suppresses close/backoff behavior and the Monster Info debug text calls the resulting strategy `Ranged`;
- `+0x1be` `instant_death_immune_1be`: retail refuses the DEAD condition when it is set;
- `+0x1bf` `combat_behavior_1bf` and `+0x1c0` `combat_morale_1c0`: these are distinct adjacent selectors; the prior model had collapsed the semantic onto `+0x1c0`;
- `+0x247` `attack_multiple_targets_247`, `+0x248` `camouflage_248`, `+0x25b` `hostility_radius_25b`, `+0x263` `material_263`, `+0x268` `significant_kill_268`, and `+0x26a` `unborn_26a`.

`special_attack_kind_0e3` also improves the surrounding recovered API: the tables at `0x0061EEFC`, `0x0061EFFC`, `0x0061F0FC`, and the display-name table at `0x0061EC14` are special-attack metadata rather than generic AI-kind tables.

Two apparent editor mappings are deliberately rejected. Cosmic Forge presents `+0x0e2` as a flee-chance control, while retail directly consumes `+0x0e1` as flee chance and `+0x0e2` as advance chance. Likewise, a Cosmic Forge label for a `flags_0d0` bit conflicts with established retail behavior. The canonical executable wins in both cases.

### Monster combat move range

Cosmic Forge's Wizardry 8 Monster Editor dialog 212 connects control 1139 (`Combat Move Range`) to monster-record offset `+0x1ba` in both directions:

- `0x00714068` reads `DWORD PTR [eax+0x1ba]` as a float for the control;
- `0x0071bcbb` writes the edited float back to `[ecx+0x1ba]`.

Retail independently consumes the same field at `0x004e5990`, multiplying it by the float at `0x005ed4f0`, and the result is used by monster movement, combat, noise and AI range logic. The source therefore uses the descriptive names `combat_move_range_1ba` and `GetMonsterCombatMoveRange`. Cosmic Forge establishes the designer-facing meaning; it does not establish Sir-Tech's original identifier or the physical units.

### Trigger item-group seed

`Trigger + 0x354` is initialized from `GetTickCount() + Random(30000)`, serialized with the trigger, and supplied to `srand` by `Trigger::GenerateItemGroup` at `0x00445500`. Cosmic Forge's True Random Loot patch removes that reseed call rather than replacing the item-generation algorithm.

The timer-oriented `next_activation_time_354` name is therefore misleading. The source uses `item_group_seed_354`. This is a semantic rename only: retail's `srand` call and deterministic loot behavior remain unchanged.

### Physical-surface fields and flags

Cosmic Forge's physical-face editor operates on `0x4c`-byte records, matching the established `W8GDSurface` size. Its paired read/write controls identify:

- `+0x3c`: footstep surface/environment;
- `+0x3d`: footstep material/substance;
- `+0x48`: slope.

Retail independently feeds `+0x3c/+0x3d` through `GetGroundSurfaceInfo` into the footstep sound path, and derives `+0x48` from the face normal's Y component. The source therefore uses `footstep_surface_3c`, `footstep_material_3d`, and `slope_48`. The two caller-provided `GDProp` bytes are named accordingly.

The editor also labels flag `0x04` as the walkable/floor property and `0x40` as participation in pathfinding generation. Retail independently treats `0x04` as the floor/walkability gate in surface classification and collision code and `0x40` as path-generation state in `OctPath`. The source therefore declares `W8_GD_SURFACE_WALKABLE` and `W8_GD_SURFACE_PATHFINDING`; the remaining flag bits stay unnamed.

The `+0x40` data field is deliberately *not* renamed. Cosmic Forge exposes it as party-camera height for authored physical floors, but retail-generated trigger surfaces also use the same slot as mutable state. The source must retain that ambiguity until the file/runtime ownership boundary is resolved.

### Profession experience groups are compiler lowering

The supplied `classesexpgroup.cfdat` contains exactly fifteen bytes:

```text
00 01 01 01 01 02 01 00 00 00 03 03 02 03 03
```

Indexed by profession, those four group IDs exactly reproduce the four case partitions in retail `CalcXPGoal` at `0x004ef090`: group 0 selects weight 1000, group 1 selects 1400, group 2 selects 1600, and group 3 selects 1200. CFAgent writes the bytes at English address `0x004ef1e0`, inside the function's executable range.

That address is therefore not evidence for an authored 15-byte global. Cosmic Forge is patching compiler-generated switch/lookup data inside `CalcXPGoal`. The recovered source keeps the authored-looking switch; `cfdat-overrides.csv` records the override as compiler lowering rather than rejecting the destination as an impossible data address.

### Condition 16/17 ordering

Cosmic Forge presents conditions 16 and 17 in the reverse order from the retail model. Retail itself is decisive here: both the condition icon catalog and the spell/condition-name table contain the ordered run `Webbed, Asleep, Paralyzed, Unconscious, Dead`. The runtime monster visual path also maps condition slot N directly to icon N-1 for slots 1 through 18.

The canonical ordering therefore remains 15=`Asleep`, 16=`Paralyzed`, 17=`Unconscious`, 18=`Dead`. Cosmic Forge's reversed presentation is an editor-side mapping quirk and must not be projected into the recovered runtime enum.

### Monster experience and its override

Cosmic Forge's Monster Editor reads monster-record `+0x181` into control 1101, whose resource label is `Experience`. Retail `GetMonsterExperience` at `0x004e6780` returns `+0x26b` when that value is nonzero and otherwise returns the `+0x181` base. Its Monster Info consumer displays that effective value only after the species status reaches the post-combat known state; the helper's other retail consumer uses the value as an HP-weighted combat-difficulty proxy when selecting combat music and difficulty effects.

The source therefore names `+0x181` `experience_181`, `+0x26b` `experience_override_26b`, and the accessor `GetMonsterExperience`. `Experience` is directly supported for the base field by Cosmic Forge and for the effective helper by the retail display path; the override spelling is a descriptive relationship inferred from the helper's precedence rule, not an original Sir-Tech identifier.

## Findings deliberately not promoted

- Physical-face `+0x3e`, `+0x40`, and `+0x44`, plus all still-unidentified surface flag bits, remain unresolved.
- Conflicting Cosmic Forge labels for monster `+0x0e2` and `flags_0d0` are not promoted because canonical retail consumers disagree.
- A Cosmic Forge editor label alone does not establish an original Sir-Tech identifier; names in the reviewed field map retain `external-semantic` authority unless stronger evidence exists.

These boundaries are intentional: Cosmic Forge is most useful when it narrows a retail investigation, not when its editor labels are imported wholesale.
