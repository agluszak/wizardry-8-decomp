cmake_minimum_required(VERSION 3.20)

project(wizardry8_decomp LANGUAGES C CXX)

option(WIZ8_LINT_COMPILE_ONLY "Compile recovered C++ with modern diagnostics" OFF)
option(WIZ8_FULL_DIAGNOSTICS "Report recovery warnings without making them errors" OFF)

# Global policy only: one compile-settings interface, one lint registry, then
# the components. Component sources, includes, defines and products belong to
# the component directory, never here.
include(cmake/CompileSettings.cmake)
include(cmake/Lint.cmake)

if(WIZ8_LINT_COMPILE_ONLY)
    set(WIZ8_BUILD_PRODUCTS OFF)
else()
    if(NOT MSVC)
        message(FATAL_ERROR "The matching build requires the pinned MSVC 6 container")
    endif()
    set(WIZ8_BUILD_PRODUCTS ON)
    enable_language(RC)
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/GR-" CACHE STRING "" FORCE)
    set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
        "/DEBUG /INCREMENTAL:NO /BASE:0x10000000" CACHE STRING "" FORCE)
    # Final images stay at the build root even though components own sources.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
    include(cmake/ProductSupport.cmake)
    include(cmake/Reccmp.cmake)
endif()

add_subdirectory(src/sgp)
add_subdirectory(src/surrender)
add_subdirectory(src/srext_jpegimporter)
add_subdirectory(src/wiz8)
add_subdirectory(src/srext_unzip)

if(WIZ8_LINT_COMPILE_ONLY)
    wiz8_configure_lint_targets()
else()
    reccmp_configure()
endif()
