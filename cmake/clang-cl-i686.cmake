# Modern diagnostics over the real VC6 headers. This toolchain is compile-only
# and never participates in matching or linked-image comparison.
set(CMAKE_SYSTEM_NAME Windows)
if(WIZ8_ANALYSIS_X64)
    set(CMAKE_SYSTEM_PROCESSOR AMD64)
    set(_target x86_64-pc-windows-msvc)
else()
    set(CMAKE_SYSTEM_PROCESSOR x86)
    set(_target i686-pc-windows-msvc)
endif()
set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_C_COMPILER_TARGET ${_target})
set(CMAKE_CXX_COMPILER_TARGET ${_target})
# LLVM archives the retained SGP objects without Microsoft's lib.exe.
find_program(CMAKE_AR NAMES llvm-lib-21 llvm-lib REQUIRED)
# Compiler probes stay compile-only; this lane does not link Windows images.
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

set(MSVC_INCLUDE_DIRS
    "/opt/msvc6-vc98-include"
    "/opt/msvc6-vc98-mfc-include"
    "/opt/msvc6-vc98-atl-include"
)
foreach(directory IN LISTS MSVC_INCLUDE_DIRS)
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:-imsvc${directory}>)
endforeach()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(WIZ8_ANALYSIS_BUILD ON CACHE INTERNAL "Compile only with modern diagnostics" FORCE)
