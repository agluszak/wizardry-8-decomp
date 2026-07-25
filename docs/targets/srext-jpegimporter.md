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

The current auto-analysis missed important function starts even though the vtables reference them:

- `0x10014E60` validates an input JPEG and returns surface metadata;
- `0x10014F30` imports and converts JPEG pixels into an `srColorSurface`;
- `0x10015200` exports an `srColorSurface` and parses the `QUALITY` option;
- `0x10015450`, `0x10015460`, and `0x10015470` implement SurRender type/registration helpers;
- `0x100155E0` and `0x100155F0` are exporter-base this-adjusting thunks.

These functions must be created in the reviewed analysis model before the 241-function Ghidra count
can be treated as complete. The next recovery step is to type the vtables at `0x10016D68`,
`0x10016D70`, and `0x10016D7C`, then recover the `srJPEGImporter` object layout from the three large
methods rather than assigning names from proximity alone.
