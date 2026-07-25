# `srEXT_JPEGImporter.dll` target

The JPEG extension is byte-identical in all five materialized variants. Its SHA-256 is
`0bf545937bd86ce2195c08b6dbf6b70e1a1071a9fce449ccb9f06dfa384de490`; the file is 118,876
bytes and has image base `0x10000000`. The PE timestamp is `2000-09-17T13:57:29Z`. This is now the
first matching-decompilation target.

The CodeView record names the original PDB as
`D:\srsdk1x\msvc6\obj\extensions\jpegimporter\release\srEXT_JPEGImporter.pdb`. LINK 6.0, the
MSVC runtime imports, and Rich records with build 8447 all support the VC6-era toolchain finding.
The PDB path additionally identifies the original source domain as the SurRender SDK extension
tree, not Wizardry game code.

## Binary interface

The DLL exports only:

| Address | Ordinal | Name |
| --- | ---: | --- |
| `0x100155D0` | 1 | `srGetLibraryVersion` |
| `0x10014B70` | 2 | `srInitPlugin` |

`srGetLibraryVersion` is recovered in owned source: it returns `0x012A0209`. `srInitPlugin`
allocates `0x48` bytes, constructs an importer/exporter object at offset four, and installs the
complete-object vtable at `0x10016D68`.

There are 98 imported functions: 69 from `SR.DLL`, 24 from `MSVCRT.DLL`, four from
`MSVCP60.DLL`, and `DisableThreadLibraryCalls` from `KERNEL32.DLL`. The dependency on `SR.DLL` is
the main interface-recovery problem for a replacement build.

## Code ownership

The reviewed inventory now accounts for 250 addresses: 241 functions discovered by the current
Ghidra database plus nine code entries recovered directly from vtable references and disassembly.
Source-built Function ID evidence identifies 160 functions as IJG JPEG release 6 and `0x10015915`
as `__DllMainCRTStartup@12`. For the JPEG matches,
all tested compiler rows agree on one source symbol at 156 addresses. Four addresses are deliberately
left ambiguous because the machine bodies are shared by two source routines:

| Address | Candidate symbols |
| --- | --- |
| `0x10001450` | `jpeg_destroy_compress`, `jpeg_destroy_decompress` |
| `0x10007E80` | `jpeg_destroy_compress`, `jpeg_destroy_decompress` |
| `0x10012990` | `jpeg_free_small`, `jpeg_free_large` |
| `0x100129B0` | `jpeg_free_small`, `jpeg_free_large` |

The observed link layout provides a useful first ownership boundary. The tracked inventory assigns
196 addresses to IJG JPEG 6, three to the small Sir-Tech codec adapter, 32 to the SurRender plugin,
and 19 to VC6 runtime/glue (including the separately identified CRT entry point):

| Range | Discovered functions | Interpretation |
| --- | ---: | --- |
| `0x10001000`-`0x100013DF` | 3 | Sir-Tech codec adapter around the IJG API |
| `0x100013E0`-`0x10014B6F` | 196 | IJG JPEG 6, including 36 internal bodies below the FID threshold |
| `0x10014B70`-`0x100155D5` | 23 currently defined, plus missed entries | Sir-Tech SurRender plugin and import/export adapter |
| `0x100155D6` onward | 19 currently defined, plus thunks | compiler, import, exception, and DLL-startup glue |

The range interpretation is a review aid, not a blanket naming rule. Individual reviewed addresses
and their evidence are tracked in `config/analysis/functions/srext-jpegimporter.csv`.

## Recovered plugin behavior

The binary strings establish the class name `srJPEGImporter` and the error paths
`srJPEGImporter::importSurface` and `srJPEGImporter::exportSurface`. The constructor at
`0x10014D40` registers both `jpeg` and `jpg` as importer and exporter types. Export options recognize
`QUALITY`, default it to 100, and clamp the parsed value before calling the JPEG encoder.

The allocation and field accesses prove a two-object layout. The `0x48`-byte plugin wrapper owns a
four-byte plugin vptr followed by a `0x44`-byte `srJPEGImporter`. The importer object begins with
four-byte importer and exporter vptrs, a `0x30`-byte codec-operation record, and a `0x0C`-byte export
options record. These exact offsets are represented in `src/srext_jpegimporter/layout.h`; fields
without semantic evidence remain named by offset.

The original auto-analysis missed important function starts even though the vtables reference them:

- `0x10014E60` validates an input JPEG and returns surface metadata;
- `0x10014F30` imports and converts JPEG pixels into an `srColorSurface`;
- `0x10015200` exports an `srColorSurface` and parses the `QUALITY` option;
- `0x10015450`, `0x10015460`, and `0x10015470` implement the emitted
  `srColorSurface` runtime-class slots;
- `0x100155E0` and `0x100155F0` are exporter-base this-adjusting thunks.

All eight starts now exist in the live Ghidra program. The reviewed function map records their body
sizes, method roles, and signatures. Ghidra also now contains the exact release-6 definitions for
`jpeg_compress_struct` (`0x168`), `jpeg_decompress_struct` (`0x2C8`), `jpeg_error_mgr` (`0x84`),
`jpeg_memory_mgr` (`0x30`), and the other public/internal IJG records. Fifty-nine target functions
have exact prototypes applied directly from `jpeglib.h` or `jpegint.h`; the remaining named IJG
functions are file-local routines for which the headers intentionally provide no declaration.

The `srJPEGPlugin`, `srJPEGImporter`, codec-state, export-option, surface-description, pixel-format,
and four vtable structures are installed as Ghidra data types. Applying the typed virtual signature
at `0x10014E60`, for example, now renders the outputs as `description->width`, `height`, `pitch`, and
`pixel_format` rather than anonymous pointer offsets. The four relevant vtables are:

| Address | Object | Slots |
| --- | --- | --- |
| `0x10016D68` | complete `srJPEGPlugin` | deleting destructor, description |
| `0x10016D70` | `srSurfaceIOManager::SurfaceExporter` subobject | adjusted type name, adjusted destructor, `exportSurface` |
| `0x10016D7C` | `srSurfaceIOManager::SurfaceImporter` subobject | type name, deleting destructor, `getSurfaceDesc`, `importSurface` |
| `0x10016D8C` | locally emitted `srColorSurface` | 52 slots through `minify` |

The importer/exporter signatures are not inferred from stack cleanup alone. `SR.DLL` exports the
base `getSurfaceDesc` decorated symbol, and its manager call sites establish the other two contracts:

```cpp
int getSurfaceDesc(srColorSurfaceIFace::SurfaceDesc&, srBinIStream&,
                   const srSurfaceIOManager::ImportInfo&);
srColorSurfaceIFace* importSurface(srBinIStream&,
                                   const srSurfaceIOManager::ImportInfo&);
void exportSurface(srBinOStream&, srColorSurfaceIFace&,
                   const srSurfaceIOManager::ExportInfo&);
```

## Recovered SDK boundary

The first recovered SurRender headers are under `include/surrender/`. They encode the proven
multiple-inheritance shape, the virtual `srBinStream` base used by directional streams, the exact
`0x28`-byte surface-description record, and the 52 observed `srColorSurface` vtable positions.
Unknown option fields remain explicit; only the export option-string pointer at `+0x08` is currently
semantic. The `0x0C` declarations are the observed prefix needed by this plug-in, not yet a claim
that no later SDK build extends either record.

`config/analysis/surrender/jpeg-sr-imports.csv` records every one of the 69 `SR.DLL` imports with
its exact decorated name, ordinal, demangled signature, calling convention, importing module, and a
representative code or vtable reference. `src/srext_jpegimporter/sr-jpeg-imports.def` is the matching
minimal import-library definition. It was generated from and checked against `sr.dll` SHA-256
`cec1caf85861c34bc4583ef1c69209e96a6930bdfc9af545c429f7470a8b6165`; it does not contain engine
code.

## Compilable skeleton

The owned `plugin.cpp` now builds a `0x48`-byte wrapper with the proven `0x44`-byte importer/exporter
subobject, registers `jpeg` and `jpg` through the original `SR.DLL`, and preserves exports and
ordinals 1 and 2. The operation methods remain deliberate stubs until the IJG state is typed.

The skeleton was compiled and linked successfully using the pinned VC6 SP5 image with `/MD /O2
/GX /GR-`, preferred base `0x10000000`, and incremental linking disabled. The verified link order is
the adapter object, all 46 exact IJG release-6 `/MD /O2` objects, then the SurRender plug-in object.
With dead stripping disabled while the adapter is stubbed, the candidate has `.text` size `0x13DB2`
and starts its three adapter placeholders at `0x10001000`, followed immediately by the IJG object
run. The resulting test DLL is a 32-bit PE with exactly the two original export names and ordinals,
and imports `SR.dll`, `MSVCRT.dll`, and `KERNEL32.dll`. This proves the narrow ABI, source corpus,
object order, and import library are linkable; it is not yet a drop-in replacement because JPEG
decode and encode still return failure/no data.
