# The clang-cl diagnostics lane over the canonical component targets.
#
# Components define one object target per recovered source group and register
# it here. The product build ignores the registry; the lint configure applies
# modern diagnostics to the exact same sources, headers, defines and per-source
# properties, so the two graphs cannot drift into parallel source lists.

define_property(GLOBAL PROPERTY WIZ8_LINT_TARGETS
    BRIEF_DOCS "Recovered component targets the clang lane compiles"
    FULL_DOCS "Recovered component targets the clang lane compiles")

function(wiz8_lint_target target)
    cmake_parse_arguments(ARG "VENDOR" "" "" ${ARGN})
    if(ARG_VENDOR)
        set_property(TARGET ${target} PROPERTY WIZ8_LINT_PROFILE vendor)
    else()
        set_property(TARGET ${target} PROPERTY WIZ8_LINT_PROFILE recovered)
    endif()
    set_property(GLOBAL APPEND PROPERTY WIZ8_LINT_TARGETS ${target})
endfunction()

# clang-cl/VC6 compatibility noise from the reconstructed corpus. No
# decompilation-correctness diagnostic belongs in this list. Clang 19
# promotes several legacy idioms the corpus deliberately preserves to
# errors: the reviewed raw-callback casts, Info-ZIP K&R definitions, and
# VC6-era implicit copies. Recovered C++ still has a handful of those
# callback casts (region catalog, screen-lifecycle table, trigger
# activation, AnimObj tail-call, UnZip password/service), so the
# suppression stays shared rather than being restored for recovered
# targets only.
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
    $<$<COMPILE_LANGUAGE:CXX>:-Wshadow-field>
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

# WIZ8_SGP is the retained SFI C library. It compiles through the same headers
# and defines, but only the decompilation-correctness diagnostics gate it: its
# upstream C style warnings (pointer-sign, unused-but-set, incompatible pointer
# types, ...) are vendor behavior and stay report-only in `wiz8 diagnostics`.
# Signed comparisons are included in that vendor exception because the
# reconstructed callers already fix their own side. The recovery warnings below
# are report-only in the diagnostics lane and errors in the gating lane, so
# `wiz8 diagnostics` never fails on what it is supposed to report. Clang 19
# additionally promotes implicit declarations and mismatched callback pointers
# to errors by default; those stay demoted to warnings here for the same
# vendor reason instead of rewriting retained C.
function(wiz8_configure_vendor_lint_target target)
    target_include_directories(${target} BEFORE PRIVATE tools/lint/include)
    target_link_libraries(${target} PRIVATE wiz8_compile_settings)
    target_compile_definitions(${target} PRIVATE WIZ8_CLANG_LINT)
    target_compile_options(${target} PRIVATE
        /W4 ${WIZ8_LINT_COMPAT_FLAGS} /MD /U_DEBUG
        -Wsometimes-uninitialized
        -Wswitch
        -Warray-bounds
        -Wmissing-field-initializers
        -Wno-error=implicit-function-declaration
        -Wno-error=incompatible-function-pointer-types
    )
    if(NOT WIZ8_FULL_DIAGNOSTICS)
        target_compile_options(${target} PRIVATE
            -Werror=sometimes-uninitialized
            -Werror=switch
            -Werror=array-bounds
            -Werror=missing-field-initializers
        )
    endif()
endfunction()

function(wiz8_configure_lint_targets)
    get_property(targets GLOBAL PROPERTY WIZ8_LINT_TARGETS)
    if(NOT targets)
        message(FATAL_ERROR "no recovered targets registered for the clang lane")
    endif()
    foreach(target IN LISTS targets)
        get_property(profile TARGET ${target} PROPERTY WIZ8_LINT_PROFILE)
        if(profile STREQUAL "vendor")
            wiz8_configure_vendor_lint_target(${target})
        else()
            wiz8_configure_lint_target(${target})
        endif()
    endforeach()
    if(WIZ8_FULL_DIAGNOSTICS)
        set(umbrella WIZ8_CLANG_DIAGNOSTICS)
    else()
        set(umbrella WIZ8_CLANG_LINT)
    endif()
    add_custom_target(${umbrella} DEPENDS ${targets})
endfunction()
