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
    vertex_pipe.cpp
    file_stream.cpp
    filter.cpp
    color_surface.cpp
    generic_vp.cpp
    gerd.cpp
    gerd_filters.cpp
    global_recycler.cpp
    heap.cpp
    huffman.cpp
    illuminator.cpp
    io_managers.cpp
    light.cpp
    material.cpp
    memory_pool.cpp
    mesh_model.cpp
    triangle_culler.cpp
    model.cpp
    model_instance.cpp
    modeler.cpp
    mutex.cpp
    node.cpp
    scene.cpp
    stream.cpp
    string_table.cpp
    system.cpp
    palette.cpp
    pixel_convert.cpp
    renderer.cpp
    texture.cpp
    texture_file.cpp
    texture_map.cpp
    scheduler.cpp
    statistics.cpp
    thread.cpp
    timer.cpp
    variable_timer.cpp
    triangulator.cpp
    type_registry.cpp
    vc6_runtime.cpp
    video_manager.cpp
    window.cpp
    window_out.cpp
    vector_processor.cpp
)
target_compile_definitions(wiz8_surrender_objects PRIVATE SURRENDER_BUILD)
target_compile_options(wiz8_surrender_objects PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/GX->")
set_source_files_properties(
    bounder.cpp
    camera.cpp
    clipplane.cpp
    color_surface.cpp
    config.cpp
    core.cpp
    debug.cpp
    debug_vp.cpp
    dynamic_library.cpp
    extension.cpp
    fog.cpp
    vertex_pipe.cpp
    file_stream.cpp
    gerd.cpp
    gerd_filters.cpp
    global_recycler.cpp
    heap.cpp
    huffman.cpp
    illuminator.cpp
    io_managers.cpp
    light.cpp
    material.cpp
    mesh_model.cpp
    model.cpp
    model_instance.cpp
    modeler.cpp
    mutex.cpp
    node.cpp
    palette.cpp
    renderer.cpp
    scene.cpp
    scheduler.cpp
    stream.cpp
    string_table.cpp
    texture.cpp
    texture_file.cpp
    texture_map.cpp
    type_registry.cpp
    vector_processor.cpp
    video_manager.cpp
    window_out.cpp
    PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/GX>")
target_link_libraries(wiz8_surrender_objects PRIVATE wiz8_compile_settings)
wiz8_lint_target(wiz8_surrender_objects)

if(NOT WIZ8_ANALYSIS_BUILD)
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/surrender-objects.txt"
        CONTENT "$<JOIN:$<TARGET_OBJECTS:wiz8_surrender_objects>,\n>\n")
    wiz8_compile_resource(SURRENDER_RESOURCE_OBJECT sr.rc)

    add_library(SURRENDER SHARED
        $<TARGET_OBJECTS:wiz8_surrender_objects>
        sr.def
        "${SURRENDER_RESOURCE_OBJECT}"
    )
    # msvcrt must resolve ??3 (operator delete) before msvcprt's delop_s.obj
    # claims it; retail imports ??3 from MSVCRT.dll.
    target_link_libraries(SURRENDER PRIVATE
        msvcrt.lib wiz8_compile_settings version.lib
        # srTimer's hi-resolution path calls timeGetTime; retail sr.dll imports
        # WINMM.dll for it.
        winmm.lib
    )
    target_link_options(SURRENDER PRIVATE
        /DEBUG /DEBUGTYPE:CV /INCREMENTAL:NO
        /OPT:NOREF /OPT:NOICF /BASE:0x10000000 /FILEALIGN:0x1000
        /OSVERSION:4.0 /SUBSYSTEM:WINDOWS,4.0
        "/MAP:${CMAKE_BINARY_DIR}/sr.map"
    )
    set_target_properties(SURRENDER PROPERTIES OUTPUT_NAME sr)

endif()
