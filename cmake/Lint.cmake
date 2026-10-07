# The clang-cl diagnostics lane over the canonical component targets.
# Apply diagnostics as each component declares its recovered/vendor targets.
if(WIZ8_ANALYSIS_BUILD)
    add_custom_target(WIZ8_CLANG_LINT)
endif()

function(wiz8_lint_target target)
    if(NOT WIZ8_ANALYSIS_BUILD)
        return()
    endif()
    cmake_parse_arguments(ARG "VENDOR" "" "" ${ARGN})
    target_link_libraries(${target} PRIVATE wiz8_compile_settings)
    target_include_directories(${target} BEFORE PRIVATE "${PROJECT_SOURCE_DIR}/tools/lint/include")
    target_compile_definitions(${target} PRIVATE WIZ8_CLANG_LINT)

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
        # WIZ8_SGP retains the released SFI source style in C++: only the
        # decompilation-correctness diagnostics gate it. Its released-source style
        # warnings, signed comparisons, implicit declarations and mismatched callback
        # pointers are vendor behavior, not reconstruction errors.
        target_compile_options(${target} PRIVATE
            /UNOMINMAX
            -Wno-cast-function-type-mismatch
            -Wno-char-subscripts
            -Wno-mismatched-tags
            -Wno-tautological-compare
            -Wno-unknown-escape-sequence
            /MD /U_DEBUG
            -Wno-error=implicit-function-declaration
            -Wno-error=incompatible-function-pointer-types
            -Werror=sometimes-uninitialized
            -Werror=switch
            -Werror=array-bounds
            -Werror=missing-field-initializers
        )
    else()
        # ABI-faithful char indices, callback mismatches and null-this tests use
        # function-local pragmas rather than global recovered-code suppressions.
        # Retail block-clears and block-copies some records holding vfptrs
        # (docs/retail-bugs.md); those diagnostics stay visible, uncast.
        # Suspicious original behavior gets a local, evidence-backed suppression
        # at its site.
        target_compile_options(${target} PRIVATE -Werror
            -Wno-error=dynamic-class-memaccess -Wno-error=nontrivial-memcall
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
    add_dependencies(WIZ8_CLANG_LINT ${target})
endfunction()
