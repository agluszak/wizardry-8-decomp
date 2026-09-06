function(reccmp_add_target TARGET)
    cmake_parse_arguments(ARG "" "ID" "" ${ARGN})
    if(NOT ARG_ID)
        message(FATAL_ERROR "reccmp_add_target requires ID")
    endif()
    set_property(TARGET ${TARGET} PROPERTY WIZ8_RECCMP_ID "${ARG_ID}")
    set_property(GLOBAL APPEND PROPERTY WIZ8_RECCMP_TARGETS ${TARGET})
endfunction()

function(reccmp_configure)
    set(content "project: '../..'\ntargets:\n")
    get_property(targets GLOBAL PROPERTY WIZ8_RECCMP_TARGETS)
    foreach(target IN LISTS targets)
        get_property(id TARGET ${target} PROPERTY WIZ8_RECCMP_ID)
        string(APPEND content
            "  ${id}:\n"
            "    path: '$<TARGET_FILE_NAME:${target}>'\n"
            "    pdb: '$<TARGET_PDB_FILE_NAME:${target}>'\n"
        )
    endforeach()
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/reccmp-build.yml" CONTENT "${content}")
endfunction()
