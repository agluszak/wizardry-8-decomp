set(WIZ8_SGP_RUNTIME_SOURCES
    "${SGP_SOURCE}/Compression.c"
    "${SGP_SOURCE}/RegInst.c"
    "${SGP_SOURCE}/Random.c"
    "${SGP_SOURCE}/soundman.c"
    "${SGP_SOURCE}/timer.c"
    "${SGP_SOURCE}/shading.c"
    "${SGP_SOURCE}/mousesystem.c"
    "${SGP_SOURCE}/DirectDraw Calls.c"
    "${SGP_SOURCE}/DirectX Common.c"
    "${SGP_SOURCE}/FileMan.c"
    "${SGP_SOURCE}/LibraryDataBase.c"
    "${SGP_SOURCE}/Container.c"
    "${SGP_SOURCE}/Button System.c"
    "${SGP_SOURCE}/Font.c"
    "${SGP_SOURCE}/vobject.c"
    "${SGP_SOURCE}/himage.c"
    "${SGP_SOURCE}/STCI.c"
    "${SGP_SOURCE}/PCX.C"
    "${SGP_SOURCE}/impTGA.c"
    "${SGP_SOURCE}/vobject_blitters.c"
    "${SGP_SOURCE}/DEBUG.C"
    "${SGP_SOURCE}/English.c"
    "${SGP_SOURCE}/input.c"
    "${SGP_SOURCE}/sgp.c"
    "${SGP_SOURCE}/MemMan.c"
    "${SGP_SOURCE}/line.c"
    "${SGP_SOURCE}/vsurface.c"
)

set(WIZ8_SGP_ANALYSIS_ONLY_SOURCES
    "${SGP_SOURCE}/Compression.c"
    "${SGP_SOURCE}/DbMan.c"
    "${SGP_SOURCE}/ExceptionHandling.cpp"
)

set_source_files_properties(
    "${SGP_SOURCE}/PCX.C"
    "${SGP_SOURCE}/DEBUG.C"
    PROPERTIES LANGUAGE C
)
set_source_files_properties("${SGP_SOURCE}/vsurface.c" PROPERTIES
    COMPILE_DEFINITIONS "FillSurfaceRect=SgpReleasedFillSurfaceRect"
    COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/sgp-compat/video2.h"
)
# Retail's font printers keep first-party bodies (they lock retail surfaces),
# but the stateless accessors below are byte-identical to the oracle (see
# build/reports/sgp/harness.csv) and link from it; only the modified palette
# setter, the file-backed loader, and the two printers stay first-party.
# The vanilla spellings of those stay available under SgpReleased names for
# the untouched Font.c internals.
set_source_files_properties("${SGP_SOURCE}/Font.c" PROPERTIES
    COMPILE_DEFINITIONS
        "SetFontObjectPalette16BPP=SgpReleasedSetFontObjectPalette16BPP;LoadFontFile=SgpReleasedLoadFontFile;gprintf=SgpReleasedGprintf;mprintf=SgpReleasedMprintf"
)
set_source_files_properties("${SGP_SOURCE}/himage.c" PROPERTIES
    COMPILE_DEFINITIONS WIZ8_EXTERNAL_PIXEL_FORMAT
)
set_source_files_properties("${SGP_SOURCE}/soundman.c" PROPERTIES
    COMPILE_OPTIONS "/FI${SGP_SOURCE}/LibraryDataBase.h"
)
set_source_files_properties("${SGP_SOURCE}/DirectDraw Calls.c" PROPERTIES
    COMPILE_OPTIONS "/FI${SGP_SOURCE}/sgp.h"
)
set_source_files_properties("${SGP_SOURCE}/DEBUG.C" PROPERTIES
    COMPILE_OPTIONS "/FI${SGP_SOURCE}/VObject.h"
)
set_source_files_properties("${SGP_SOURCE}/ExceptionHandling.cpp" PROPERTIES
    COMPILE_OPTIONS "/FIwindows.h"
)
set_source_files_properties("${SGP_SOURCE}/sgp.c" PROPERTIES
    COMPILE_DEFINITIONS "WinMain=SgpRetainedWinMain"
)
set_source_files_properties("${SGP_SOURCE}/mousesystem.c" PROPERTIES
    COMPILE_DEFINITIONS "RenderFastHelp=SgpReleasedRenderFastHelp"
)

function(wiz8_add_sgp_objects target profile)
    add_library(${target} OBJECT EXCLUDE_FROM_ALL ${ARGN})
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/sgp-compat"
        "${SGP_SOURCE}"
    )
    target_compile_options(${target} PRIVATE /nologo /Z7)
    if(profile STREQUAL "MATCH")
        target_compile_options(${target} PRIVATE /O2 /Ob2 /G5)
    elseif(profile STREQUAL "DEBUG")
        target_compile_options(${target} PRIVATE /Od /Oy- /Ob0)
    else()
        message(FATAL_ERROR "unknown SGP compile profile: ${profile}")
    endif()
    target_compile_definitions(${target} PRIVATE
        gusAlphaMask=g_alpha_mask_650f48
        gusRedMask=g_red_mask_650f4a
        gusGreenMask=g_green_mask_650f4c
        gusBlueMask=g_blue_mask_650f4e
        gusRedShift=g_red_shift_650f50
        gusBlueShift=g_blue_shift_650f52
        gusGreenShift=g_green_shift_650f54
        ghTinyMonoFont=g_tiny_mono_font_683690
    )
endfunction()

wiz8_add_sgp_objects(WIZ8_SGP_RUNTIME MATCH ${WIZ8_SGP_RUNTIME_SOURCES})
wiz8_add_sgp_objects(WIZ8_SGP_ANALYSIS MATCH ${WIZ8_SGP_ANALYSIS_ONLY_SOURCES})
