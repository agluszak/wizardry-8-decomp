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
        ${WIZ8_SOURCE_UNITS}
    )
    # Product code consumes the reconstructed SGP component's owning headers.
    target_link_libraries(wiz8_recovered_objects PRIVATE wiz8_compile_settings)

    wiz8_add_import_library(WIZ8_SR src/wiz8/imports/sr.def
        IMPORT_OBJECTS "${CMAKE_CURRENT_BINARY_DIR}/sr-assert-import.lib")
    wiz8_add_import_library(WIZ8_MSS32 src/wiz8/imports/mss32.def PRESERVE_C_DECORATION)
    wiz8_add_import_library(WIZ8_BINKW32 src/wiz8/imports/binkw32.def PRESERVE_C_DECORATION)

    # Every product uses the same SGP archive. Comparison can also inspect
    # individual object contributions when a function is discarded at link.
    wiz8_add_executable(
        TARGET WIZ8
        OUTPUT Wiz8
        MAP Wiz8.map
        ALLOW_UNRESOLVED
        LIBRARIES WIZ8_SGP ddraw.lib gdi32.lib user32.lib
    )
    reccmp_add_target(WIZ8 ID WIZ8)

    wiz8_add_executable(
        TARGET WIZ8_RUNTIME
        OUTPUT Wiz8Runtime
        MAP Wiz8Runtime.map
        OPT_REF
        ALLOW_UNRESOLVED
        LIBRARIES WIZ8_SGP ddraw.lib gdi32.lib user32.lib
    )
    # The semantic test product links the same recovered and retained runtime
    # objects, but owns a console entry point and test driver that never enter
    # either matching image.
    wiz8_add_executable(
        TARGET WIZ8_RUNTIME_TEST
        OUTPUT Wiz8RuntimeTest
        MAP Wiz8RuntimeTest.map
        OPT_REF
        CONSOLE
        ALLOW_UNRESOLVED
        SOURCES tests/runtime/wiz8_runtime_test.cpp
        LIBRARIES WIZ8_SGP ddraw.lib gdi32.lib user32.lib
    )
