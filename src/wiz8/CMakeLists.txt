if(ZLIB_SOURCE)
    # The canonical executable statically links pristine zlib 1.0.4; 46 of its
    # bodies are already reviewed by their source markers and provenance claims.
    # Build the pinned source as a real library so bring-up resolves inflate*
    # by linking it, rather than by stubbing library code.
    wiz8_collect_zlib_units(WIZ8_ZLIB_UNITS "${ZLIB_SOURCE}")
    list(TRANSFORM WIZ8_ZLIB_UNITS PREPEND "${ZLIB_SOURCE}/")
    add_library(WIZ8_ZLIB_1_0_4 STATIC EXCLUDE_FROM_ALL ${WIZ8_ZLIB_UNITS})
    target_compile_options(WIZ8_ZLIB_1_0_4 PRIVATE /nologo /O2 /MD)
    target_compile_definitions(WIZ8_ZLIB_1_0_4 PRIVATE NDEBUG WIN32)
    target_include_directories(WIZ8_ZLIB_1_0_4 PRIVATE "${ZLIB_SOURCE}")
endif()

    include(src/wiz8/sources.cmake)
    # Compile recovered first-party sources once, then reuse those exact
    # objects in the comparison, runtime and semantic-test products.
    add_library(wiz8_recovered_objects OBJECT
        ${WIZ8_ORIGINAL_UNITS}
        ${WIZ8_TEMPLATE_EMISSIONS}
        ${WIZ8_PROVISIONAL_AND_QUARANTINE_UNITS}
    )
    # The vendored SGP headers are on the path so recovered bodies that call
    # into SGP declare its types from the real header rather than restating
    # them. GETFILESTRUCT is the first: LoadSaveGame.cpp walks the save
    # directory through GetFileFirst.
    target_link_libraries(wiz8_recovered_objects PRIVATE wiz8_compile_settings)

    add_library(wiz8_debug_objects OBJECT
        ${WIZ8_ORIGINAL_UNITS}
        ${WIZ8_TEMPLATE_EMISSIONS}
        ${WIZ8_PROVISIONAL_AND_QUARANTINE_UNITS}
    )
    target_link_libraries(wiz8_debug_objects PRIVATE wiz8_debug_compile_settings)

    wiz8_add_import_library(WIZ8_SR src/wiz8/imports/sr.def
        IMPORT_OBJECTS "${CMAKE_CURRENT_BINARY_DIR}/sr-assert-import.lib")
    wiz8_add_import_library(WIZ8_MSS32 src/wiz8/imports/mss32.def PRESERVE_C_DECORATION)
    wiz8_add_import_library(WIZ8_BINKW32 src/wiz8/imports/binkw32.def PRESERVE_C_DECORATION)

    # The comparison image retains every recovered first-party section with
    # /OPT:NOREF. Released SGP source belongs only to the /OPT:REF runtime
    # products; source-backed retail identities are compared by the SGP oracle.
    wiz8_add_executable(
        TARGET WIZ8_BRINGUP
        OUTPUT Wiz8
        MAP Wiz8.map
        ALLOW_UNRESOLVED
        LIBRARIES ddraw.lib gdi32.lib user32.lib
    )
    reccmp_add_target(WIZ8_BRINGUP ID WIZ8)
    add_custom_target(WIZ8 DEPENDS WIZ8_BRINGUP)

    wiz8_add_executable(
        TARGET WIZ8_RUNTIME
        OUTPUT Wiz8Runtime
        MAP Wiz8Runtime.map
        OPT_REF
        ALLOW_UNRESOLVED
        LIBRARIES ddraw.lib dxguid.lib gdi32.lib user32.lib
    )
    add_custom_target(WIZ8_RUNNABLE DEPENDS WIZ8_RUNTIME)

    # The semantic test product links the same recovered and retained runtime
    # objects, but owns a console entry point and test driver that never enter
    # either matching image.
    wiz8_add_executable(
        TARGET WIZ8_RUNTIME_MATCH
        OUTPUT Wiz8RuntimeMatch
        MAP Wiz8RuntimeMatch.map
        OPT_REF
        CONSOLE
        ALLOW_UNRESOLVED
        SOURCES tests/runtime/wiz8_runtime_test.cpp
        LIBRARIES ddraw.lib dxguid.lib gdi32.lib user32.lib
    )
    wiz8_add_executable(
        TARGET WIZ8_RUNTIME_DEBUG
        OUTPUT Wiz8RuntimeDebug
        MAP Wiz8RuntimeDebug.map
        OPT_REF
        CONSOLE
        ALLOW_UNRESOLVED
        OBJECT_TARGET wiz8_debug_objects
        COMPILE_SETTINGS wiz8_debug_compile_settings
        SOURCES tests/runtime/wiz8_runtime_test.cpp
        LIBRARIES ddraw.lib dxguid.lib gdi32.lib user32.lib
    )
if(SGP_SOURCE)
    include(cmake/Sgp.cmake)

    target_sources(WIZ8_RUNTIME PRIVATE
        $<TARGET_OBJECTS:WIZ8_SGP_RUNTIME>
    )
    target_sources(WIZ8_RUNTIME_MATCH PRIVATE
        $<TARGET_OBJECTS:WIZ8_SGP_RUNTIME>
    )
    wiz8_add_sgp_objects(WIZ8_SGP_DEBUG DEBUG ${WIZ8_SGP_RUNTIME_SOURCES})
    target_sources(WIZ8_RUNTIME_DEBUG PRIVATE
        $<TARGET_OBJECTS:WIZ8_SGP_DEBUG>
    )
endif()
