function(wiz8_load_sgp_model)
    file(READ "${WIZ8_REPO_ROOT}/config/sgp.yml" model)
    string(JSON flag_count LENGTH "${model}" project_flags)
    math(EXPR flag_last "${flag_count} - 1")
    set(project_flags)
    foreach(index RANGE 0 ${flag_last})
        string(JSON flag GET "${model}" project_flags ${index})
        list(APPEND project_flags "${flag}")
    endforeach()

    string(JSON unit_count LENGTH "${model}" units)
    math(EXPR unit_last "${unit_count} - 1")
    foreach(index RANGE 0 ${unit_last})
        string(JSON source GET "${model}" units ${index} source)
        string(JSON overlay GET "${model}" units ${index} overlay)
        string(JSON use_pch GET "${model}" units ${index} pch)
        set(source_path "${SGP_SOURCE}/${source}")
        set(definitions)
        if(use_pch)
            list(APPEND definitions WIZ8_PRECOMPILED_HEADERS)
        endif()
        string(JSON definition_count ERROR_VARIABLE definitions_error
            LENGTH "${model}" units ${index} definitions)
        if(definitions_error STREQUAL "NOTFOUND")
            math(EXPR definition_last "${definition_count} - 1")
            foreach(definition_index RANGE 0 ${definition_last})
                string(JSON definition GET "${model}"
                    units ${index} definitions ${definition_index})
                list(APPEND definitions "${definition}")
            endforeach()
        endif()
        set_source_files_properties("${source_path}" PROPERTIES
            COMPILE_OPTIONS "/I${WIZ8_REPO_ROOT}/config/sgp-overlays/${overlay}"
            COMPILE_DEFINITIONS "${definitions}"
        )
        string(JSON language ERROR_VARIABLE language_error
            GET "${model}" units ${index} language)
        if(language_error STREQUAL "NOTFOUND")
            set_source_files_properties("${source_path}" PROPERTIES LANGUAGE "${language}")
        endif()
        string(JSON group_count LENGTH "${model}" units ${index} groups)
        math(EXPR group_last "${group_count} - 1")
        foreach(group_index RANGE 0 ${group_last})
            string(JSON group GET "${model}" units ${index} groups ${group_index})
            list(APPEND ${group}_SOURCES "${source_path}")
        endforeach()
    endforeach()
    set(WIZ8_SGP_PROJECT_FLAGS "${project_flags}" PARENT_SCOPE)
    foreach(group WIZ8_SGP_RETAINED WIZ8_SGP_RUNTIME_SHARED
                  WIZ8_SGP_RUNTIME_CORE WIZ8_SGP_PROBE_ONLY)
        set(${group}_SOURCES "${${group}_SOURCES}" PARENT_SCOPE)
    endforeach()
endfunction()

function(wiz8_add_sgp_group target)
    add_library(${target} OBJECT EXCLUDE_FROM_ALL ${ARGN})
    target_include_directories(${target} PRIVATE
        "${WIZ8_REPO_ROOT}/config/sgp-overlays/common"
        "${SGP_SOURCE}"
    )
    target_compile_options(${target} PRIVATE ${WIZ8_SGP_PROJECT_FLAGS})
endfunction()
