# `W8LevelRuntimeBlock` recovery inventory

This is a non-gating inventory of the still-positional members in
`include/wiz8/layouts/main_game_screen.h`. Names are retained until more than
initialization or a single opaque consumer establishes a role. Offsets and
access sites below are for the current recovered source model.

## Multiple recovered access sites

| Offset | Member | Recovered access sites | Present evidence |
|---:|---|---|---|
| `0x0fc` | `value_0fc` | `Screens.cpp`, `MainGameScreen.cpp` | gates two portrait-refresh paths, but the opposite branch shapes do not yet establish a stable positive name |
| `0x108` | `flag_108` | `Screens.cpp`, `MainGameScreen.cpp` | cleared during level-block initialization and set when a portrait refresh is scheduled |
| `0x271` | `flag_271` | `Screens.cpp`, `mipe.cpp`, `MGSSpellCasting.cpp`, `MainGameScreen.cpp` | toggles across MIPE, spell-casting, and main-game panel transitions; the shared abstraction remains unresolved |
| `0x272` | `flag_272` | `Screens.cpp`, `mipe.cpp` | inverse of `flag_271` in the recovered MIPE transitions, with no independent consumer yet |
| `0x2e8` | `value_2e8` | `Screens.cpp`, `MGSTextBox.cpp` | initialized from `g_font_683660` and returned by the text-box accessor; the exact font role is unresolved |
| `0x314` | `flag_314` | `Screens.cpp`, `MainGameScreen.cpp` | controls a combat-portrait/target presentation path; nearby anonymous callees still own the exact mode semantics |
| `0x326` | `flag_326` | `Screens.cpp`, `MGSRadarMap.cpp` | initialized clear and gates a radar-map update path |
| `0x327` | `flag_327` | `Screens.cpp`, `MainGameScreen.cpp`, `ReviewCharacterScreen.cpp`, `Combat.cpp`, `Health Stamina Mana.cpp` | set by the endgame transition and suppresses ordinary main-game/combat presentation updates |
| `0x328` | `flag_328` | `Screens.cpp`, `MainGameScreen.cpp` | selects one of two adjacent per-frame dispatch paths; only one recovered behavioral consumer exists |

## Initialization-only or single recovered consumer

These members have no second independent behavioral use, so assigning a
semantic name would currently overstate the evidence:

- `0x0f0 flag_0f0`, `0x194 value_194`, `0x198 value_198`, `0x210 flag_210`,
  `0x214 clock_214`, `0x218 flag_218`, `0x23c value_23c`, `0x270 flag_270`,
  `0x278 value_278`, `0x284 value_284`, `0x288 value_288`, `0x28c value_28c`,
  `0x2ac value_2ac`, `0x2b0 value_2b0`, `0x2b4 value_2b4`, `0x2f4 value_2f4`,
  `0x31c flag_31c`, and `0x324 flag_324` are only initialized in `Screens.cpp`.
- `0x2e4 unknown_2e4[0]` is the only byte within that range touched by
  recovered source, in `Screens.cpp`.
- `0x24d flag_24d` has one recovered write in `MainGameScreen.cpp`.

## No direct recovered source access

The following storage is covered by the layout but has no direct field access
in checked-in recovered bodies: `unknown_000[0xf0]`, `unknown_0f1[3]`,
`unknown_0f8[4]`, `unknown_111[3]`, `unknown_158`, `unknown_15a[0x12]`,
`value_19c`, `unknown_1f9[3]`, `unknown_211[3]`,
`unknown_219[7]`, `unknown_22c[0xc]`, `unknown_24c`, `unknown_24e[2]`,
`unknown_262[2]`, `unknown_273`, `unknown_290[0x10]`, `unknown_2b8[8]`,
`unknown_2c1[3]`, `unknown_2c9`, the remaining bytes of `unknown_2e4[4]`,
`unknown_2f9[3]`, `unknown_301[3]`,
`unknown_315[3]`, `unknown_31d[3]`, and `unknown_329[3]`.

The fixed slots at `0x170..0x188` remain `party_slots_170[7]`: initialization
proves they are not interchangeable with the formation-highlight slot at
`0x18c`, but there are not yet recovered behavioral consumers that distinguish
their individual roles.
