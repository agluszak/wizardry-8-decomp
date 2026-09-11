cmake_minimum_required(VERSION 3.20)

project(wizardry8_decomp LANGUAGES C CXX)

option(WIZ8_LINT_COMPILE_ONLY "Compile recovered C++ with modern diagnostics" OFF)
option(WIZ8_FULL_DIAGNOSTICS "Report recovery warnings without making them errors" OFF)

include(cmake/CompileSettings.cmake)

if(WIZ8_LINT_COMPILE_ONLY)
    # One source-model projection per component: the same sources, headers,
    # defines and per-source properties as the product build, compiled by
    # clang-cl. Each component owns its source list and ABI-relevant compile
    # model once; both the product target and the lint object below consume
    # those exact variables/functions. WIZ8_CLANG_LINT is an umbrella over all
    # of them, so SGP and the recovered extension adaptors are linted too.
    add_subdirectory(src/sgp)
    include(src/wiz8/sources.cmake)
    include(src/surrender/sources.cmake)
    include(src/srext_jpegimporter/first_party_sources.cmake)
    include(src/srext_unzip/first_party_sources.cmake)

    # clang-cl/VC6 compatibility noise from the reconstructed corpus. No
    # decompilation-correctness diagnostic belongs in this list. Clang 19
    # promotes several legacy idioms the corpus deliberately preserves to
    # errors: the reviewed raw-callback casts, Info-ZIP K&R definitions, and
    # VC6-era implicit copies.
    set(WIZ8_LINT_COMPAT_FLAGS
        -Xclang -fno-wchar
        -fms-extensions
        -ferror-limit=0
        -fdiagnostics-parseable-fixits
        -Wno-unknown-pragmas
        -Wno-ignored-pragmas
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-private-field
        -Wno-undefined-inline
        -Wno-invalid-offsetof
        -Wno-cast-function-type-mismatch
        -Wno-char-subscripts
        -Wno-deprecated-copy
        -Wno-deprecated-non-prototype
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
        -Wno-visibility
        -Wno-writable-strings
    )
    # The decompilation-correctness diagnostics. The gating lane makes them
    # errors; the diagnostics lane reports them without failing. Suspicious
    # original behavior gets a local, evidence-backed suppression at its site.
    set(WIZ8_RECOVERY_WARNINGS
        -Wsometimes-uninitialized
        -Wswitch
        -Warray-bounds
        -Wsign-compare
        -Wmissing-field-initializers
        $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
        $<$<COMPILE_LANGUAGE:CXX>:-Winconsistent-missing-override>
    )

    function(wiz8_configure_lint_target target)
        target_link_libraries(${target} PRIVATE wiz8_compile_settings)
        target_include_directories(${target} BEFORE PRIVATE tools/lint/include)
        target_compile_definitions(${target} PRIVATE WIZ8_CLANG_LINT)
        target_compile_options(${target} PRIVATE /W4 ${WIZ8_LINT_COMPAT_FLAGS})
        if(NOT WIZ8_FULL_DIAGNOSTICS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
        target_compile_options(${target} PRIVATE ${WIZ8_RECOVERY_WARNINGS})
    endfunction()

    add_library(wiz8_clang_game OBJECT ${WIZ8_SOURCE_UNITS})
    wiz8_configure_lint_target(wiz8_clang_game)
    target_include_directories(wiz8_clang_game PRIVATE /zlib)

    add_library(wiz8_clang_surrender OBJECT ${SURRENDER_SOURCES})
    wiz8_configure_lint_target(wiz8_clang_surrender)
    surrender_configure_compile_model(wiz8_clang_surrender)

    # Only the recovered/adapted code around the vendored codecs; the pristine
    # IJG and Info-ZIP trees keep their upstream warnings.
    add_library(wiz8_clang_jpeg OBJECT ${SREXT_JPEG_FIRST_PARTY_SOURCES})
    wiz8_configure_lint_target(wiz8_clang_jpeg)
    srext_jpeg_configure_first_party_compile_model(wiz8_clang_jpeg)

    add_library(wiz8_clang_unzip OBJECT ${SREXT_UNZIP_FIRST_PARTY_SOURCES})
    wiz8_configure_lint_target(wiz8_clang_unzip)
    srext_unzip_configure_first_party_compile_model(wiz8_clang_unzip)

    # WIZ8_SGP is the retained SFI C library. It compiles through the same
    # headers and defines, but only the decompilation-correctness diagnostics
    # gate it: its upstream C style warnings (pointer-sign, unused-but-set,
    # incompatible pointer types, ...) are vendor behavior and stay report-only
    # in `wiz8 diagnostics`. Signed comparisons are included in that vendor
    # exception because the reconstructed callers already fix their own side.
    # Clang 19 additionally promotes implicit declarations and mismatched
    # callback pointers to errors by default; those stay demoted to warnings
    # here for the same vendor reason instead of rewriting retained C.
    target_include_directories(WIZ8_SGP BEFORE PRIVATE tools/lint/include)
    target_link_libraries(WIZ8_SGP PRIVATE wiz8_compile_settings)
    target_compile_definitions(WIZ8_SGP PRIVATE WIZ8_CLANG_LINT)
    target_compile_options(WIZ8_SGP PRIVATE
        /W4 ${WIZ8_LINT_COMPAT_FLAGS} /MD /U_DEBUG
        -Wsometimes-uninitialized
        -Wswitch
        -Warray-bounds
        -Wmissing-field-initializers
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-function-pointer-types
    )
    if(NOT WIZ8_FULL_DIAGNOSTICS)
        target_compile_options(WIZ8_SGP PRIVATE
            -Werror=sometimes-uninitialized
            -Werror=switch
            -Werror=array-bounds
            -Werror=missing-field-initializers
        )
    endif()

    if(WIZ8_FULL_DIAGNOSTICS)
        set(wiz8_clang_target WIZ8_CLANG_DIAGNOSTICS)
    else()
        set(wiz8_clang_target WIZ8_CLANG_LINT)
    endif()
    add_custom_target(${wiz8_clang_target}
        DEPENDS
            wiz8_clang_game
            wiz8_clang_surrender
            WIZ8_SGP
            wiz8_clang_jpeg
            wiz8_clang_unzip
    )
    return()
endif()

if(NOT MSVC)
    message(FATAL_ERROR "The matching build requires the pinned MSVC 6 container")
endif()

enable_language(RC)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/GR-" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
    "/DEBUG /INCREMENTAL:NO /BASE:0x10000000" CACHE STRING "" FORCE)

include(cmake/Reccmp.cmake)
include(cmake/Product.cmake)
reccmp_configure()
