# SurRender import visibility

`SR_DLL_IMPORT` is a consumer code-generation contract, not a tag meaning "this symbol belongs to SR.DLL". On the MSVC6 build it changes whether calls go through an imported address, whether a class emits local special members/vtables, and whether a header body can be inlined. Adding or removing it can therefore change reccmp results without changing the apparent C++ API.

## Evidence order

Use consumer evidence to decide visibility:

1. The original consumer import surface (`src/wiz8/imports/sr.def` and the extension import libraries) establishes that a symbol crosses the DLL boundary.
2. Retail caller assembly establishes whether that consumer actually calls an import or expands a header body/client emission locally.
3. SR.DLL exports establish the provider ABI and signatures, but an export by itself never proves `dllimport` in a consumer header.

A virtual reached only through an object's vtable does not need `SR_DLL_IMPORT` merely because the implementation lives in SR.DLL.

## Rules

- Prefer no import attribute until consumer evidence requires one.
- Use member-level `SR_DLL_IMPORT` when only individual members are imported.
- Use class-wide `class SR_DLL_IMPORT` only when the consumer ABI supports class-wide behavior, especially imported constructors/destructors/assignment/vftables or other compiler-generated class machinery.
- Keep a proven header-visible body inline even if SR.DLL also exports an out-of-line copy.
- Do not add `SR_DLL_IMPORT` to fix a linker error, to mark ownership, or because a provider export was recovered.
- Do not remove an existing class-wide import as cleanup without checking the consumer imports and emitted vtable/special-member behavior.
- When evidence is incomplete, leave the declaration unchanged and record the uncertainty rather than toggling the attribute speculatively.

## Audited cases

The `srCore` accessors `getRegistry`, `getMaterial`, `getStatisticsManager`, and `getTimer` are header-visible. Retail users load their fields directly; making them imports inserts calls that are absent from the original code.

`srTextureFile`, `srTriangulator`, `srExponentTable`, and `srFStreamOpener` are provider-side declarations with no corresponding known Wizardry/JPEG/ZIP consumer imports. They therefore do not carry class-wide `SR_DLL_IMPORT` even though SR.DLL exports their provider symbols.

Class-wide import is still correct where the consumer evidence reaches that far. `srTimer`, for example, is constructed by Wizardry and its imported virtual surface participates in the client-local vtable emission. `srMaterialIFace` imports compiler-generated destructor/assignment symbols in addition to its named method, so reducing it to a single member annotation would lose class ABI information.

`srBinIStream` and `srDebugVP` are being converted away from stale blanket annotations in their dedicated recovery work. The repository gate accepts either state for those two so the independent changes can merge in either order.

## Gate

`tests/repository/test_surrender_import_visibility.py` freezes the audited class-wide import set and the proven inline `srCore` accessors. A PR that adds or removes a class-wide annotation must update that audit intentionally and explain the consumer/codegen evidence. Provider-only classes are explicitly rejected.

This gate protects source-level ABI decisions; it does not claim that `SR_DLL_IMPORT` was the literal macro name used by the original SDK.
