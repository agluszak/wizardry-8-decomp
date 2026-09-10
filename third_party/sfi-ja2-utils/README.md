# JA2 Utils source ancestry

`utils/Text_Input.c` and `utils/Text_Input.h` are released JA2 Utils sources,
not members of the SGP project or product build inputs. The Wizardry derivative
remains in `src/wiz8/local_code/Text_Input.cpp`.

- Upstream repository: `https://github.com/ja2-stracciatella/ja2-stracciatella.git`
- Upstream revision: `5ac0a9d56d27e8a7e2c4a7b48ed8932ae7f64033`
- Upstream tree: `52766c4237e63d7a3d619796784947ed1681f24e`
- Upstream commit: `Initial Import`, dated 2004-09-06
- Source location: `ja2/Build/Utils/Text_Input.{c,h}`
- License: [`SFI-SCLA`](../../src/sgp/SFI%20Source%20Code%20license%20agreement.txt)
- License blob: `b66aabc6f7affb4fca0b6cc2d6288f3225ecd0b1`

The license permits modification and redistribution for non-commercial purposes subject to its
conditions. It is included verbatim. Do not remove copyright or other notices. When modifying a
vendored file, add a prominent notice describing the change and its date, as required by paragraph
4 of the license.

The two ancestor files are copied from that revision without modification.
The reconstructed SGP project itself is in `src/sgp`; there is no runtime SGP
dependency under `third_party`.

## Demonstrated Wizardry delta

- `TEXTINPUTNODE` is `0x6c` bytes rather than `0x68`; allocation and field accesses
  establish the added bytes at `0x60..0x63` before the two list links. Its embedded
  `MOUSE_REGION` remains SGP's type.
- `TextInputColors` is `0x18` bytes rather than `0x16`, including the inactive-field
  colour at `0x16` used by the retail scheme writer and renderer.
- `InitTextInputModeWithScheme` at `0x005d3520` includes session setup and supports
  three Wizardry schemes; the separate writer is at `0x005d35e0`.
- `AddTextInputField` at `0x005d39b0` accepts an enabled state, returns the assigned
  field ID and initializes the extra mouse-routing bytes.
- Retained product bodies use Wizardry wide-input filters, surfaces, cursor and
  modal state, and callbacks. Source ancestry does not move their ownership to SGP.

Do not fabricate missing JA2 headers to make this ancestor a second implementation.
