# Reconstructed Wizardry SGP project. The released SFI project is the source
# ancestor; this list records the object contributions present in Wizardry.
add_library(WIZ8_SGP STATIC
    sgp.c
    input.c
    vsurface.c
    Container.c
    DEBUG.C
    MemMan.c
    FileMan.c
    vobject.c
    timer.c
    Font.c
    soundman.c
    mousesystem.c
    "Button System.c"
    "Cursor Control.c"
    "Button Sound Control.c"
    RegInst.c
    "DirectDraw Calls.c"
    "DirectX Common.c"
    LibraryDataBase.c
    WizLibs.c
    line.c
    himage.c
    STCI.c
    PCX.C
    impTGA.c
    vobject_blitters.c
    shading.c
    Compression.c
    Random.c
    English.c
)

# The retained non-JA2 cursor entry folds to the byte-zero body at 0x5a1140;
# MSYS_UpdateMouseRegion still calls it. Button creation calls the scheme-byte
# writer at 0x413fb0. The product ButtonSound.cpp is a different component.
# DbMan, Install and WinFont have no recognized retained function/data surface;
# retail imports also exclude WinFont/Mutex APIs. Mutex startup/shutdown and
# ExceptionHandling are disabled in the Wizardry source branch. video.c is the
# JA2 renderer, replaced by Wizardry's Video2.cpp, not a second SGP renderer.

set_source_files_properties(PCX.C DEBUG.C PROPERTIES LANGUAGE C)
set_source_files_properties(DEBUG.C PROPERTIES COMPILE_DEFINITIONS _NO_DEBUG_TXT)
target_compile_options(WIZ8_SGP PRIVATE /nologo /Z7 /O2 /Ob2)
if(NOT WIZ8_LINT_COMPILE_ONLY)
    target_compile_options(WIZ8_SGP PRIVATE /G5)
endif()
target_include_directories(WIZ8_SGP PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(WIZ8_SGP PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/wiz8"
    "${PROJECT_SOURCE_DIR}/include/wiz8/engine_code"
    "${PROJECT_SOURCE_DIR}/include/wiz8/sgp-compat"
)
# DirectX Common.c owns the two DirectDraw IIDs; search the SDK archive after
# extracting the SGP objects for the remaining GUIDs.
target_link_libraries(WIZ8_SGP PUBLIC dxguid.lib)

# The retained vendor library keeps its own diagnostics profile in the clang
# lane: only decompilation-correctness warnings gate it.
wiz8_lint_target(WIZ8_SGP VENDOR)
