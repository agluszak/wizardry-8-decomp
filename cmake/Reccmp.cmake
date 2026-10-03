function(reccmp_configure)
    add_custom_target(reccmp-products DEPENDS ${ARGN})

    # Images live at the build root. Use filenames in the host manifest,
    # rather than the compiler container's Z: paths.
    set(content "project: '../..'\nsource-index: '../source-index.json'\ntargets:\n")
    foreach(target IN LISTS ARGN)
        string(APPEND content
            "  ${target}:\n"
            "    path: '$<TARGET_FILE_NAME:${target}>'\n"
            "    pdb: '$<TARGET_PDB_FILE_NAME:${target}>'\n"
        )
    endforeach()
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/reccmp-build.yml" CONTENT "${content}")
endfunction()
