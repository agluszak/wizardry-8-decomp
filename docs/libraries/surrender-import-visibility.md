# SurRender import visibility

`SR_DLL_IMPORT` is a consumer code-generation contract, not a tag meaning "this symbol belongs to SR.DLL". On the MSVC6 build it changes whether calls go through an imported address, whether a class emits local special members/vtables, and whether a header body can be inlined. Adding or removing it can therefore change reccmp results without changing the apparent C++ API.

## Evidence order

Use consumer evidence to decide visibility:

1. The original consumer import surface (`src/wiz8/imports/sr.def`, `src/srext_jpegimporter/sr-jpeg-imports.def`, and `src/srext_unzip/sr-unzip-imports.def`) establishes that a symbol crosses the DLL boundary.
2. Retail caller assembly establishes whether that consumer actually calls an import or expands a header body/client emission locally.
3. SR.DLL exports establish the provider ABI and signatures, but an export by itself never proves `dllimport` in a consumer header.

Check the decorated symbol's **owner**, not merely whether a type name occurs in the mangling. For example, `srPixelConvert` appears in `srColorSurface` constructor signatures, but that does not make `srPixelConvert` methods imports.

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

`srTextureFile`, `srTriangulator`, `srExponentTable`, and `srFStreamOpener` are provider-side declarations with no corresponding known Wizardry/JPEG/ZIP consumer imports. They therefore do not carry class-wide `SR_DLL_IMPORT` even though SR.DLL exports provider symbols for some of them.

The same rule applies at member granularity. `srBounder`, `srDebugDD`, `srThread`, `srMutex`, `srMemoryPool`, `srEnvironmentMapper`, `srVideoManager`, `srBinIAsyncStream`, and the `srBinFStream`/`srBinIFStream`/`srBinIOFStream`/`srBinOFStream` family have no known consumer-owned imports, so their headers contain no `SR_DLL_IMPORT`. Their provider exports, vtables, vbtables, recovered bodies, or appearance as parameter/return types do not change that.

Mixed headers are audited declaration by declaration. `srPixelConvert` has one known imported member: `mapPixelFormat(e_surfaceType, PixelFormat&)`, used by both Wiz8 and the JPEG extension. Its reverse overload, `selectFuncs`, and the `PixelFormat` helper methods are not known consumer imports and therefore are not annotated. In `srImporter.h`, `srSurfaceIOManager::exportSurface` is imported by Wiz8, while `SurfaceImporter::getSurfaceDesc` and the `srHierarchyIOManager` / `srModelIOManager` import/export helpers are provider-side declarations.

Class-wide import is still correct where the consumer evidence reaches that far. `srTimer`, for example, is constructed by Wizardry and its imported virtual surface participates in the client-local vtable emission. `srMaterialIFace` imports compiler-generated destructor/assignment symbols in addition to its named method, so reducing it to a single member annotation would lose class ABI information.

`srBinIStream` and `srDebugVP` are being converted away from stale blanket annotations in their dedicated recovery work. The repository gate accepts either state for those two so the independent changes can merge in either order.

## Gate

`tests/repository/test_surrender_import_visibility.py` freezes the audited class-wide import set, rejects consumer-import annotations in audited provider-only headers, locks the exact import count/spelling in mixed audited headers, keeps `srFStreamOpener` provider-only, and protects the proven inline `srCore` accessors. A PR that changes these visibility decisions must update the audit intentionally and explain the consumer/codegen evidence.

This gate protects source-level ABI decisions; it does not claim that `SR_DLL_IMPORT` was the literal macro name used by the original SDK.
