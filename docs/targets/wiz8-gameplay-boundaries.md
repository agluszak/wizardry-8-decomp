# Initial `Wiz8.exe` gameplay boundaries

The first manually owned executable source unit deliberately starts with deterministic
primitives rather than a large parser or gameplay state machine. Their canonical bodies are small,
fully typed, and used broadly enough to validate the compiler configuration before it shapes larger
recovery work.

| Address | Function | Canonical size | Evidence |
| --- | --- | ---: | --- |
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

The owned definitions live in `src/wiz8/gameplay_boundaries.c` and `src/wiz8/spell_backfire.cpp`
and retain explicit `FUNCTION`
markers. `WIZ8_GAMEPLAY_BOUNDARIES` is a real VC6 CMake object target built by
`just build WIZ8_GAMEPLAY_BOUNDARIES`; it uses the pinned SP5 `/O2 /G6 /MD` environment alongside the
already exact compression and plug-in targets. The review map is
`config/analysis/reccmp/wiz8-gameplay-boundaries.csv`.

The target adds `/G6`, which is matching-relevant for this translation unit: it changes VC6's
instruction scheduling to the canonical order. With `/O2 /G6 /MD`, ten bodies match exactly after
masking COFF relocations where needed:

| Function | Result | Relocation-normalized SHA-256 |
| --- | --- | --- |
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

`CanSpellBackfire` is retained because the typed semantics and all exception sets are grounded in
the canonical body, but it is intentionally classified as `structurally-strong`, not exact. The
current VC6 output emits a separate true-return block for one nested switch. That unresolved
code-layout difference is recorded instead of being hidden behind a misleading hash.
