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
the main interface-recovery problem for a replacement build. The rebuilt image now has this exact
module and symbol set. Marking the four recovered importer/exporter interfaces as
`__declspec(novtable)` removes three spurious abstract-base vtables and the otherwise spurious
`MSVCRT!_purecall` import.

## Code ownership

The reviewed inventory now accounts for 388 addresses. The initial 250-address Ghidra census
missed 133 private IJG functions plus the file-local Sir-Tech error callback at `0x100010D0`.
All 134 starts have now been created and named in the live program.
Source-built Function ID evidence identifies 160 functions as IJG JPEG release 6 and `0x10015915`
as `__DllMainCRTStartup@12`. For the JPEG matches, all tested compiler rows agree on one source
symbol at 156 addresses. The linked PDB, object map, body sizes, and in-object sequence resolve the
remaining 173 IJG bodies. This includes four identical-body pairs which FID alone cannot separate:

| Address | Candidate symbols |
| --- | --- |
| `0x10001450` | `jpeg_destroy_compress` in `jcapimin.c` |
| `0x10007E80` | `jpeg_destroy_decompress` in `jdapimin.c` |
| `0x10012990` | `jpeg_free_small` in `jmemnobs.c` |
| `0x100129B0` | `jpeg_free_large` in `jmemnobs.c` |

The observed link layout provides a useful first ownership boundary. The tracked inventory assigns
329 addresses to IJG JPEG 6, four to the small Sir-Tech codec adapter, 32 to the SurRender plugin,
and 23 to VC6 runtime/glue (including the separately identified CRT entry point):

| Range | Discovered functions | Interpretation |
| --- | ---: | --- |
| `0x10001000`-`0x100013DF` | 4 | Sir-Tech codec adapter around the IJG API |
| `0x100013E0`-`0x10014B6F` | 329 | Complete IJG JPEG 6 linked function set |
| `0x10014B70`-`0x100155D5` | 23 currently defined, plus missed entries | Sir-Tech SurRender plugin and import/export adapter |
| `0x100155D6` onward | 19 currently defined, plus thunks | compiler, import, exception, and DLL-startup glue |

The range interpretation is a review aid, not a blanket naming rule. Individual reviewed addresses
and their evidence are tracked in `config/analysis/functions/srext-jpegimporter.csv`.

## Recovered plugin behavior

The binary strings establish the class name `srJPEGImporter` and the error paths
`srJPEGImporter::importSurface` and `srJPEGImporter::exportSurface`. The constructor at
`0x10014D40` null-checks the surface manager, registers `jpg` and then `jpeg` as importer and
exporter types, and initializes the persistent codec options to limit 200 and quality 75. Export options recognize
`QUALITY`, default it to 100, clamp the parsed floating-point value to `[0.0, 1.0]`, multiply it by
100, and truncate the result to the original byte field before calling the JPEG encoder.

The stdio bridge is also recovered rather than treated as ordinary CRT code. Its `fread` and
`fwrite` replacements catch every exception from the active SurRender stream and rethrow the exact
input- or output-corruption string embedded in the original. The user `DllMain` at `0x100155B0`
calls `DisableThreadLibraryCalls` only for `DLL_PROCESS_ATTACH` and otherwise returns success.

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

All eight starts now exist in the live Ghidra program. Four previously missed MSVCP60 static-init
starts at `0x10015530`, `0x10015560`, `0x10015570`, and `0x100155A0` were also created. The current
map applies 369 accepted identities with no failures. The reviewed function map records their body
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
multiple-inheritance shape, the virtual `srBinStream` base used by directional streams, the
directional output stream's protected virtual `vput(char)` slot and resulting vptr/vbptr layout, the exact
`0x28`-byte surface-description record, and the 52 observed `srColorSurface` vtable positions.
The plugin constructs a zero-data `srJPEGColorSurface` wrapper around the imported concrete
`srColorSurface`. This typed inheritance accounts for the original call to the `SR.DLL` constructor
followed by installation of the extension-local vtable; no raw vptr assignment is needed in owned
source.
Unknown option fields remain explicit; only the export option-string pointer at `+0x08` is currently
semantic. The `0x0C` declarations are the observed prefix needed by this plug-in, not yet a claim
that no later SDK build extends either record.

`config/analysis/surrender/jpeg-sr-imports.csv` records every one of the 69 `SR.DLL` imports with
its exact decorated name, ordinal, demangled signature, calling convention, importing module, and a
representative code or vtable reference. `src/srext_jpegimporter/sr-jpeg-imports.def` is the matching
minimal import-library definition. It was generated from and checked against `sr.dll` SHA-256
`cec1caf85861c34bc4583ef1c69209e96a6930bdfc9af545c429f7470a8b6165`; it does not contain engine
code.

## Matching build

The owned `plugin.cpp` now builds a `0x48`-byte wrapper with the proven `0x44`-byte importer/exporter
subobject, registers `jpeg` and `jpg` through the original `SR.DLL`, and preserves exports and
ordinals 1 and 2. `getSurfaceDesc`, pixel import, pixel export, `QUALITY` parsing, surface lifetime,
and the four locally owned surface runtime-class methods now have typed implementations. The IJG
implementation itself remains the pristine upstream release-6 source.

The root `CMakeLists.txt` owns the product build. It compiles the pinned pristine IJG release-6
source with the two SurRender stdio adaptations, then links the recovered adapter and plug-in using
the VC6 SP5 image, `/MD /O2 /GR-`, base `0x10000000`, `/OPT:NOREF`, and `/OPT:NOICF`. The stream
bridge retains `/GX` for its two explicit handlers; the recovered plug-in unit uses `/GX-` to
reproduce the observed lifetime functions without synthetic unwind prologues. The product
uses the explicit original object order: the decisive tail is `jmemmgr.c`, `jmemnobs.c`,
`jquant1.c`, `jquant2.c`, and `jutils.c`. FID seed discovery remains glob-based because link order is
irrelevant there. The image contains pinned
32-bit Windows CMake 3.26.6 and drives the original NMake/VC6 tools under Wine. The build emits a VC6
PDB, linker map, the exact two exports, and a local `reccmp-build.yml`.

`reccmp` is pinned to commit `574601de72a0ddabdcf2d386ddb6f9d727af4ce1`. The accepted IJG
identities used by comparison are in `config/analysis/reccmp/srext-jpegimporter.csv`. Repeated IJG
private names are qualified per translation unit in PDB/COFF only; this removes ambiguous comparison
pairing without changing machine code. A complete comparison currently proves exact machine-code
matches for:

| Original address | Recovered function | Match |
| --- | --- | ---: |
| `0x10001000` | header adapter | 100% |
| `0x100010D0` | IJG error callback | 100% |
| `0x100010F0` | encoder adapter | 100% |
| `0x10001290` | decoder adapter | 100% |
| `0x10014B70` | plug-in factory | 100% |
| `0x10014BA0`-`0x10014BD0` | plug-in description and lifetime | 100% |
| `0x10014BE0`, `0x10014CC0` | SurRender stream bridges | effectively 100% |
| `0x10014D40`-`0x10014E10` | importer lifetime and option initialization | 100% |
| `0x10014E30` | header operation | effectively 100% |
| `0x100151D0` | surface deleting destructor | 100% |
| `0x10015400` | export-option initialization | 100% |
| `0x10015450` | surface class ID | 100% |
| `0x100155B0` | user `DllMain` | 100% |
| `0x100155D0` | library version export | 100% |

The report now covers 366 code and data identities, including all 329 linked IJG functions, every
reviewed first-party/lifetime function, the four concrete vtables, and the two active-stream globals.
All 366 are implemented, aggregate accuracy is 98.92%, and 333 functions are address-aligned. The
four vtables compare at 100%; the globals match byte-for-byte; the PE exports, import set, and version
resource also match the original. The original Sir-Tech configuration omits both `fflush` and `ferror` from the IJG
stdio destination; reproducing that fact removes the former `0x20` tail drift and puts every IJG
translation unit at its original address. The larger denominator includes every recovered public operation:
`getSurfaceDesc` is 53.57%, `importSurface` is 29.41%, and `exportSurface` is 74.14%. The low import
score reflects unresolved
local/control-flow selection rather than missing behavior; its allocation branches, vtable install,
decode failure cleanup, and 1/3/4-component row conversions are all represented.
This proves that upstream IJG remains upstream source: only the Sir-Tech adapter and SurRender ABI
are manually recovered.

## Runtime replacement proof

The rebuilt DLL was installed only in a reflinked copy of the materialized GOG tree and exercised
with Wine 9.0 in a dedicated 32-bit prefix. The original game loaded, unloaded, and loaded the
replacement extension again during normal startup, then remained running for the 20-second smoke
window. The only diagnostics were the pre-existing Wine display and wined3d warnings.

A small VC6 harness loaded the original `sr.dll`, called `srInit`, constructed each extension through
`srInitPlugin`, and invoked the recovered importer and exporter vtables. Both original and rebuilt
extensions behaved identically for RGB input, IJG-generated grayscale input, a truncated JPEG, and
malformed non-JPEG data. Three consecutive RGB imports in one process also produced the same
surface dimensions and success state, exercising cleanup of the global active-stream bridge.

The output tests use `srBinOMStream` from the original `sr.dll`, including its hidden complete-object
constructor flag and virtual `srBinStream` base. Every case was repeated three times. Original and
rebuilt output was byte-identical according to size and FNV-1a:

| Option | Encoded bytes | FNV-1a |
| --- | ---: | --- |
| absent | 301,859 | `5c89dfdb` |
| `QUALITY=0.1` | 18,730 | `7479d645` |
| `QUALITY=1.0` | 301,859 | `5c89dfdb` |
| `QUALITY=bogus` | 7,982 | `79ece4cf` |

All outputs begin with the JPEG `FFD8` marker. Destruction of each stream and surface, deletion of
the plugin, `FreeLibrary`, `srExit`, and repeated process runs completed without an exception or
Wine error. This validates the recovered normalized `QUALITY` semantics as well as the
`srBinOStream` primary-vptr/vbptr ABI against the original engine.
