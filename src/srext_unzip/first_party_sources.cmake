# Single definition of the recovered/adapted UnZip compile model. Both the
# product SREXT_UNZIP target and the clang-cl lint object consume this exact
# first-party source list and compile model, including the ABI/layout setting
# /Zp4. The pristine Info-ZIP tree keeps its upstream warnings and stays out
# of this model.
set(SREXT_UNZIP_FIRST_PARTY_SOURCES
    src/srext_unzip/api_subset.c
    src/srext_unzip/windll_subset.c
    src/srext_unzip/infozip_adapter.c
    src/srext_unzip/plugin.cpp
)

function(srext_unzip_configure_first_party_compile_model target)
    # /Zp4 is layout (accepted by clang-cl and required in both lanes).
    # /GX stays product-only: it selects VC6 EH prologues and Clang rejects
    # it as unused, so the product target adds it next to /W3 /O2.
    target_compile_options(${target} PRIVATE /Zp4)
    target_compile_definitions(${target} PRIVATE
        WIN32 _WINDOWS WINDLL USE_EF_UT_TIME DLL
    )
    target_include_directories(${target} PRIVATE
        include
        "${INFOZIP_SOURCE}"
        "${INFOZIP_SOURCE}/windll"
        "${INFOZIP_SOURCE}/win32"
    )
endfunction()
