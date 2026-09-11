# Single definition of the SurRender compile model. Both the product
# SURRENDER target and the clang-cl lint object consume this exact source
# list and compile model, so adding a unit or changing an ABI-relevant flag
# cannot drift one lane without the other.
set(SURRENDER_SOURCES
    src/surrender/core.cpp
    src/surrender/dynamic_library.cpp
    src/surrender/extension.cpp
    src/surrender/string_table.cpp
    src/surrender/system.cpp
    src/surrender/type_registry.cpp
    src/surrender/vector_processor.cpp
)

function(surrender_configure_compile_model target)
    target_compile_definitions(${target} PRIVATE SURRENDER_BUILD)
    # /GX selects VC6 exception prologues (codegen only). Clang's cl driver
    # rejects it as unused, and the lint lane never reproduces EH bytes, so
    # keep it in the VC6 product lane while both lanes share the define.
    target_compile_options(${target} PRIVATE
        $<$<C_COMPILER_ID:MSVC>:/GX->
        $<$<CXX_COMPILER_ID:MSVC>:/GX->
    )
    set_source_files_properties(
        src/surrender/extension.cpp
        src/surrender/type_registry.cpp
        PROPERTIES COMPILE_OPTIONS
            "$<$<CXX_COMPILER_ID:MSVC>:/GX>")
endfunction()
