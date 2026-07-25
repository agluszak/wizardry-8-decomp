# Initial `Wiz8.exe` gameplay boundaries

The first manually owned executable source unit deliberately starts with three deterministic
primitives rather than a large parser or gameplay state machine. Their canonical bodies are small,
fully typed, and used broadly enough to validate the compiler configuration before it shapes larger
recovery work.

| Address | Function | Canonical size | Evidence |
| --- | --- | ---: | --- |
| `0x00517970` | `RollDice` | 53 | Reads packed `W8Dice`; performs `count` independent `GetRandomNumber(sides) + 1` rolls after the signed base. |
| `0x005179b0` | `IntegerPower` | 24 | Returns one for exponent zero; otherwise multiplies by the base exactly `exponent` times. |
| `0x005179d0` | `ClampInteger` | 25 | Stores the maximum above range, the minimum below range, and otherwise leaves the value unchanged. |

The owned definitions live in `src/wiz8/gameplay_boundaries.c` and retain explicit `FUNCTION`
markers. `WIZ8_GAMEPLAY_BOUNDARIES` is a real VC6 CMake object target built by
`just build WIZ8_GAMEPLAY_BOUNDARIES`; it uses the pinned SP5 `/O2 /G6 /MD` environment alongside the
already exact compression and plug-in targets. The review map is
`config/analysis/reccmp/wiz8-gameplay-boundaries.csv`.

The target adds `/G6`, which is matching-relevant for this translation unit: it changes VC6's
instruction scheduling to the canonical order. With `/O2 /G6 /MD`, all three bodies match exactly
after masking only the four-byte external-call relocation in `RollDice`:

| Function | Result | Relocation-normalized SHA-256 |
| --- | --- | --- |
| `RollDice` | exact, 53/53 bytes | `9e88bdc5744063e0d522ea20c95810352faed566df73a58552566ce96017ab63` |
| `IntegerPower` | exact, 24/24 bytes | `4454e70e52316ed73ef32bf813e14e16145c25fa3f402079afd9de08ecc375e8` |
| `ClampInteger` | exact, 25/25 bytes | `13755985809557446cbd5b7e269ef859eeb4fbaab4313ec284ad3d8232cf7b17` |
