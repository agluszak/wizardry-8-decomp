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
