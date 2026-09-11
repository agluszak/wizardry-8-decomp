set(IJG_JPEG_SOURCE "" CACHE PATH "Pinned IJG release 6 source tree")
if(NOT IJG_JPEG_SOURCE)
    message(FATAL_ERROR "IJG_JPEG_SOURCE must point at the pinned IJG release 6 tree")
endif()

# Product link order recovered from the original address map. In
# particular, the two memory-manager objects precede quantization and
# jutils; the discovery glob remains appropriate only for FID seeds.
set(IJG_JPEG_UNITS
    jcapimin.c
    jcapistd.c
    jccoefct.c
    jccolor.c
    jcdctmgr.c
    jchuff.c
    jcinit.c
    jcmainct.c
    jcmarker.c
    jcmaster.c
    jcomapi.c
    jcparam.c
    jcphuff.c
    jcprepct.c
    jcsample.c
    jctrans.c
    jdapimin.c
    jdapistd.c
    jdatadst.c
    jdatasrc.c
    jdcoefct.c
    jdcolor.c
    jddctmgr.c
    jdhuff.c
    jdinput.c
    jdmainct.c
    jdmarker.c
    jdmaster.c
    jdmerge.c
    jdphuff.c
    jdpostct.c
    jdsample.c
    jdtrans.c
    jerror.c
    jfdctflt.c
    jfdctfst.c
    jfdctint.c
    jidctflt.c
    jidctfst.c
    jidctint.c
    jidctred.c
    jmemmgr.c
    jmemnobs.c
    jquant1.c
    jquant2.c
    jutils.c
)
set(IJG_JPEG_SOURCES ${IJG_JPEG_UNITS})
list(TRANSFORM IJG_JPEG_SOURCES PREPEND "${IJG_JPEG_SOURCE}/")

# IJG repeats private helper names across translation units. Qualifying
# those static identifiers changes only COFF/PDB names, not machine code,
# and gives reccmp durable, unambiguous source ownership.
set_source_files_properties("${IJG_JPEG_SOURCE}/jccoefct.c" PROPERTIES
    COMPILE_DEFINITIONS
        "start_pass_coef=jccoefct_start_pass_coef;start_iMCU_row=jccoefct_start_iMCU_row;compress_output=jccoefct_compress_output")
set_source_files_properties("${IJG_JPEG_SOURCE}/jctrans.c" PROPERTIES
    COMPILE_DEFINITIONS
        "start_pass_coef=jctrans_start_pass_coef;start_iMCU_row=jctrans_start_iMCU_row;compress_output=jctrans_compress_output")
set_source_files_properties("${IJG_JPEG_SOURCE}/jccolor.c" PROPERTIES
    COMPILE_DEFINITIONS
        "grayscale_convert=jccolor_grayscale_convert;null_convert=jccolor_null_convert")
set_source_files_properties("${IJG_JPEG_SOURCE}/jdcolor.c" PROPERTIES
    COMPILE_DEFINITIONS
        "grayscale_convert=jdcolor_grayscale_convert;null_convert=jdcolor_null_convert;build_ycc_rgb_table=jdcolor_build_ycc_rgb_table")
set_source_files_properties("${IJG_JPEG_SOURCE}/jchuff.c" PROPERTIES
    COMPILE_DEFINITIONS
        "emit_bits=jchuff_emit_bits;dump_buffer=jchuff_dump_buffer;emit_restart=jchuff_emit_restart;flush_bits=jchuff_flush_bits")
set_source_files_properties("${IJG_JPEG_SOURCE}/jcphuff.c" PROPERTIES
    COMPILE_DEFINITIONS
        "emit_bits=jcphuff_emit_bits;dump_buffer=jcphuff_dump_buffer;emit_restart=jcphuff_emit_restart;flush_bits=jcphuff_flush_bits")
set_source_files_properties("${IJG_JPEG_SOURCE}/jcmainct.c" PROPERTIES
    COMPILE_DEFINITIONS
        "start_pass_main=jcmainct_start_pass_main;process_data_simple_main=jcmainct_process_data_simple_main")
set_source_files_properties("${IJG_JPEG_SOURCE}/jdmainct.c" PROPERTIES
    COMPILE_DEFINITIONS
        "start_pass_main=jdmainct_start_pass_main;process_data_simple_main=jdmainct_process_data_simple_main")
set_source_files_properties("${IJG_JPEG_SOURCE}/jcmaster.c" PROPERTIES
    COMPILE_DEFINITIONS
        "initial_setup=jcmaster_initial_setup;per_scan_setup=jcmaster_per_scan_setup")
set_source_files_properties("${IJG_JPEG_SOURCE}/jdinput.c" PROPERTIES
    COMPILE_DEFINITIONS
        "start_input_pass=jdinput_start_input_pass;per_scan_setup=jdinput_per_scan_setup;initial_setup=jdinput_initial_setup")
set_source_files_properties("${IJG_JPEG_SOURCE}/jdcoefct.c" PROPERTIES
    COMPILE_DEFINITIONS
        "start_input_pass=jdcoefct_start_input_pass;start_iMCU_row=jdcoefct_start_iMCU_row")
set_source_files_properties("${IJG_JPEG_SOURCE}/jdhuff.c" PROPERTIES
    COMPILE_DEFINITIONS "process_restart=jdhuff_process_restart")
set_source_files_properties("${IJG_JPEG_SOURCE}/jdphuff.c" PROPERTIES
    COMPILE_DEFINITIONS "process_restart=jdphuff_process_restart")
set_source_files_properties("${IJG_JPEG_SOURCE}/jdmerge.c" PROPERTIES
    COMPILE_DEFINITIONS "build_ycc_rgb_table=jdmerge_build_ycc_rgb_table")
set(IJG_STDIO_SOURCES
    "${IJG_JPEG_SOURCE}/jdatadst.c"
    "${IJG_JPEG_SOURCE}/jdatasrc.c"
)
set_source_files_properties(${IJG_STDIO_SOURCES} PROPERTIES
    OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ijg_stdio_override.h"
    COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/ijg_stdio_override.h"
)
# The simple lifetime functions (constructors, destructors, accessors) have
# no C++ EH prologue in the original; getSurfaceDesc/importSurface/
# exportSurface do have one (a synthesized SEH frame installed via
# fs:[0]). They are split into surface_transfer.cpp, compiled at the
# project default /GX, so plugin.cpp can keep /GX- for the rest.
set_source_files_properties(plugin.cpp PROPERTIES
    COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/GX->")

# Recovered adapter code contributes the first and last object groups; the
# pristine IJG translation units sit between them in original link order.
add_library(wiz8_jpeg_codec_objects OBJECT codec_adapter.cpp)
add_library(wiz8_jpeg_plugin_objects OBJECT
    stream_bridge.cpp
    plugin.cpp
    surface_transfer.cpp
)
foreach(target IN ITEMS wiz8_jpeg_codec_objects wiz8_jpeg_plugin_objects)
    wiz8_enable_cpp_compat(${target})
    target_include_directories(${target} PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/config/fid-overlays/ijg-jpeg-6"
        "${IJG_JPEG_SOURCE}"
    )
    target_link_libraries(${target} PRIVATE wiz8_compile_settings)
    wiz8_lint_target(${target})
endforeach()

if(WIZ8_BUILD_PRODUCTS)
    wiz8_add_import_library(
        SREXT_JPEG_SR
        sr-jpeg-imports.def
    )

    # VC6's LINK cannot consume the RC compiler's raw .res under this Wine
    # toolchain; convert it to a COFF object exactly as the UnZip component
    # does.
    find_program(CVTRES_EXECUTABLE cvtres.exe REQUIRED)
    set(SREXT_JPEG_RESOURCE_RES
        "${CMAKE_CURRENT_BINARY_DIR}/srEXT_JPEGImporter_resource.res")
    set(SREXT_JPEG_RESOURCE_OBJECT
        "${CMAKE_CURRENT_BINARY_DIR}/srEXT_JPEGImporter_resource.obj")
    add_custom_command(
        OUTPUT "${SREXT_JPEG_RESOURCE_OBJECT}"
        COMMAND "${CMAKE_RC_COMPILER}"
            "/fo${SREXT_JPEG_RESOURCE_RES}"
            "${CMAKE_CURRENT_SOURCE_DIR}/srEXT_JPEGImporter.rc"
        COMMAND "${CVTRES_EXECUTABLE}" /nologo /machine:ix86
            "/out:${SREXT_JPEG_RESOURCE_OBJECT}"
            "${SREXT_JPEG_RESOURCE_RES}"
        DEPENDS srEXT_JPEGImporter.rc
        VERBATIM
    )
    set_source_files_properties("${SREXT_JPEG_RESOURCE_OBJECT}" PROPERTIES
        EXTERNAL_OBJECT TRUE
        GENERATED TRUE
    )

    add_library(SREXT_JPEGIMPORTER SHARED
        $<TARGET_OBJECTS:wiz8_jpeg_codec_objects>
        ${IJG_JPEG_SOURCES}
        $<TARGET_OBJECTS:wiz8_jpeg_plugin_objects>
        srEXT_JPEGImporter.def
        "${SREXT_JPEG_RESOURCE_OBJECT}"
    )
    wiz8_enable_cpp_compat(SREXT_JPEGIMPORTER)
    add_dependencies(
        SREXT_JPEGIMPORTER
        ${SREXT_JPEG_SR_IMPORT_TARGET}
    )
    target_include_directories(SREXT_JPEGIMPORTER PRIVATE
        "${PROJECT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/config/fid-overlays/ijg-jpeg-6"
        "${IJG_JPEG_SOURCE}"
    )
    target_link_libraries(
        SREXT_JPEGIMPORTER
        PRIVATE
            wiz8_compile_settings
            "${SREXT_JPEG_SR_IMPORT_LIBRARY}"
    )
    target_link_options(SREXT_JPEGIMPORTER PRIVATE
        /DEBUGTYPE:CV
        /OPT:NOREF
        /OPT:NOICF
        "/MAP:${CMAKE_BINARY_DIR}/srEXT_JPEGImporter.map"
    )
    set_target_properties(SREXT_JPEGIMPORTER PROPERTIES
        OUTPUT_NAME srEXT_JPEGImporter
        PREFIX ""
        PDB_NAME srEXT_JPEGImporter
    )
    reccmp_add_target(SREXT_JPEGIMPORTER)
endif()
