# Wizardry product delta

The released SFI SGP `LibraryDataBase.c` is retained as the canonical source
unit. Wizardry 8 adds one reviewed behavior to `GetLibraryIDFromFileName`: a
patch library with an equal or shorter base path becomes the match only when
that requested file is present in the patch. This is the `fPatchLibrary` branch
recovered from retail function `0x004131B0`.

The corresponding Wizardry layouts add `fMapFile` to `LibraryInitHeader`, use
the alignment byte after `fLibraryOpen` as `fPatchLibrary`, and append the two
file-mapping fields to `LibraryHeaderStruct`. Their retail sizes are `0x103`
and `0x28`, respectively. The four accepted exact bodies in this unit remain
unchanged and are linked from the vendored source rather than restated.

## Text input fork

`utils/Text_Input.c` and `utils/Text_Input.h` are the pristine released source
oracle. They are not product build inputs: the released unit includes several
unpublished game headers and Wizardry does not contain that unit unchanged.
The product fork remains in `src/wiz8/local_code/Text_Input.cpp`, with the
released source supplying authored names, ordinary control flow, and behavior
where retail does not contradict it.

The demonstrated Wizardry boundary is:

- `TEXTINPUTNODE` is `0x6c` bytes rather than the released `0x68`. Retail
  allocation and field accesses add the bytes at `0x60` through `0x63` before
  the two list links; its embedded `MOUSE_REGION` remains the released SGP type.
- `TextInputColors` is `0x18` bytes rather than the released `0x16`. The retail
  scheme writer and inactive-field renderer use the appended
  `usWizardryInactiveColor` at `0x16`.
- Retail `InitTextInputModeWithScheme` at `0x005D3520` incorporates session
  setup and accepts three Wizardry presentation schemes. The separate retail
  scheme writer is at `0x005D35E0`; the released unit supports only its JA2
  default scheme.
- Retail `AddTextInputField` at `0x005D39B0` accepts an initial enabled state,
  returns the assigned field ID, and initializes the added node bytes used by
  Wizardry mouse routing. These are product API and object-layout changes, not
  adapters around the released function.
- The remaining retained bodies use Wizardry's wide-input filters, rendering
  surfaces, cursor state, modal-input checks, and UI callbacks. A body stays in
  the product fork unless focused retail comparison proves that the released
  implementation is an unchanged source identity.

This separation is intentional. Compiling the released file by fabricating its
missing product headers, or hiding the fork behind replacement interfaces,
would overstate the source oracle and lose the evidenced Wizardry ABI.
