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
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/srext_jpegimporter/ijg_stdio_override.h"
        COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/srext_jpegimporter/ijg_stdio_override.h"
    )
    # The simple lifetime functions (constructors, destructors, accessors) have
    # no C++ EH prologue in the original; getSurfaceDesc/importSurface/
    # exportSurface do have one (a synthesized SEH frame installed via
    # fs:[0]). They are split into surface_transfer.cpp, compiled at the
    # project default /GX, so plugin.cpp can keep /GX- for the rest.
    set_source_files_properties(src/srext_jpegimporter/plugin.cpp PROPERTIES
        COMPILE_OPTIONS "/GX-"
    )

    set(SR_IMPORT_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/sr.lib")
    add_custom_command(
        OUTPUT "${SR_IMPORT_LIBRARY}"
        COMMAND lib.exe /nologo /machine:ix86
            "/def:${CMAKE_CURRENT_SOURCE_DIR}/src/srext_jpegimporter/sr-jpeg-imports.def"
            "/out:${SR_IMPORT_LIBRARY}"
        DEPENDS src/srext_jpegimporter/sr-jpeg-imports.def
        VERBATIM
    )
    add_custom_target(surrender_import_library DEPENDS "${SR_IMPORT_LIBRARY}")

    # Source ordering is intentional: the original DLL links the Sir-Tech codec
    # adapter first, IJG release 6 next, and the SurRender plug-in code last.
    add_library(SREXT_JPEGIMPORTER SHARED
        src/srext_jpegimporter/codec_adapter.cpp
        ${IJG_JPEG_SOURCES}
        src/srext_jpegimporter/stream_bridge.cpp
        src/srext_jpegimporter/plugin.cpp
        src/srext_jpegimporter/surface_transfer.cpp
        src/srext_jpegimporter/srEXT_JPEGImporter.def
        src/srext_jpegimporter/srEXT_JPEGImporter.rc
    )
    wiz8_enable_cpp_compat(SREXT_JPEGIMPORTER)
    add_dependencies(SREXT_JPEGIMPORTER surrender_import_library)
    target_include_directories(SREXT_JPEGIMPORTER PRIVATE
        include
        src/srext_jpegimporter
        config/fid-overlays/ijg-jpeg-6
        "${IJG_JPEG_SOURCE}"
    )
    target_link_libraries(SREXT_JPEGIMPORTER PRIVATE "${SR_IMPORT_LIBRARY}")
    target_link_options(SREXT_JPEGIMPORTER PRIVATE
        /DEBUGTYPE:CV
        /OPT:NOREF
        /OPT:NOICF
        "/MAP:${CMAKE_CURRENT_BINARY_DIR}/srEXT_JPEGImporter.map"
    )
    set_target_properties(SREXT_JPEGIMPORTER PROPERTIES
        OUTPUT_NAME srEXT_JPEGImporter
        PREFIX ""
        PDB_NAME srEXT_JPEGImporter
    )
