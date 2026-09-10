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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wiz8/library_database.c"
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
    "${SGP_SOURCE}/LibraryDataBase.c"
)

set_source_files_properties(
    "${SGP_SOURCE}/PCX.C"
    "${SGP_SOURCE}/DEBUG.C"
    PROPERTIES LANGUAGE C
)
set_source_files_properties("${SGP_SOURCE}/vsurface.c" PROPERTIES
    COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/sgp-compat/video2.h"
)
set_source_files_properties("${SGP_SOURCE}/himage.c" PROPERTIES
    COMPILE_DEFINITIONS WIZ8_EXTERNAL_PIXEL_FORMAT
)
set_source_files_properties(
    "${SGP_SOURCE}/FileMan.c"
    "${SGP_SOURCE}/soundman.c"
    PROPERTIES COMPILE_OPTIONS
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/sgp-compat/LibraryDataBase.h"
)
set_source_files_properties("${SGP_SOURCE}/soundman.c" PROPERTIES
    COMPILE_DEFINITIONS
        "InitializeSoundManager=SgpReleasedInitializeSoundManager;ShutdownSoundManager=SgpReleasedShutdownSoundManager"
)
set_source_files_properties("${SGP_SOURCE}/DirectDraw Calls.c" PROPERTIES
    COMPILE_OPTIONS "/FI${SGP_SOURCE}/sgp.h"
)
set_source_files_properties("${SGP_SOURCE}/DEBUG.C" PROPERTIES
    COMPILE_DEFINITIONS _NO_DEBUG_TXT
    COMPILE_OPTIONS "/FI${SGP_SOURCE}/VObject.h"
)
set_source_files_properties("${SGP_SOURCE}/Font.c" PROPERTIES
    COMPILE_DEFINITIONS "CreateEnglishTransTable=SgpReleasedCreateEnglishTransTable"
)
set_source_files_properties("${SGP_SOURCE}/ExceptionHandling.cpp" PROPERTIES
    COMPILE_OPTIONS "/FIwindows.h"
)
set_source_files_properties("${SGP_SOURCE}/sgp.c" PROPERTIES
    COMPILE_DEFINITIONS "WinMain=SgpReleasedWinMain"
    COMPILE_OPTIONS
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/sgp_source_overrides.h"
)
set_source_files_properties("${SGP_SOURCE}/FileMan.c" PROPERTIES
    # Retail uses two 520-byte stack buffers instead of the released heap
    # allocations. Keep that override on FileMan.h's original C interface.
    COMPILE_DEFINITIONS "AddSubdirectoryToPath=SgpReleasedAddSubdirectoryToPath"
)

function(wiz8_add_sgp_target target kind)
    add_library(${target} ${kind} EXCLUDE_FROM_ALL ${ARGN})
    if(kind STREQUAL "STATIC")
        # DirectX Common.c supplies two DirectDraw IIDs itself; search the
        # remaining SDK GUIDs only after extracting SGP's objects.
        target_link_libraries(${target} PUBLIC dxguid.lib)
    endif()
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/sgp-compat"
        "${SGP_SOURCE}"
    )
    target_compile_options(${target} PRIVATE /nologo /Z7)
    target_compile_options(${target} PRIVATE /O2 /Ob2 /G5)
    target_compile_definitions(${target} PRIVATE
        ghTinyMonoFont=g_tiny_mono_font_683690
    )
endfunction()

# Runtime consumers link the dependency archive. Oracle comparisons still use
# the individual compiled objects, including the analysis-only units.
wiz8_add_sgp_target(WIZ8_SGP_RUNTIME STATIC ${WIZ8_SGP_RUNTIME_SOURCES})
wiz8_add_sgp_target(WIZ8_SGP_ANALYSIS OBJECT ${WIZ8_SGP_ANALYSIS_ONLY_SOURCES})
