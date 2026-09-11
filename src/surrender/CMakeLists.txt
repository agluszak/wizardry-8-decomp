include("${CMAKE_CURRENT_SOURCE_DIR}/src/surrender/sources.cmake")
add_library(SURRENDER SHARED
    ${SURRENDER_SOURCES}
    src/surrender/sr.def
)
surrender_configure_compile_model(SURRENDER)
target_link_libraries(SURRENDER PRIVATE wiz8_compile_settings version.lib)
target_link_options(SURRENDER PRIVATE
    /DEBUG /DEBUGTYPE:CV /INCREMENTAL:NO
    /OPT:NOREF /OPT:NOICF /BASE:0x10000000 /FILEALIGN:0x1000
    /OSVERSION:4.0 /SUBSYSTEM:WINDOWS,4.0
    # Comparison image, not a runnable replacement: extension.cpp retains its
    # unrecovered srConfig and logging dependencies. Never link clients here.
    /FORCE:UNRESOLVED
    "/MAP:${CMAKE_CURRENT_BINARY_DIR}/sr.map"
)
set_target_properties(SURRENDER PROPERTIES
    OUTPUT_NAME sr
    PREFIX ""
    PDB_NAME sr
)
reccmp_add_target(SURRENDER ID SURRENDER)
