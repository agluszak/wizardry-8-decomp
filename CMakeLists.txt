cmake_minimum_required(VERSION 3.20)

project(wizardry8_decomp LANGUAGES C CXX)

option(WIZ8_LINT_COMPILE_ONLY "Compile recovered C++ with modern diagnostics" OFF)
option(WIZ8_FULL_DIAGNOSTICS "Report recovery warnings without making them errors" OFF)

# Keep provider static-data definitions in provider mode in the lint/index TU too.
set_source_files_properties(src/surrender/extension.cpp src/surrender/type_registry.cpp src/surrender/vector_processor.cpp PROPERTIES
    COMPILE_DEFINITIONS SURRENDER_BUILD)

if(WIZ8_LINT_COMPILE_ONLY)
    include(src/wiz8/sources.cmake)
    if(WIZ8_FULL_DIAGNOSTICS)
        set(wiz8_clang_target WIZ8_CLANG_DIAGNOSTICS)
    else()
        set(wiz8_clang_target WIZ8_CLANG_LINT)
    endif()
    add_library(${wiz8_clang_target} OBJECT
        ${WIZ8_ORIGINAL_UNITS}
        ${WIZ8_TEMPLATE_EMISSIONS}
        ${WIZ8_PROVISIONAL_AND_QUARANTINE_UNITS}
        src/surrender/core.cpp
        src/surrender/dynamic_library.cpp
        src/surrender/extension.cpp
        src/surrender/string_table.cpp
        src/surrender/system.cpp
        src/surrender/type_registry.cpp
        src/surrender/vector_processor.cpp
    )
    target_include_directories(${wiz8_clang_target} PRIVATE
        tools/lint/include
        include
        include/wiz8/sgp-compat
        third_party/sfi-sgp/sgp
        /zlib
    )
    target_compile_definitions(${wiz8_clang_target} PRIVATE
        NOMINMAX WIN32_LEAN_AND_MEAN NDEBUG WIZ8_CLANG_LINT
    )
    target_compile_options(${wiz8_clang_target} PRIVATE
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/compat/compiler.h"
        /W4
        -Xclang
        -fno-wchar
        -fms-extensions
        -fdiagnostics-parseable-fixits
        -Wno-unknown-pragmas
        -Wno-ignored-pragmas
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-private-field
        -Wno-undefined-inline
        -Wno-invalid-offsetof
        -Wno-char-subscripts
        -Wno-deprecated-register
        -Wno-delete-non-abstract-non-virtual-dtor
        -Wno-inconsistent-dllimport
        -Wno-missing-braces
        -Wno-microsoft-cast
        -Wno-microsoft-enum-value
        -Wno-microsoft-exception-spec
        -Wno-microsoft-goto
        -Wno-microsoft-template
        -Wno-mismatched-tags
        -Wno-non-virtual-dtor
        -Wno-tautological-compare
        -Wno-unknown-escape-sequence
        -Wno-writable-strings
    )
    set(WIZ8_RECOVERY_WARNINGS
        -Wsometimes-uninitialized
        -Wswitch
        -Warray-bounds
        -Wsign-compare
        -Wmissing-field-initializers
    )
    target_compile_options(${wiz8_clang_target} PRIVATE ${WIZ8_RECOVERY_WARNINGS})
    if(NOT WIZ8_FULL_DIAGNOSTICS)
        # These remain report-only for the unrecovered corpus. Promote a unit
        # below once its warning debt has been reviewed; intentional original
        # behavior should then carry a local, evidence-backed suppression.
        target_compile_options(${wiz8_clang_target} PRIVATE
            -Werror
            -Werror=inconsistent-missing-override
            -Werror=suggest-override
            -Werror=suggest-destructor-override
            -Werror=overloaded-virtual
            -Wno-error=array-bounds
            -Wno-error=missing-field-initializers
            -Wno-error=sign-compare
            -Wno-error=sometimes-uninitialized
            -Wno-error=switch
        )
        set(WIZ8_STRICT_WARNING_UNITS
            src/wiz8/engine_code/BitArray.cpp
        )
        set(WIZ8_STRICT_RECOVERY_WARNINGS
            -Werror=sometimes-uninitialized
            -Werror=switch
            -Werror=array-bounds
            -Werror=sign-compare
            -Werror=missing-field-initializers
        )
        set_property(SOURCE ${WIZ8_STRICT_WARNING_UNITS} APPEND PROPERTY
            COMPILE_OPTIONS ${WIZ8_STRICT_RECOVERY_WARNINGS}
        )
    endif()
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

include(cmake/Reccmp.cmake)
include(cmake/Product.cmake)
reccmp_configure()
