add_library(wiz8_surrender_objects OBJECT
    bounder.cpp
    core.cpp
    dynamic_library.cpp
    extension.cpp
    string_table.cpp
    system.cpp
    type_registry.cpp
    vector_processor.cpp
)
target_compile_definitions(wiz8_surrender_objects PRIVATE SURRENDER_BUILD)
target_compile_options(wiz8_surrender_objects PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/GX->")
set_source_files_properties(
    bounder.cpp
    extension.cpp
    type_registry.cpp
    PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/GX>")
target_link_libraries(wiz8_surrender_objects PRIVATE wiz8_compile_settings)
wiz8_lint_target(wiz8_surrender_objects)

if(WIZ8_BUILD_PRODUCTS)
    add_library(SURRENDER SHARED
        $<TARGET_OBJECTS:wiz8_surrender_objects>
        sr.def
    )
    target_link_libraries(SURRENDER PRIVATE wiz8_compile_settings version.lib)
    target_link_options(SURRENDER PRIVATE
        /DEBUG /DEBUGTYPE:CV /INCREMENTAL:NO
        /OPT:NOREF /OPT:NOICF /BASE:0x10000000 /FILEALIGN:0x1000
        /OSVERSION:4.0 /SUBSYSTEM:WINDOWS,4.0
        # Comparison image, not a runnable replacement: extension.cpp retains its
        # unrecovered srConfig and logging dependencies. Never link clients here.
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
