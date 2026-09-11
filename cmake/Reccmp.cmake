function(reccmp_add_target TARGET)
    set_property(GLOBAL APPEND PROPERTY WIZ8_RECCMP_TARGETS ${TARGET})
endfunction()

function(reccmp_configure)
    get_property(targets GLOBAL PROPERTY WIZ8_RECCMP_TARGETS)

    add_custom_target(reccmp-products DEPENDS ${targets})

    # The product build runs inside the VC6 container, so $<TARGET_FILE:...>
    # would record the container's Z: view. reccmp runs on the host and joins
    # these paths with the config's own directory, so record each target's
    # actual output directory relative to the build root, with the file name
    # resolved by the generator.
    set(content "project: '../..'\ntargets:\n")
    foreach(target IN LISTS targets)
        get_target_property(output_dir ${target} RUNTIME_OUTPUT_DIRECTORY)
        if(NOT output_dir)
            get_target_property(output_dir ${target} BINARY_DIR)
        endif()
        file(RELATIVE_PATH output_relative "${CMAKE_BINARY_DIR}" "${output_dir}")
        if(output_relative STREQUAL "" OR output_relative STREQUAL ".")
            set(output_prefix "")
        else()
            set(output_prefix "${output_relative}/")
        endif()
        string(APPEND content
            "  ${target}:\n"
            "    path: '${output_prefix}$<TARGET_FILE_NAME:${target}>'\n"
            "    pdb: '${output_prefix}$<TARGET_PDB_FILE_NAME:${target}>'\n"
        )
    endforeach()
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/reccmp-build.yml" CONTENT "${content}")
endfunction()
