# Single definition of the recovered/adapted JPEG importer compile model.
# Both the product SREXT_JPEGIMPORTER target and the clang-cl lint object
# consume this exact first-party source list and compile model. The pristine
# IJG tree keeps its upstream warnings and stays out of this model.
set(SREXT_JPEG_FIRST_PARTY_SOURCES
    src/srext_jpegimporter/codec_adapter.cpp
    src/srext_jpegimporter/stream_bridge.cpp
    src/srext_jpegimporter/plugin.cpp
    src/srext_jpegimporter/surface_transfer.cpp
)

function(srext_jpeg_configure_first_party_compile_model target)
    target_include_directories(${target} PRIVATE
        include
        src/srext_jpegimporter
        config/fid-overlays/ijg-jpeg-6
        "${IJG_JPEG_SOURCE}"
    )
    # The simple lifetime functions have no C++ EH prologue in the original;
    # only surface_transfer.cpp keeps the project default /GX. /GX is VC6
    # codegen only (Clang rejects it as unused), so guard it to the product
    # lane; both lanes still share the source list and includes above.
    set_source_files_properties(src/srext_jpegimporter/plugin.cpp PROPERTIES
        COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/GX->"
    )
endfunction()
