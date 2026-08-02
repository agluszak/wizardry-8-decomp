set(WIZ8_SGP_WHOLE_SOURCES
    "${SGP_SOURCE}/Random.c"
    "${SGP_SOURCE}/soundman.c"
    "${SGP_SOURCE}/timer.c"
    "${SGP_SOURCE}/shading.c"
)

set(WIZ8_SGP_RUNTIME_PARTIAL_SOURCES
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
    "${SGP_SOURCE}/input.c"
    "${SGP_SOURCE}/sgp.c"
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

function(wiz8_add_sgp_objects target)
    add_library(${target} OBJECT EXCLUDE_FROM_ALL ${ARGN})
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/sgp-compat"
        "${SGP_SOURCE}"
    )
    target_compile_options(${target} PRIVATE /O2 /Ob2 /G5 /MD)
    target_compile_definitions(${target} PRIVATE
        gusAlphaMask=g_alpha_mask_650f48
        gusRedMask=g_red_mask_650f4a
        gusGreenMask=g_green_mask_650f4c
        gusBlueMask=g_blue_mask_650f4e
        gusRedShift=g_red_shift_650f50
        gusBlueShift=g_blue_shift_650f52
        gusGreenShift=g_green_shift_650f54
    )
endfunction()

wiz8_add_sgp_objects(WIZ8_SGP_WHOLE ${WIZ8_SGP_WHOLE_SOURCES})
wiz8_add_sgp_objects(WIZ8_SGP_VSURFACE "${SGP_SOURCE}/vsurface.c")
wiz8_add_sgp_objects(WIZ8_SGP_RUNTIME ${WIZ8_SGP_RUNTIME_PARTIAL_SOURCES})
wiz8_add_sgp_objects(WIZ8_SGP_ANALYSIS ${WIZ8_SGP_ANALYSIS_ONLY_SOURCES})
