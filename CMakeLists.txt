cmake_minimum_required(VERSION 3.20)

project(wizardry8_decomp LANGUAGES C CXX)

option(WIZ8_LINT_COMPILE_ONLY "Compile recovered C++ with modern diagnostics" OFF)

if(WIZ8_LINT_COMPILE_ONLY)
    include(src/wiz8/sources.cmake)
    add_library(WIZ8_CLANG_LINT OBJECT
        ${WIZ8_ORIGINAL_UNITS}
        ${WIZ8_TEMPLATE_EMISSIONS}
        ${WIZ8_UNATTRIBUTED_UNITS}
        src/surrender/core.cpp
        src/wiz8/zlib_wrappers.c
    )
    target_include_directories(WIZ8_CLANG_LINT PRIVATE
        tools/lint/include
        include
        include/wiz8/sgp-compat
        third_party/sfi-sgp/sgp
        /zlib
    )
    target_compile_definitions(WIZ8_CLANG_LINT PRIVATE
        NOMINMAX WIN32_LEAN_AND_MEAN NDEBUG WIZ8_CLANG_LINT
    )
    target_compile_options(WIZ8_CLANG_LINT PRIVATE
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/compat/compiler.h"
        /W4
        -Xclang
        -fno-wchar
        -fms-extensions
        -fdiagnostics-parseable-fixits
        -Werror
        -Werror=inconsistent-missing-override
        -Werror=suggest-override
        -Werror=suggest-destructor-override
        -Werror=overloaded-virtual
        -Wno-unknown-pragmas
        -Wno-ignored-pragmas
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-private-field
        -Wno-undefined-inline
        -Wno-invalid-offsetof
        -Wno-array-bounds
        -Wno-char-subscripts
        -Wno-deprecated-register
        -Wno-delete-non-abstract-non-virtual-dtor
        -Wno-inconsistent-dllimport
        -Wno-missing-field-initializers
        -Wno-missing-braces
        -Wno-microsoft-cast
        -Wno-microsoft-enum-value
        -Wno-microsoft-exception-spec
        -Wno-microsoft-goto
        -Wno-microsoft-template
        -Wno-mismatched-tags
        -Wno-non-virtual-dtor
        -Wno-sign-compare
        -Wno-sometimes-uninitialized
        -Wno-switch
        -Wno-tautological-compare
        -Wno-unknown-escape-sequence
        -Wno-writable-strings
    )
    return()
endif()

if(NOT MSVC)
    message(FATAL_ERROR "The matching build requires the pinned MSVC 6 container")
endif()

enable_language(RC)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /MD /Z7 /DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /MD /GX /GR- /Z7 /DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
    "/DEBUG /INCREMENTAL:NO /BASE:0x10000000" CACHE STRING "" FORCE)

include(cmake/Product.cmake)
