# SurRender differential runner

`sr_difftest.exe` is a VC6 console program linked against the SurRender
provider import library. It runs the same cases in separate Wine processes
against different `sr.dll` builds. Retail is the reference. The trace prints
only values and logical identities, never process addresses, so traces from
different DLLs can be diffed line by line.

```sh
# build SURRENDER + runner, then compare retail (reference) with the rebuilt DLL
uv run wiz8 sr-difftest --build
# any number of candidates; the first --dll is the reference
uv run wiz8 sr-difftest \
  --dll retail=$WIZ8_WORK_DIR/variants/gog-base/sr.dll \
  --dll rebuilt=build/decomp/sr.dll \
  --dll other=/path/to/other/sr.dll
# subsets: exact names or trailing-* prefixes
uv run wiz8 sr-difftest 'fixed.transform3*' 'probe.*'
```

Output goes to `build/differential/sr/`: one directory per variant, each
holding its staged DLL set and `trace.txt`, plus `report.json`. The command
exits 1 on any divergence. Each variant directory contains only the runner,
that variant's `sr.dll`, and the retail `MSVCRT.DLL`/`MSVCP60.DLL`, loaded
native-first through `WINEDLLOVERRIDES`. Every container runs with
`--network none`.

Runner options: `--seed N` (default 1), `--generated K` rounds,
`--window FIRST COUNT` (rerun only those elements; the driver uses this for
minimized reproducers), `--blobs FILE` (run the `stream.xread.*` cases on
the blobs in FILE; the driver uses this for cross-reads), `--list`.

## Cases

* `fixed.transform3.swapxy.v123.{separate,inplace}`: the X/Y swap matrix
  applied to (1,2,3). Expected (2,1,3) in both modes.
* `fixed.<transform op>.<matrix>.n4`, `fixed.<op>.general.n0/n1`: these cover
  transform3, transform4, transform3to4, transformOrtho and
  transformPerspective, each with the identity, swapxy, translate, rotz90,
  rotx30, persp, ortho and general matrices. The transforms go through the
  public `srVectorProcessor` wrappers.
* `fixed.transformIndexed*`: repeated indices and the n0 case.
* `gen.<op>.<separate|inplace>.n<count>.r<round>`: deterministic xorshift
  inputs for every op in the table in `sr_difftest.cpp`, at counts
  0/1/2/3/5/8/17/64. The per-case seed comes from the case name plus `--seed`.
  Outputs have guard bytes around them, so underruns and overruns get
  reported. Each op runs under SEH.
* `probe.registry.grow-shrink`: an `srRegistry` with 40 named instances.
  After each insert and each removal, the probe checks that every live name is
  still found. Capacity starts at 4, so the 5th insert grows the table.
* `probe.gerd.driver-lists`: a runner-owned `srDD` (`ProbeDD`) reports 4
  texture formats (ARGB4444, RGB565, BGRA32, P8) and 2 display modes to a real `srGERD::createContext`.
* `probe.gerd.default-texture`: `openWindow` builds the 64x64, 7-level
  default texture through `createNewTexture`'s MultiRequest. The probe
  prints each mip level's size and FNV checksum as `ProbeDD::bindTexture`
  sees it.
* `probe.camera.*`: `srCamera::processPush` and the MODELVIEW matrix for
  identity, translated, rotated and translated+rotated cameras.
* `fixed.clipFlags.boundaries`, `fixed.divByW.boundaries`,
  `fixed.transformPerspective.clip-points.persp`: homogeneous points on, just
  inside and just outside each clip plane, plus `w = 0` and negative `w`.
* `fixed.testBoundingBox.<matrix>.<box>`: boxes inside, straddling, outside,
  exactly on the clip volume, enclosing it, and degenerate (a point), each
  under four matrices.
* `fixed.<transform op>.<yzx|zxy|xzy|zyx|swapxy-translate|projective-w>.n4`:
  axis permutations with nonzero translation, and a projective bottom row.
* `surface.read.<type>` / `surface.write.<type>`: every `e_surfaceType` at
  7x5. The read case fills raw bytes (unused bits included) and decodes them
  with getPixel, getPixelRow, getPixelColumn and getPixels. The write case
  encodes with fill, setPixel, setPixelRow, setPixelColumn, setPixels and
  setPixelRaw.
* `surface.copy.<A>.<B>` and `surface.format.<A>.<B>`: every type pair, run
  through `copy` (same size) and through `changePixelFormat(..., 1)`.
* `surface.scale.<type>.<src>-<dst>.<filter>` and `surface.rescale.*`: six
  types, ten size pairs (2x up, 2x down, mixed, 1x1 edge cases) and five
  filter settings, through `copy` and `rescale`. With the triangle filter at
  exactly 2x, `copy` dispatches to `magnify`/`minify`.
* `surface.op.<type>.<op>`: lines (including clipped and reversed endpoints),
  flips, rotate180, swapPixelRows, flipRectangle, remapPixels, the channel
  copy and swap, adjust, adjustSaturation, getChannelStatistics, both blit
  forms, composite, clamping, and addNoise with a fixed `srand` seed.
* `palette.quantize`: srPalette quantize and matchPalette.
* `huffman.roundtrip.<distribution>.n<count>`: an `srHuffman::Sampler`,
  `Compressor` and `Decompressor` round trip over `srBinOMStream` /
  `srBinIMStream`, using the game's BitArray header (symbol count, 6-bit code
  width, data count) before the symbol table. Distributions: one symbol, two
  symbols, uniform 16, geometric, bytes, wide 32-bit values, Fibonacci
  frequencies (deepest tree). The case prints the sampler table, the packed
  bytes and the decoded symbols.
* `huffman.bitstream`: `BitOStream::put` / `BitIStream::get` at widths 0..32,
  plus `rewind` and `getBit`.
* `culler.setClipFlags.v<n>`, `culler.getClipMask.spheres`,
  `culler.transformClipPlane.matrices` (256 planes, including `z = 0` and
  `w = 0`, under identity, translation, general affine and projective
  matrices), `culler.buildAVT.v<n>` (with `setupLinearArray`) and
  `culler.cull.v<n>` (cull modes 0/1/2, with and without an active-triangle
  list and object-space clipping): the exported `srTriangleCuller` statics.
* `triangulator.v<n>`: `srTriangulator` over convex, star, comb and
  collinear-edge polygons with 3 to 17 points.
* `memorypool.v<n>`: 400-step allocate/free/lock/unlock sequences on an
  `srMemoryPool`, over four alignments, first and best fit, with and without
  `maskArea`.
* `stringtable.v<n>`: `addString`, `addSeparatedStrings` (empty fields,
  several separators, `append_slash`), growth past 64 entries, copy
  assignment and `reset`.
* `exponent.table.v<n>` / `exponent.cached.sequence`: `srExponentTable`
  values across [0, 1], and the `srCachedExponentTable` reuse, release and
  free sequence.
* `palette.quantizer.v<n>` / `palette.sampler.v<n>`: `srPalette::Quantizer`
  over 1 to 256 colours and four channel-bit layouts, and
  `Sampler::createOptimalPalette` over random, clustered, grey and
  coarsely sampled colour sets.

* `stream.write.*`, `stream.read.*`, `stream.file.o<n>`,
  `stream.edge.empty-large`: `srBinOMStream` writes every `operator<<` type
  and the raw writers in both byte orders, and prints the buffer as a
  `blob` line. `srBinIMStream` decodes the same layout through every
  `operator>>` and getter, plus EOF, `clear`, the `seek` variants and the
  `Failure` exception. `srBinOFStream`/`srBinIFStream` repeat the round trip
  through a temporary file.
* `stream.xread.<id>@<variant>-written`: the driver collects every
  variant's `blob` lines, and each variant then decodes every other
  variant's blobs (`--blobs`). The written bytes and the decoded values are
  compared with the reference's decode of its own blobs.
* `texture.mips.<A>.<B>.<WxH>.from<n>.<filter>`: `srTextureMap` over a
  type-A source with odd and small sizes. `getDimensions`,
  `getTextureParms` and `getMipmapData` fill a type-B MultiRequest down to
  1x1, starting at level n, into padded-pitch buffers with guard bytes.
* `texture.gerd.<type>.<WxH>.v<hints>`: `srGERD::setTexture` and an
  explicit `applyDrawStateChanges` (an exported private member, called
  through its export) bind the texture to `ProbeDD`. With and without
  `NO_MIPMAPS`/`MIPMAP_BEST` hints, the probe prints each level's checksum,
  the top level's first rows in the stored device format, and
  `getTextureInfo`.
* `surface.blitclip.<A>.<B>.r<n>`: `blit` with source and destination
  rects that are clipped, partly outside, empty or reversed.
* `modeler.shape.v<n>`: `createSphere`, `createTorus` and `createGrid`
  over the detail ranges, including degenerate counts. The probe prints
  per-triangle hashes, `getUniqueVertexCount`, `getMaxVertexDist` and
  `getAxialBounds`.
* `modeler.xform.v<n>`: random whole-modeler and per-triangle `scale`,
  `move` and `rotate`, followed by `findClosestVertex` and `findVertex`.
* `modeler.rotate.v<n>`, `mesh.apply.v<n>`: four chained random 3x3
  transforms through `srModeler::rotate` (whole modeler, or every triangle
  through the per-triangle overload) and `srMeshModel::applyMatrix`.
  Different summation orders change the float result only rarely, so a
  clean run here is weak evidence; the fixes were checked against the
  retail instructions.
* `modeler.tesselate.v<n>`, `modeler.smooth.v<n>`, `modeler.map.v<n>`,
  `modeler.polygon.v<n>`, `modeler.manage.v<n>`: `tesselateEdges` (whole
  modeler and per triangle), `autoSmooth` followed by `convert`,
  `planarMap`/`planarMapAbsolute`/`cylinderMap`/`removeMapping`, `addPolygon`
  over convex, star, random-radius and non-planar polygons in both
  windings, and triangle editing (degenerate corners, disable/enable, flip,
  `removeDisabledTriangles`, `addFromModeler`, `setTriangleCount`).
* `mesh.convert.v<n>`, `mesh.ops.v<n>`: `srModeler::convert` with both
  flag values into fresh and populated meshes, then `srMeshModel`
  `getTriMesh` (vertex normals, polygon equations, bounds), the
  bounding box and sphere, `getAverageRadius`/`getMaxRadius`,
  `centerVertices`, `applyMatrix`, `scale`, `relocateVertices`,
  `flipFaces`, `scaleToMaxRadius`/`scaleToAverageRadius` and
  `findClosestVertex`.
* `node.xform.v<n>`: a three-level `srNode` hierarchy plus a loose node
  under random `setLocation`, `move*`, `rotate*`, `setRotation` variants,
  `setScale`, `pitchAt`/`yawAt`/`rollAt`, `offsetLocation` and
  `setWorldSpaceLocation`. The probe prints the world matrix in double and
  float, local and world rotation, location, scale and DOF, then repeats
  after `setParent` (with and without preserving the world transform) and
  after `setWorldSpaceMatrix`.
* `bounder.v<n>`: an `srBounder` over `srModelInstance` children (direct
  and under a rotated, scaled group) holding converted meshes. The probe
  prints each child's `getLocalBounds`, then the bounder's bounds after
  `forceUpdateBounds` and again after a child moves and `updateBounds` runs.
* `envmap.v<n>`: `srEnvironmentMapper::isActive`/`process` (called through
  their exports) on a raw `srVertexPipe` image with the eye-direction and
  eye-normal scratch already marked ready. Inputs include unit and
  non-unit normals, direction == normal, and reflections onto (0, 0, -1),
  where the divisor is zero. The probe prints the generated ST0 range, the
  lazy-setup mask and a checksum of the whole guarded ST buffer. The
  setupEyeSpace* paths are not reached.

## Aliasing scope

In-place mode (`dest == source0`) runs only for ops whose in-place use
appears in retail code. The list below was found by scanning retail Wiz8.exe
and sr.dll. A call site counts when it pushes the same register twice for
dest and source, with no write in between, and then makes an srVP vtable
call.

| op | retail call site |
| --- | --- |
| transform3 (+0x224) | Wiz8 0x46EC5C |
| transform4 (+0x220), transformPerspective (+0x240), transformOrtho (+0x23c) | sr 0x10025B52 / 0x10025B5A / 0x10025B68 |
| add3K (+0xd4) | Wiz8 0x46ECAD |
| reverse | sr 0x1005767D |
| subFF / mulFF / clampMin / clampUnit / specularPow | sr 0x1004D497 / 0x10033F50 / 0x1004D4F9 / 0x1002B5A3 / 0x1004D51F |
| axpy4VKFF / axpy4VKF | sr 0x1004D39C / 0x1002BD1B |
| mulIndexed4V / mulIndexedF | sr 0x1002B5D7 / 0x1002B439 |
| normalize3 | sr 0x1003E676, Wiz8 0x472F62 |
| addFF / mul3F | Wiz8 0x47474E / 0x47239C |

The scan is heuristic. It can miss aliasing that runs through different
registers or memory, and other aliasing patterns (partial overlap,
`dest == source1`) are not exercised.

The runner prints each wrapper's vtable slot offset by decoding its
compiled vcall thunk. These offsets match the retail call sites above.

## Caveats

* `ProbeDD` follows `include/surrender/srDD.h`. The probes can only check
  the GERD side of the DD interface; no retail driver DLL is involved.
* The retail list layout `{pointer @0, count @4}` was confirmed from the
  retail `srGERD::initTextureFormats` (0x10018F30) and `initDisplayModeList`
  (0x10018FE0) instructions. Retail provider DLLs were not checked.
* Every variant shares the same Wine build and the same retail CRT, so CRT
  and loader behaviour cannot differ between variants.
* After `resize`, surface contents are uninitialized heap memory, so the
  rescale case prints only the result and the size at that point.
* `srExponentTable::getValue` does not clamp, and an exponent of 0 leaves
  the table unfilled, so the cases stay inside [0, 1] and skip exponent 0.
  Anything else reads stack garbage in both builds.
* Clean traces show agreement only on these inputs. They do not prove
  equivalence.
