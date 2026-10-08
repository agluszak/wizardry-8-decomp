cmake_minimum_required(VERSION 3.20)

project(wizardry8_decomp LANGUAGES C CXX)

# Global policy only: shared compile settings and immediate lint configuration.
# Component sources, includes, defines and products belong to the component
# directory, never here.
include(cmake/CompileSettings.cmake)
include(cmake/Lint.cmake)

if(NOT WIZ8_ANALYSIS_BUILD)
    if(NOT MSVC)
        message(FATAL_ERROR "The matching build requires the pinned MSVC 6 container")
    endif()
    enable_language(RC)
    # CRT selection is explicit on the component targets, not a build-type default.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "")
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
add_subdirectory(tests/differential)

if(NOT WIZ8_ANALYSIS_BUILD)
    reccmp_configure(SURRENDER SREXT_JPEGIMPORTER WIZ8 SREXT_UNZIP)
endif()
