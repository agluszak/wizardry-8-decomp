# Reconstructed Wizardry SGP project. The released SFI project is the source
# ancestor; this list records the object contributions present in Wizardry.

add_library(WIZ8_SGP STATIC
    sgp.cpp
    input.cpp
    vsurface.cpp
    Container.cpp
    DEBUG.cpp
    MemMan.cpp
    FileMan.cpp
    vobject.cpp
    timer.cpp
    Font.cpp
    soundman.cpp
    mousesystem.cpp
    "Button System.cpp"
    "Cursor Control.cpp"
    "Button Sound Control.cpp"
    RegInst.cpp
    "DirectDraw Calls.cpp"
    "DirectX Common.cpp"
    LibraryDataBase.cpp
    WizLibs.cpp
    line.cpp
    himage.cpp
    STCI.cpp
    PCX.cpp
    impTGA.cpp
    vobject_blitters.cpp
    shading.cpp
    Compression.cpp
    Random.cpp
    English.cpp
)

# The retained non-JA2 cursor entry folds to the byte-zero body at 0x5a1140;
# MSYS_UpdateMouseRegion still calls it. Button creation calls the scheme-byte
# writer at 0x413fb0. The product ButtonSound.cpp is a different component.
# Released units without a Wizardry contribution are not part of this source
# tree; README.md records them and the pinned released baseline.

set_source_files_properties(DEBUG.cpp PROPERTIES COMPILE_DEFINITIONS _NO_DEBUG_TXT)
target_compile_options(WIZ8_SGP PRIVATE /nologo /Z7 /O2 /Ob2)
if(NOT WIZ8_ANALYSIS_BUILD)
    target_compile_options(WIZ8_SGP PRIVATE /G5 /MD)
endif()
target_include_directories(WIZ8_SGP PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(WIZ8_SGP PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/wiz8"
    "${PROJECT_SOURCE_DIR}/include/wiz8/engine_code"
    "${PROJECT_SOURCE_DIR}/include/wiz8/sgp-compat"
)
# DirectX Common.cpp owns the two DirectDraw IIDs; search the SDK archive after
# extracting the SGP objects for the remaining GUIDs.
target_link_libraries(WIZ8_SGP PUBLIC dxguid.lib)

# The retained vendor library keeps its own diagnostics profile in the clang
# lane: only decompilation-correctness warnings gate it.
wiz8_lint_target(WIZ8_SGP VENDOR)
