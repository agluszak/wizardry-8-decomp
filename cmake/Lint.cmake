# The clang-cl diagnostics lane over the canonical component targets.
# Apply diagnostics as each component declares its recovered/vendor targets.
if(WIZ8_ANALYSIS_BUILD)
    if(WIZ8_FULL_DIAGNOSTICS)
        set(WIZ8_LINT_UMBRELLA WIZ8_CLANG_DIAGNOSTICS)
    else()
        set(WIZ8_LINT_UMBRELLA WIZ8_CLANG_LINT)
    endif()
    add_custom_target(${WIZ8_LINT_UMBRELLA})
endif()

function(wiz8_lint_target target)
    if(NOT WIZ8_ANALYSIS_BUILD)
        return()
    endif()
    cmake_parse_arguments(ARG "VENDOR" "" "" ${ARGN})
    target_link_libraries(${target} PRIVATE wiz8_compile_settings)
    target_include_directories(${target} BEFORE PRIVATE "${PROJECT_SOURCE_DIR}/tools/lint/include")
    target_compile_definitions(${target} PRIVATE WIZ8_CLANG_LINT)
    if(WIZ8_ANALYSIS_X64)
        target_compile_options(${target} PRIVATE
            -Wpointer-to-int-cast
            -Wint-to-pointer-cast
            -Wshorten-64-to-32
        )
    endif()

    # Unavoidable Clang/VC6 driver and language-model compatibility. These are
    # not reconstruction diagnostics; recovered and vendor targets share them.
    # Layout offsetof on polymorphic recovered classes, C++98 implicit copies,
    # non-virtual destructors, unused incomplete recovery, Microsoft extensions,
    # and writable-string VC6 APIs stay here: "fixing" them invents source the
    # binary does not contain.
    target_compile_options(${target} PRIVATE
        /W4
        -Xclang -fno-wchar # clang-cl equivalent of VC6's /Zc:wchar_t- wchar model
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

    if(ARG_VENDOR)
        # WIZ8_SGP retains the released SFI source style in C++. It uses the same headers
        # and defines, but only the decompilation-correctness diagnostics gate it: its
        # released-source style warnings (pointer-sign, unused-but-set, incompatible pointer
        # types, ...) are vendor behavior and stay report-only in `wiz8 diagnostics`.
        # Signed comparisons are included in that vendor exception because the
        # reconstructed callers already fix their own side. The recovery warnings below
        # are report-only in the diagnostics lane and errors in the gating lane, so
        # `wiz8 diagnostics` never fails on what it is supposed to report. Clang 19
        # additionally promotes implicit declarations and mismatched callback pointers
        # to errors by default; those stay demoted to warnings here for the same
        # vendor reason instead of rewriting retained source.
        target_compile_options(${target} PRIVATE
            /UNOMINMAX
            -Wno-cast-function-type-mismatch
            -Wno-char-subscripts
            -Wno-mismatched-tags
            -Wno-tautological-compare
            -Wno-unknown-escape-sequence
            /MD /U_DEBUG
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
    else()
        # ABI-faithful char indices, callback mismatches and null-this tests use
        # function-local pragmas rather than global recovered-code suppressions.
        if(NOT WIZ8_FULL_DIAGNOSTICS)
            # Retail block-clears and block-copies some records holding vfptrs
            # (docs/retail-bugs.md); those diagnostics stay visible, uncast.
            target_compile_options(${target} PRIVATE -Werror
                -Wno-error=dynamic-class-memaccess -Wno-error=nontrivial-memcall)
        endif()
        # The decompilation-correctness diagnostics. The gating lane makes them
        # errors; the diagnostics lane reports them without failing. Suspicious
        # original behavior gets a local, evidence-backed suppression at its site.
        target_compile_options(${target} PRIVATE
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
    endif()
    add_dependencies(${WIZ8_LINT_UMBRELLA} ${target})
endfunction()
