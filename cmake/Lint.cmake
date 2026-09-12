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

# Unavoidable Clang/VC6 driver and language-model compatibility. These are
# not reconstruction diagnostics; recovered and vendor targets share them.
# Layout offsetof on polymorphic recovered classes, C++98 implicit copies,
# non-virtual destructors, unused incomplete recovery, Microsoft extensions,
# and writable-string VC6 APIs stay here: "fixing" them invents source the
# binary does not contain.
set(WIZ8_CLANG_COMPAT_FLAGS
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
    -Wno-non-virtual-dtor
    -Wno-visibility
    -Wno-writable-strings
)

# Recovered-code suppressions. Keep this list empty unless a diagnostic is
# proven both noisy and unusable as a reconstruction check. ABI-faithful
# `char` indices, verified callback-table mismatches, and VC6 null-this
# tests are function-local pragmas, not entries here.
set(WIZ8_RECOVERED_SUPPRESSIONS)

# Retained SGP/vendor C still produces these as upstream style, not as
# reconstruction defects. Recovered C++ enables the corresponding warnings
# in WIZ8_RECOVERY_WARNINGS instead.
set(WIZ8_VENDOR_SUPPRESSIONS
    -Wno-cast-function-type-mismatch
    -Wno-char-subscripts
    -Wno-mismatched-tags
    -Wno-tautological-compare
    -Wno-unknown-escape-sequence
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
    -Wcast-function-type-mismatch
    -Wtautological-compare
    -Wchar-subscripts
    -Wunknown-escape-sequence
    -Wpragma-pack
    $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
    $<$<COMPILE_LANGUAGE:CXX>:-Winconsistent-missing-override>
    $<$<COMPILE_LANGUAGE:CXX>:-Wshadow-field>
    $<$<COMPILE_LANGUAGE:CXX>:-Wmismatched-tags>
)

function(wiz8_configure_lint_target target)
    target_link_libraries(${target} PRIVATE wiz8_compile_settings)
    target_include_directories(${target} BEFORE PRIVATE tools/lint/include)
    target_compile_definitions(${target} PRIVATE WIZ8_CLANG_LINT)
    target_compile_options(${target} PRIVATE
        /W4 ${WIZ8_CLANG_COMPAT_FLAGS} ${WIZ8_RECOVERED_SUPPRESSIONS})
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
        /W4 ${WIZ8_CLANG_COMPAT_FLAGS} ${WIZ8_VENDOR_SUPPRESSIONS} /MD /U_DEBUG
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
