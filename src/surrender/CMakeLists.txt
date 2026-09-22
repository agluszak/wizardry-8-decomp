add_library(wiz8_surrender_objects OBJECT
    bounder.cpp
    camera.cpp
    clipplane.cpp
    config.cpp
    core.cpp
    debug.cpp
    debug_dd.cpp
    debug_vp.cpp
    dynamic_library.cpp
    environment_mapper.cpp
    exponent_table.cpp
    extension.cpp
    fog.cpp
    file_stream.cpp
    generic_vp.cpp
    gerd.cpp
    gerd_filters.cpp
    heap.cpp
    huffman.cpp
    io_managers.cpp
    model_instance.cpp
    node.cpp
    stream.cpp
    string_table.cpp
    system.cpp
    palette.cpp
    timer.cpp
    variable_timer.cpp
    triangulator.cpp
    type_registry.cpp
    vc6_runtime.cpp
    vector_processor.cpp
)
target_compile_definitions(wiz8_surrender_objects PRIVATE SURRENDER_BUILD)
target_compile_options(wiz8_surrender_objects PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/GX->")
set_source_files_properties(
    bounder.cpp
    camera.cpp
    clipplane.cpp
    config.cpp
    debug.cpp
    debug_vp.cpp
    extension.cpp
    fog.cpp
    file_stream.cpp
    gerd_filters.cpp
    heap.cpp
    huffman.cpp
    stream.cpp
    type_registry.cpp
    vector_processor.cpp
    PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/GX>")
target_link_libraries(wiz8_surrender_objects PRIVATE wiz8_compile_settings)
wiz8_lint_target(wiz8_surrender_objects)

if(WIZ8_BUILD_PRODUCTS)
    add_library(SURRENDER SHARED
        $<TARGET_OBJECTS:wiz8_surrender_objects>
        sr.def
    )
    # msvcrt must resolve ??3 (operator delete) before msvcprt's delop_s.obj
    # claims it; retail imports ??3 from MSVCRT.dll.
    target_link_libraries(SURRENDER PRIVATE msvcrt.lib wiz8_compile_settings version.lib)
    target_link_options(SURRENDER PRIVATE
        /DEBUG /DEBUGTYPE:CV /INCREMENTAL:NO
        /OPT:NOREF /OPT:NOICF /BASE:0x10000000 /FILEALIGN:0x1000
        /OSVERSION:4.0 /SUBSYSTEM:WINDOWS,4.0
        # Comparison image, not a runnable replacement: emitted vtables still
        # reference unrecovered virtuals (srDebugDD/srNode/srVP machinery) and
        # other first-party bodies remain outstanding. Never link clients here.
        /FORCE:UNRESOLVED
        "/MAP:${CMAKE_BINARY_DIR}/sr.map"
    )
    set_target_properties(SURRENDER PROPERTIES
        OUTPUT_NAME sr
        PREFIX ""
        PDB_NAME sr
    )
    reccmp_add_target(SURRENDER)
endif()
