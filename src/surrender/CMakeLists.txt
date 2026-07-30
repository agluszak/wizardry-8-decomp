add_library(SURRENDER SHARED
    src/surrender/core.cpp
    src/surrender/dynamic_library.cpp
    src/surrender/sr.def
)
target_compile_definitions(SURRENDER PRIVATE SURRENDER_BUILD)
target_compile_options(SURRENDER PRIVATE /GX-)
target_link_libraries(SURRENDER PRIVATE wiz8_compile_settings version.lib)
target_link_options(SURRENDER PRIVATE
    /DEBUG /DEBUGTYPE:CV /INCREMENTAL:NO
    /OPT:NOREF /OPT:NOICF /BASE:0x10000000 /FILEALIGN:0x1000
    /OSVERSION:4.0 /SUBSYSTEM:WINDOWS,4.0
    "/MAP:${CMAKE_CURRENT_BINARY_DIR}/sr.map"
)
set_target_properties(SURRENDER PROPERTIES
    OUTPUT_NAME sr
    PREFIX ""
    PDB_NAME sr
)
