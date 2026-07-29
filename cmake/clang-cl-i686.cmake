# Modern diagnostics over the real VC6 headers. This toolchain is compile-only
# and never participates in matching or linked-image comparison.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)

set(_target i686-pc-windows-msvc)
set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_C_COMPILER_TARGET ${_target})
set(CMAKE_CXX_COMPILER_TARGET ${_target})
# The lint target emits object files only. Avoid CMake's static-library probe,
# which requires Microsoft's lib.exe even though the real build never links.
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
set(WIZ8_LINT_COMPILE_ONLY ON CACHE BOOL "Compile only with modern diagnostics")
