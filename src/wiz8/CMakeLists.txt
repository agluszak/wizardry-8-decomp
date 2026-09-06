if(ZLIB_SOURCE)
    # These five functions are Wizardry-owned adapters around pristine zlib
    # 1.0.4, not a second copy of the library implementation.
    add_library(WIZ8_ZLIB_WRAPPERS OBJECT
        src/wiz8/zlib_wrappers.c
    )
    target_include_directories(WIZ8_ZLIB_WRAPPERS PRIVATE "${ZLIB_SOURCE}")

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

    wiz8_add_import_library(WIZ8_SR src/wiz8/imports/sr.def)
    wiz8_add_import_library(WIZ8_MSS32 src/wiz8/imports/mss32.def)
    wiz8_add_import_library(WIZ8_BINKW32 src/wiz8/imports/binkw32.def)

    # The comparison image retains every recovered section with /OPT:NOREF;
    # the runtime image selects only referenced COMDATs with /OPT:REF.
    wiz8_add_executable(
        TARGET WIZ8_BRINGUP
        OUTPUT Wiz8
        MAP Wiz8.map
        LIBRARIES ddraw.lib gdi32.lib user32.lib
    )
    reccmp_add_target(WIZ8_BRINGUP ID WIZ8)
    add_custom_target(WIZ8 DEPENDS WIZ8_BRINGUP)

    wiz8_add_executable(
        TARGET WIZ8_RUNTIME
        OUTPUT Wiz8Runtime
        MAP Wiz8Runtime.map
        OPT_REF
        LIBRARIES ddraw.lib dxguid.lib gdi32.lib user32.lib
    )
    add_custom_target(WIZ8_RUNNABLE DEPENDS WIZ8_RUNTIME)

    # The semantic test product links the same recovered and retained runtime
    # objects, but owns a console entry point and test driver that never enter
    # either matching image.
    wiz8_add_executable(
        TARGET WIZ8_RUNTIME_TEST
        OUTPUT Wiz8RuntimeTest
        MAP Wiz8RuntimeTest.map
        OPT_REF
        CONSOLE
        SOURCES tests/runtime/wiz8_runtime_test.cpp
        LIBRARIES ddraw.lib dxguid.lib gdi32.lib user32.lib
    )
if(SGP_SOURCE)
    include(cmake/Sgp.cmake)

    target_sources(WIZ8_BRINGUP PRIVATE
        $<TARGET_OBJECTS:WIZ8_SGP_WHOLE>
        $<TARGET_OBJECTS:WIZ8_SGP_VSURFACE>
    )
    target_sources(WIZ8_RUNTIME PRIVATE
        $<TARGET_OBJECTS:WIZ8_SGP_WHOLE>
        $<TARGET_OBJECTS:WIZ8_SGP_VSURFACE>
        $<TARGET_OBJECTS:WIZ8_SGP_RUNTIME>
    )
    target_sources(WIZ8_RUNTIME_TEST PRIVATE
        $<TARGET_OBJECTS:WIZ8_SGP_WHOLE>
        $<TARGET_OBJECTS:WIZ8_SGP_VSURFACE>
        $<TARGET_OBJECTS:WIZ8_SGP_RUNTIME>
    )
endif()
