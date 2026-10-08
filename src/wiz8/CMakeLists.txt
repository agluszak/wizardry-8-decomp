set(ZLIB_SOURCE "" CACHE PATH "Pinned zlib 1.0.4 source tree used by recovered game code")

# Only the common libraries and output naming are shared. Each product below
# spells out its source objects and linker policy.
function(wiz8_configure_executable TARGET OUTPUT)
    add_dependencies(${TARGET}
        ${WIZ8_SR_IMPORT_TARGET}
        ${WIZ8_MSS32_IMPORT_TARGET}
        ${WIZ8_BINKW32_IMPORT_TARGET}
    )
    target_link_libraries(${TARGET} PRIVATE
        wiz8_compile_settings
        WIZ8_SGP ddraw.lib gdi32.lib user32.lib
        "${WIZ8_SR_IMPORT_LIBRARY}"
        "${WIZ8_MSS32_IMPORT_LIBRARY}"
        "${WIZ8_BINKW32_IMPORT_LIBRARY}"
        WIZ8_ZLIB_1_0_4
    )
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "${OUTPUT}")
endfunction()

# The canonical executable statically links pristine zlib 1.0.4; 46 of its
# bodies are already reviewed by their source markers and provenance claims.
# Build the pinned source as a real library to resolve inflate*
# by linking it, rather than by stubbing library code.
# GLOB is lexically sorted; preserve the existing archive order.
file(GLOB WIZ8_ZLIB_UNITS CONFIGURE_DEPENDS "${ZLIB_SOURCE}/*.c")
list(REMOVE_ITEM WIZ8_ZLIB_UNITS
    "${ZLIB_SOURCE}/example.c" "${ZLIB_SOURCE}/minigzip.c")
add_library(WIZ8_ZLIB_1_0_4 STATIC EXCLUDE_FROM_ALL ${WIZ8_ZLIB_UNITS})
target_compile_options(WIZ8_ZLIB_1_0_4 PRIVATE /nologo /O2 /MD)
target_compile_definitions(WIZ8_ZLIB_1_0_4 PRIVATE NDEBUG WIN32)
target_include_directories(WIZ8_ZLIB_1_0_4 PRIVATE "${ZLIB_SOURCE}")

include(sources.cmake)
set(WIZ8_SOURCE_FILES ${WIZ8_SOURCE_UNITS})
list(TRANSFORM WIZ8_SOURCE_FILES PREPEND "${PROJECT_SOURCE_DIR}/")

if(WIZ8_ANALYSIS_BUILD)
    # These units keep retail's reads of locals that are unset on a defect path
    # (a short-circuited FileRead, a missing button or entity, an unhandled
    # mode). Each read is confirmed against retail: initializing the local adds
    # a store the retail body does not have. Keep the diagnostic visible, but
    # do not fail lint or indexing on those recovered paths.
    set_source_files_properties(
        "${PROJECT_SOURCE_DIR}/src/wiz8/dialog_code/PortraitQuote.cpp"
        "${PROJECT_SOURCE_DIR}/src/wiz8/dialog_code/stListBox.cpp"
        "${PROJECT_SOURCE_DIR}/src/wiz8/engine_code/ReadLevel.cpp"
        "${PROJECT_SOURCE_DIR}/src/wiz8/engine_code/ReadMesh.cpp"
        "${PROJECT_SOURCE_DIR}/src/wiz8/engine_code/Trigger.cpp"
        "${PROJECT_SOURCE_DIR}/src/wiz8/engine_code/Video2.cpp"
        "${PROJECT_SOURCE_DIR}/src/wiz8/level_specific_code/Cosmic Circle.cpp"
        "${PROJECT_SOURCE_DIR}/src/wiz8/local_code/Combat Attack.cpp"
        "${PROJECT_SOURCE_DIR}/src/wiz8/local_screens/AutomapScreen.cpp"
        PROPERTIES COMPILE_OPTIONS
        "-Wno-error=sometimes-uninitialized" # uninit-ok: retail reads these unset locals
    )
endif()
set(WIZ8_INSTRUMENTABLE_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/src/wiz8/local_code/Gameloop.cpp"
    "${PROJECT_SOURCE_DIR}/src/wiz8/local_code/RegionManager.cpp"
    "${PROJECT_SOURCE_DIR}/src/wiz8/engine_code/Video2.cpp"
    "${PROJECT_SOURCE_DIR}/src/wiz8/local_code/QuoteManager.cpp"
)
list(REMOVE_ITEM WIZ8_SOURCE_FILES ${WIZ8_INSTRUMENTABLE_SOURCE_FILES})

# Share the common recovered objects across products. Only the selected units
# are compiled again for the instrumented runtime test image. Recovered game
# units build with /Ob2, like the SGP library.
add_library(wiz8_recovered_objects OBJECT ${WIZ8_SOURCE_FILES})
target_link_libraries(wiz8_recovered_objects PRIVATE wiz8_compile_settings)
target_compile_options(wiz8_recovered_objects PRIVATE /Ob2)
wiz8_lint_target(wiz8_recovered_objects)
add_library(wiz8_recovered_instrumentable_objects OBJECT ${WIZ8_INSTRUMENTABLE_SOURCE_FILES})
target_link_libraries(wiz8_recovered_instrumentable_objects PRIVATE wiz8_compile_settings)
target_compile_options(wiz8_recovered_instrumentable_objects PRIVATE /Ob2)
wiz8_lint_target(wiz8_recovered_instrumentable_objects)

if(NOT WIZ8_ANALYSIS_BUILD)
    wiz8_add_import_library(WIZ8_SR imports/sr.def
        IMPORT_OBJECTS "${CMAKE_BINARY_DIR}/sr-assert-import.lib")
    wiz8_add_import_library(WIZ8_MSS32 imports/mss32.def PRESERVE_C_DECORATION)
    wiz8_add_import_library(WIZ8_BINKW32 imports/binkw32.def PRESERVE_C_DECORATION)

    # Every product uses the same SGP archive. Comparison can also inspect
    # individual object contributions when a function is discarded at link.
    add_executable(WIZ8 WIN32
        $<TARGET_OBJECTS:wiz8_recovered_objects>
        $<TARGET_OBJECTS:wiz8_recovered_instrumentable_objects>
    )
    wiz8_configure_executable(WIZ8 Wiz8)
    target_link_options(WIZ8 PRIVATE
        /DEBUG /DEBUGTYPE:CV /PDBTYPE:SEPT /INCREMENTAL:NO /OPT:NOREF /OPT:NOICF /FIXED:NO
        /FORCE:UNRESOLVED /BASE:0x400000 /FILEALIGN:0x1000
        /OSVERSION:4.0 /SUBSYSTEM:WINDOWS,4.0
        /STACK:0x100000,0x1000 /HEAP:0x100000,0x1000
        "/MAP:${CMAKE_BINARY_DIR}/Wiz8.map" /MAPINFO:LINES
    )
endif()

# Test products share the same recovered objects but do not belong to the game source model.
include("${PROJECT_SOURCE_DIR}/tests/runtime/targets.cmake")
