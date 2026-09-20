# Retail SurRender registry probe

Observed 2026-09-21 with the GOG base `Wiz8.exe` (`18a74ff61c65b8a2d4cfa11ffce82ad7fef022a94eaf0c2f217e479e981420d2`) and `sr.dll` (`cec1caf85861c34bc4583ef1c69209e96a6930bdfc9af545c429f7470a8b6165`). The game ran from a copied sandbox under Wine, with `winedbg --gdb` attached. GDB read the live `srCore.registry_` pointer and traversed the `srRegistry::ClassNode` child links at screen entry. No registry memory was changed, and no C++ stream object was passed to retail code. Wine loaded `sr.dll` at `0x00a30000`, so DLL vtable addresses below are process addresses; subtract `0x00a30000` to obtain RVAs.

The startup screen-0 and main-menu screen-1 snapshots contained the same 21 registered classes. Parent indentation below is the registry's logical parent relation. The number in parentheses is the live `instance_count_28`, which includes descendants; it is not an exact count of most-derived objects.

```text
root 0x0 (0)
  srRuntimeClass 0x1 (46)
    srGERD 0x4000 (1)
    srClass 0x100 (45)
      stScript 0x1000d (1)
      srModel 0x2000 (1)
        srMeshModel 0x2010 (1)
      srTextureIFace 0x2100 (22)
        srTexture 0x2110 (22)
          stTexture2D 0x1000f (20)
          srTextureMap 0x2111 (2)
      srColorSurfaceIFace 0x3100 (3)
        srColorSurface 0x3110 (3)
      srMaterialIFace 0x2200 (2)
        srMaterial 0x2210 (2)
      srNode 0x1000 (15)
        srModelInstance 0x1100 (1)
          stModelInstance2D 0x10005 (1)
        stSurface2D 0x1000e (1)
        srCamera 0x1400 (2)
        srScene 0x1010 (10)
      srPalette 0x2900 (1)
```

Every name in this startup tree has a source class declaration, including `srTextureMap` in `include/surrender/srTextureMap.h`. The registry observation independently confirms its ID and logical parent, but does not establish a physical C++ base offset. The `registerClass` flag is a lookup-index control; the live counts must not be used as evidence of abstractness.

## After loading a level

The retail game was restarted with `/LOAD /WINDOW`. The copied sandbox held an existing Quick 1 save made by a local recomp runtime run (save SHA-256 `770c3ed75fc72398c8f08b3834d95c32f6af5baee7d198ede66eeb2fe2d6cffe`). This was input data for the retail loader, not a retail-authored save. The probe reached the Please Wait screen, entered `LoadLevel` at `0x0042a6f0`, and then reached main-game screen 7 at `0x0055f8c0`; the last breakpoint is the post-load observation boundary. The registry at the first two points still had the 21 startup classes. At main-game entry it had 33 registered classes, with 4,174 descendant-inclusive instances under `srRuntimeClass`.

The twelve additional registered classes were:

| Class | ID | Registry parent | Live instances, including descendants |
| --- | --- | --- | ---: |
| `Trigger` | `0x10008` | `srClass` | 60 |
| `srIlluminator` | `0x1200` | `srNode` | 150 |
| `srLight` | `0x1220` | `srIlluminator` | 150 |
| `stGroundShadow` | `0x10010` | `srNode` | 35 |
| `stLevel` | `0x10007` | `srNode` | 2 |
| `stLight` | `0x10006` | `srLight` | 144 |
| `stMaterial` | `0x10002` | `srMaterial` | 117 |
| `stMeshModel` | `0x10003` | `srMeshModel` | 531 |
| `stModelInstance` | `0x10004` | `srModelInstance` | 690 |
| `stParticle` | `0x10009` | `srNode` | 180 |
| `stTextureAnim` | `0x10000` | `srTexture` | 37 |
| `stTextureFile` | `0x10001` | `srTexture` | 1,128 |

All twelve have source class declarations. No startup class disappeared. This run therefore found no missing registered class or provisional class name to replace in the startup-to-level path. It does not establish that other levels or plugins register no further classes.

At `srRegistry::registerInstance` (`sr.dll` RVA `0x0efb0`), the debugger recorded the class node, object pointer, current vtable and immediate registration caller. Several objects registered successively under their ancestor nodes, with the vtable changing during construction. Reading the object again at screen entry distinguished the final table from those transient values:

| Registration chain for one object | Final vtable at screen entry | Reviewed identity |
| --- | --- | --- |
| `srTextureIFace` → `srTexture` → `srTextureMap` | `0x00aa7578` (DLL RVA `0x77578`) | `srTextureMap` |
| `srNode` → `srScene` | `0x00aa7230` (DLL RVA `0x77230`) | `srScene` |
| `srMaterialIFace` → `srMaterial` | `0x00aa5538` (DLL RVA `0x75538`) | `srMaterial` |
| `srNode` → `srModelInstance` → `stModelInstance2D` | `0x005ec858` (EXE) | Wizardry self-support table |
| `srTextureIFace` → `srTexture` → `stTexture2D` | `0x005ec6c0` (EXE) | `stTexture2D` |

The immediate caller of `registerInstance` identifies a registration path, not necessarily the allocation site. For example, the `stTexture2D` registration arrived from `0x0047dc6c`; its final vtable was `0x005ec6c0`. A virtual target at a call site still needs a receiver observation at that site; this sample alone does not enumerate all possible targets.

`srEXT_Inspector.dll` (`6bc2949d6c9bca35ecc37e8f4f98ed36886a075f00d69ae86687d38f682387a1`) is a direct consumer of `getRootClass`, `getChildClass` and `getClassName`. Its code builds a class-tree view, also imports `srRuntimeClass::getName`/`getID`, and exposes scene-graph and diagnostic dump paths. Its only exports are `srGetLibraryVersion` and `srInitPlugin`; the latter creates plugin state and starts a thread. Its dump imports use the original VC6 `std::ostream` ABI. The probe did not invoke them, and neither run loaded Inspector as a plugin, so their exact text output remains unobserved.

The observations establish source-model coverage of the startup and one loaded-level registry, and a safe way to correlate registration chains with final vtables. They do not cover unregistered Wizardry records or prove C++ subobject offsets.
