cmake_minimum_required(VERSION 3.20)

project(wizardry8_decomp LANGUAGES C CXX)

option(WIZ8_BUILD_DECOMP "Configure recovered Wizardry binaries" ON)
option(WIZ8_BUILD_FID_SEEDS "Configure source-built FID seed objects" OFF)
option(WIZ8_BUILD_TOOLCHAIN_PROBE "Configure the VC6 Rich-header probe" OFF)
option(WIZ8_BUILD_SGP_PROBES "Configure released SGP source-oracle units" OFF)

set(FID_ZLIB_SOURCE "" CACHE PATH "Pinned zlib source tree used for FID seeds")
set(ZLIB_SOURCE "" CACHE PATH "Pinned zlib 1.0.4 source tree used by recovered game code")
set(FID_JPEG_SOURCE "" CACHE PATH "Pinned IJG JPEG source tree used for FID seeds")
set(FID_INFOZIP_SOURCE "" CACHE PATH "Pinned Info-ZIP source tree used for FID seeds")
set(INFOZIP_SOURCE "" CACHE PATH "Pinned Info-ZIP UnZip 5.4 source tree")
set(SGP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sfi-sgp/sgp" CACHE PATH
    "Vendored SFI-licensed Standard Gaming Platform source tree")
set(RECCMP_PROJECT_DIR_HOST "" CACHE PATH "Host path to the reccmp project")
set(RECCMP_BUILD_DIR_HOST "" CACHE PATH "Host path to this build directory")

if(NOT MSVC)
    message(FATAL_ERROR "The matching build requires the pinned MSVC 6 container")
endif()
enable_language(RC)
if(WIZ8_BUILD_DECOMP AND NOT IJG_JPEG_SOURCE)
    message(FATAL_ERROR "IJG_JPEG_SOURCE must point at the pinned IJG release 6 tree")
endif()

set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /MD /Z7 /DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /MD /GX /GR- /Z7 /DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
    "/DEBUG /INCREMENTAL:NO /BASE:0x10000000"
    CACHE STRING "" FORCE)

function(wiz8_collect_ijg_units RESULT SOURCE_ROOT)
    file(GLOB units CONFIGURE_DEPENDS RELATIVE "${SOURCE_ROOT}" "${SOURCE_ROOT}/j*.c")
    list(REMOVE_ITEM units jpegtran.c jmemansi.c jmemdos.c jmemname.c)
    list(SORT units)
    # The original link places the selected memory manager after the codec
    # units; preserve that matching-relevant order after glob discovery.
    list(REMOVE_ITEM units jmemmgr.c jmemnobs.c)
    list(APPEND units jmemmgr.c jmemnobs.c)
    set(${RESULT} ${units} PARENT_SCOPE)
endfunction()

function(wiz8_collect_zlib_units RESULT SOURCE_ROOT)
    file(GLOB units CONFIGURE_DEPENDS RELATIVE "${SOURCE_ROOT}" "${SOURCE_ROOT}/*.c")
    list(REMOVE_ITEM units example.c minigzip.c)
    list(SORT units)
    set(${RESULT} ${units} PARENT_SCOPE)
endfunction()

function(wiz8_collect_infozip_units RESULT SOURCE_ROOT)
    file(GLOB units CONFIGURE_DEPENDS RELATIVE "${SOURCE_ROOT}"
        "${SOURCE_ROOT}/*.c"
        "${SOURCE_ROOT}/win32/*.c"
        "${SOURCE_ROOT}/windll/*.c"
    )
    list(REMOVE_ITEM units
        apihelp.c envargs.c funzip.c unzip.c unzipstb.c
        win32/crc_i386.c windll/example.c
    )
    list(SORT units)
    set(${RESULT} ${units} PARENT_SCOPE)
endfunction()

if(WIZ8_BUILD_DECOMP)
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

    if(RECCMP_PROJECT_DIR_HOST AND RECCMP_BUILD_DIR_HOST)
        file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/reccmp-build.yml" CONTENT
            "project: '${RECCMP_PROJECT_DIR_HOST}'\ntargets:\n  WIZ8:\n    path: '${RECCMP_BUILD_DIR_HOST}/Wiz8.exe'\n    pdb: '${RECCMP_BUILD_DIR_HOST}/Wiz8.pdb'\n  SREXT_JPEGIMPORTER:\n    path: '${RECCMP_BUILD_DIR_HOST}/srEXT_JPEGImporter.dll'\n    pdb: '${RECCMP_BUILD_DIR_HOST}/srEXT_JPEGImporter.pdb'\n  SREXT_UNZIP:\n    path: '${RECCMP_BUILD_DIR_HOST}/srEXT_Unzip.dll'\n    pdb: '${RECCMP_BUILD_DIR_HOST}/srEXT_Unzip.pdb'\n"
        )
    endif()
endif()

if(WIZ8_BUILD_DECOMP AND ZLIB_SOURCE)
    # These five functions are Wizardry-owned adapters around pristine zlib
    # 1.0.4, not a second copy of the library implementation.
    add_library(WIZ8_ZLIB_WRAPPERS OBJECT
        src/wiz8/zlib_wrappers.c
    )
    target_include_directories(WIZ8_ZLIB_WRAPPERS PRIVATE "${ZLIB_SOURCE}")

    # The canonical executable statically links pristine zlib 1.0.4; 46 of its
    # bodies are already reviewed in evidence/reviewed/wiz8/functions.csv.
    # Build the pinned source as a real library so bring-up resolves inflate*
    # by linking it, rather than by stubbing library code.
    wiz8_collect_zlib_units(WIZ8_ZLIB_UNITS "${ZLIB_SOURCE}")
    list(TRANSFORM WIZ8_ZLIB_UNITS PREPEND "${ZLIB_SOURCE}/")
    add_library(WIZ8_ZLIB_1_0_4 STATIC EXCLUDE_FROM_ALL ${WIZ8_ZLIB_UNITS})
    target_compile_options(WIZ8_ZLIB_1_0_4 PRIVATE /nologo /O2 /MD)
    target_compile_definitions(WIZ8_ZLIB_1_0_4 PRIVATE NDEBUG WIN32)
    target_include_directories(WIZ8_ZLIB_1_0_4 PRIVATE "${ZLIB_SOURCE}")
endif()

if(WIZ8_BUILD_DECOMP)
    add_library(WIZ8_GAMEPLAY_BOUNDARIES OBJECT
        src/wiz8/bringup_gates.cpp
        src/wiz8/renderer_window.cpp
        src/wiz8/window_proc.cpp
        "src/wiz8/local_code/PC Item.cpp"
        src/wiz8/character_skills.cpp
        src/wiz8/local_code/Controls.cpp
        src/wiz8/gameplay_boundaries.cpp
        src/wiz8/local_code/Factions.cpp
        src/wiz8/local_code/LoadSaveGame.cpp
        src/wiz8/local_code/Magic.cpp
        src/wiz8/local_code/VideoObjectManager.cpp
        "src/wiz8/local_screens/MainMenuScreen.cpp"
        src/wiz8/local_screens/Screens.cpp
        src/wiz8/location_variables.cpp
        src/wiz8/local_code/MonsterManager.cpp
        src/wiz8/local_code/UtilityFunctions.cpp
        src/wiz8/dialog_code/Dialog005A80A0.cpp
        src/wiz8/dialog_code/DialogBase005D25B0.cpp
        src/wiz8/dialog_code/Object005EF894.cpp
        src/wiz8/dialog_code/StatInfoDialogs.cpp
        src/wiz8/engine_code/Object0043A910.cpp
        src/wiz8/engine_code/Object005EBCFC.cpp
        src/wiz8/engine_code/Prop005EC1E0.cpp
        src/wiz8/engine_code/GrObject.cpp
        src/wiz8/vector_005ebfb4.cpp
        src/wiz8/vector_005ec16c.cpp
        src/wiz8/vector_005ec2b8.cpp
        src/wiz8/vector_005ec324.cpp
        src/wiz8/vector_005ec514.cpp
        src/wiz8/vector_005eca98.cpp
        src/wiz8/vector_005ecaa0.cpp
        src/wiz8/vector_005ecad0.cpp
        src/wiz8/vector_005ece60.cpp
        src/wiz8/vector_005ecee4.cpp
        src/wiz8/vector_005ed018.cpp
        src/wiz8/vector_005ed2c8.cpp
        src/wiz8/vector_005ed7d4.cpp
        src/wiz8/vector_005ef08c.cpp
        src/wiz8/vector_005ec018.cpp
        src/wiz8/vector_005ed1b8.cpp
        src/wiz8/vector_005ec164.cpp
        src/wiz8/vector_005eca5c.cpp
        src/wiz8/vector_005ec294.cpp
        src/wiz8/vector_005ed840.cpp
        src/wiz8/vector_005eea28.cpp
        src/wiz8/vector_005ecf00.cpp
        src/wiz8/vector_005ec27c.cpp
        src/wiz8/local_code/Control004F5450.cpp
        src/wiz8/local_code/RegionManager.cpp
        src/wiz8/local_code/LoadSaveGame.cpp
        src/wiz8/local_code/chunk.cpp
        src/wiz8/grcycle.cpp
        src/wiz8/engine_code/GrCycle.cpp
        src/wiz8/fact_state.cpp
        src/wiz8/local_code/GameplayDatabase.cpp
        src/wiz8/dialog_code/DialogInterface.cpp
        src/wiz8/dirty_tiles.cpp
        src/wiz8/render_options.cpp
        src/wiz8/unattributed_helpers.cpp
        src/wiz8/frame_tick.cpp
        src/wiz8/game_init.cpp
        src/wiz8/gameplay_teardown.cpp
        src/wiz8/3d_code/IList.cpp
        src/wiz8/local_code/ItemManager.cpp
        src/wiz8/item_mesh.cpp
        src/wiz8/item_spawning.cpp
        src/wiz8/message_box.cpp
        src/wiz8/engine_code/game_timer.cpp
        src/wiz8/engine_code/materials.cpp
        src/wiz8/engine_code/Monster.cpp
        src/wiz8/monster_info_dialog.cpp
        src/wiz8/monster_generators.cpp
        src/wiz8/monster_lookup.cpp
        src/wiz8/npc_item_lists.cpp
        src/wiz8/spell_backfire.cpp
        src/wiz8/local_code/DisplayList.cpp
        src/wiz8/local_code/SurfaceFill.cpp
        src/wiz8/local_code/Targeting.cpp
        src/wiz8/local_code/Viewport.cpp
        src/wiz8/state_getters.cpp
        src/wiz8/vector_conversions.cpp
        src/wiz8/virtual_file_stream.cpp
        src/wiz8/engine_code/Octree.cpp
        src/wiz8/3d_code/PList.cpp
        src/wiz8/engine_code/3d.cpp
        src/wiz8/vc6_runtime.cpp
    )
    target_compile_options(WIZ8_GAMEPLAY_BOUNDARIES PRIVATE /nologo /O2 /G6 /MD)
    target_compile_definitions(WIZ8_GAMEPLAY_BOUNDARIES PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    # The vendored SGP headers are on the path so recovered bodies that call
    # into SGP declare its types from the real header rather than restating
    # them. GETFILESTRUCT is the first: LoadSaveGame.cpp walks the save
    # directory through GetFileFirst.
    target_include_directories(WIZ8_GAMEPLAY_BOUNDARIES PRIVATE
        include
        config/sgp-overlays/common
        "${SGP_SOURCE}")

    # Matching source stays free of synthetic implementations. This umbrella
    # preserves the old object-target name while giving the executable work a
    # target whose contract is explicitly source matching.
    add_custom_target(WIZ8_MATCHING DEPENDS WIZ8_GAMEPLAY_BOUNDARIES)
    if(TARGET WIZ8_ZLIB_WRAPPERS)
        add_dependencies(WIZ8_MATCHING WIZ8_ZLIB_WRAPPERS)
    endif()

    set(WIZ8_SR_IMPORT_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/wiz8-sr.lib")
    add_custom_command(
        OUTPUT "${WIZ8_SR_IMPORT_LIBRARY}"
        COMMAND lib.exe /nologo /machine:ix86
            "/def:${CMAKE_CURRENT_SOURCE_DIR}/src/wiz8/imports/sr.def"
            "/out:${WIZ8_SR_IMPORT_LIBRARY}"
        DEPENDS src/wiz8/imports/sr.def
        VERBATIM
    )
    add_custom_target(wiz8_sr_import_library DEPENDS "${WIZ8_SR_IMPORT_LIBRARY}")

    # Miles Sound System. The canonical executable imports 56 MSS32 entry points;
    # the audio bring-up gate at 0x004086D0 calls six of them, so the import
    # library is now built rather than deferred. Only an import library is
    # produced - Miles itself is library code and is never modelled.
    set(WIZ8_MSS32_IMPORT_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/wiz8-mss32.lib")
    add_custom_command(
        OUTPUT "${WIZ8_MSS32_IMPORT_LIBRARY}"
        COMMAND lib.exe /nologo /machine:ix86
            "/def:${CMAKE_CURRENT_SOURCE_DIR}/src/wiz8/imports/mss32.def"
            "/out:${WIZ8_MSS32_IMPORT_LIBRARY}"
        DEPENDS src/wiz8/imports/mss32.def
        VERBATIM
    )
    add_custom_target(wiz8_mss32_import_library DEPENDS "${WIZ8_MSS32_IMPORT_LIBRARY}")

    # Bring-up is intentionally a separate link surface. The recovered source
    # still has unresolved first-party globals and callees, so /FORCE:UNRESOLVED
    # lets LINK produce an inspectable PE/PDB without adding fake bodies to the
    # matching objects. The bring-up WinMain only proves the platform/CRT link
    # and exits; it is not an address claim.
    add_executable(WIZ8_BRINGUP WIN32
        $<TARGET_OBJECTS:WIZ8_GAMEPLAY_BOUNDARIES>
    )
    if(TARGET WIZ8_ZLIB_WRAPPERS)
        target_sources(WIZ8_BRINGUP PRIVATE $<TARGET_OBJECTS:WIZ8_ZLIB_WRAPPERS>)
    endif()
    add_dependencies(WIZ8_BRINGUP wiz8_sr_import_library wiz8_mss32_import_library)
    target_compile_options(WIZ8_BRINGUP PRIVATE /nologo /O2 /G6 /MD)
    target_compile_definitions(WIZ8_BRINGUP PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_include_directories(WIZ8_BRINGUP PRIVATE include)
    # KERNEL32 and the dynamic MSVCRT/MSVCP60 runtimes are selected by the VC6
    # compiler defaults and /MD. These are the additional Windows import libs
    # actually present in the canonical executable; do not copy Imperialism's
    # winmm/vfw/DirectSound/DirectPlay list because Wiz8.exe does not import it.
    # ddraw/gdi32/user32 are the Windows import libs the canonical executable
    # actually imports beyond the VC6 defaults (KERNEL32 and, via /MD, MSVCRT
    # and MSVCP60); do not copy Imperialism's winmm/vfw/DirectSound/DirectPlay
    # list because Wiz8.exe does not import it. MSS32 (Miles) and BINKW32 (Bink)
    # BINKW32 (Bink) is imported by the canonical executable too, but no
    # recovered source references it yet, so it stays unlinked until something
    # needs it. MSS32 (Miles) is now linked: the audio bring-up gate calls it. zlib is linked as a real library so inflate* resolves from
    # pinned upstream source instead of a stub.
    target_link_libraries(WIZ8_BRINGUP PRIVATE
        ddraw.lib
        gdi32.lib
        user32.lib
        "${WIZ8_SR_IMPORT_LIBRARY}"
        "${WIZ8_MSS32_IMPORT_LIBRARY}"
    )
    if(TARGET WIZ8_ZLIB_1_0_4)
        target_link_libraries(WIZ8_BRINGUP PRIVATE WIZ8_ZLIB_1_0_4)
    endif()
    target_link_options(WIZ8_BRINGUP PRIVATE
        /DEBUG
        /DEBUGTYPE:CV
        /INCREMENTAL:NO
        /OPT:NOREF
        /OPT:NOICF
        /FORCE:UNRESOLVED
        /BASE:0x400000
        /FILEALIGN:0x1000
        /OSVERSION:4.0
        /SUBSYSTEM:WINDOWS,4.0
        /STACK:0x100000,0x1000
        /HEAP:0x100000,0x1000
        "/MAP:${CMAKE_CURRENT_BINARY_DIR}/Wiz8.map"
    )
    set_target_properties(WIZ8_BRINGUP PROPERTIES
        OUTPUT_NAME Wiz8
        PREFIX ""
        PDB_NAME Wiz8
    )
    add_custom_target(WIZ8 DEPENDS WIZ8_BRINGUP)
endif()

if((WIZ8_BUILD_DECOMP OR WIZ8_BUILD_SGP_PROBES) AND SGP_SOURCE)
    # One target per released SGP translation unit. Each unit gets only the
    # WIZ8 SGP ALL.H surface its own source proves necessary, plus the shared
    # empty builddefines.h overlay. The common release profile is the primary
    # hypothesis: the released VC6 SGP project stores release flags at project
    # scope, and none of the probed units has a release per-file override.
    set(WIZ8_SGP_PROJECT_FLAGS /O2 /Ob2 /G5 /MD)
    function(wiz8_add_sgp_unit target source overlay)
        add_library(${target} OBJECT EXCLUDE_FROM_ALL "${SGP_SOURCE}/${source}")
        target_compile_definitions(${target} PRIVATE WIZ8_PRECOMPILED_HEADERS)
        target_include_directories(${target} PRIVATE
            "config/sgp-overlays/${overlay}"
            config/sgp-overlays/common
            "${SGP_SOURCE}"
        )
        target_compile_options(${target} PRIVATE ${WIZ8_SGP_PROJECT_FLAGS})
        if(ARGN)
            target_compile_options(${target} PRIVATE ${ARGN})
        endif()
    endfunction()

    wiz8_add_sgp_unit(WIZ8_SGP_DIRECTDRAW "DirectDraw Calls.c" directdraw)

    # Random.c proves two axes of the common profile: /Ob2 inlines Random into
    # Chance, while /G6 mismatches both arithmetic functions.
    wiz8_add_sgp_unit(WIZ8_SGP_RANDOM "Random.c" random)

    # Compression.c brackets the linked zlib objects. The whole released unit
    # is retained in the probe even though Wizardry only kept its decompression
    # half after COMDAT elimination.
    wiz8_add_sgp_unit(WIZ8_SGP_COMPRESSION "Compression.c" compression)

    wiz8_add_sgp_unit(WIZ8_SGP_FILEMAN "FileMan.c" fileman)

    # Wizardry's six embedded SLF paths recover the missing WizLibs.h library
    # count used by LibraryDataBase.c. DbMan.c has no product-private surface.
    wiz8_add_sgp_unit(WIZ8_SGP_LIBRARY_DATABASE "LibraryDataBase.c" librarydatabase)
    wiz8_add_sgp_unit(WIZ8_SGP_DBMAN "DbMan.c" dbman)

    # Container.c is the released SGP contiguous-container implementation
    # (HSTACK/HQUEUE/HLIST/HORDLIST), not Wizardry's first-party PList class.
    wiz8_add_sgp_unit(WIZ8_SGP_CONTAINER "Container.c" container)

    # DEBUG.C owns the shared release-mode formatter and debug-manager
    # boundaries. ExceptionHandling.cpp deliberately emits no code for the
    # Wizardry branch because ExceptionHandling.h leaves its implementation
    # feature disabled outside JA2.
    # The original Windows project treats the case-insensitive .C suffix as
    # C. CMake otherwise interprets uppercase .C as C++ on this host.
    set_source_files_properties("${SGP_SOURCE}/DEBUG.C" PROPERTIES LANGUAGE C)
    wiz8_add_sgp_unit(WIZ8_SGP_DEBUG "DEBUG.C" debug)
    wiz8_add_sgp_unit(
        WIZ8_SGP_EXCEPTION_HANDLING
        "ExceptionHandling.cpp"
        exceptionhandling
    )

    # Startup-spine evidence probes. The core and input objects contain released
    # APIs that Wizardry discarded and therefore are not linked wholesale into
    # the /OPT:NOREF bring-up. Random.c and timer.c are complete retained units;
    # every other configured unit remains an explicit, on-demand source oracle.
    wiz8_add_sgp_unit(WIZ8_SGP_CORE "sgp.c" sgp)
    wiz8_add_sgp_unit(WIZ8_SGP_TIMER "timer.c" timer)
    wiz8_add_sgp_unit(WIZ8_SGP_INPUT "input.c" input)
    if(WIZ8_BUILD_DECOMP)
        # Container, FileMan and input are built but deliberately not linked.
        # Only a unit the vendored source reproduces whole may join the bring-up
        # image, and these three are partial against their shipped counterparts.
        # Building them is what puts their individual bodies under
        # relocation-masked verification against the original; linking them
        # would be a claim about the image that the evidence does not support.
        add_dependencies(
            WIZ8_MATCHING
            WIZ8_SGP_RANDOM
            WIZ8_SGP_TIMER
        )
        # Compiling these is an evidence operation, not executable bring-up:
        # they are verified against the original but deliberately never linked,
        # because only a unit the vendored source reproduces whole may join the
        # image. They therefore hang off their own aggregate rather than off the
        # bring-up target, whose dependencies should be its real link inputs.
        add_custom_target(WIZ8_SGP_PROBES)
        add_dependencies(
            WIZ8_SGP_PROBES
            WIZ8_SGP_CONTAINER
            WIZ8_SGP_CORE
            WIZ8_SGP_DEBUG
            WIZ8_SGP_DIRECTDRAW
            WIZ8_SGP_FILEMAN
            WIZ8_SGP_INPUT
            WIZ8_SGP_LIBRARY_DATABASE
        )
        add_dependencies(WIZ8_MATCHING WIZ8_SGP_PROBES)
        # The image target pulls them too, so an ordinary build still produces
        # the objects verification reads. Verification does not build.
        add_dependencies(WIZ8 WIZ8_SGP_PROBES)
        target_sources(WIZ8_BRINGUP PRIVATE
            $<TARGET_OBJECTS:WIZ8_SGP_RANDOM>
            $<TARGET_OBJECTS:WIZ8_SGP_TIMER>
        )
    endif()
endif()

if(WIZ8_BUILD_DECOMP AND INFOZIP_SOURCE)
    # This is the exact VC6 static-library source set from the release-5.4
    # windll/visualc/lib project. Sir-Tech links these upstream objects into
    # srEXT_Unzip.dll and adds a narrow SurRender adapter afterward.
    set(INFOZIP_UNZIP_SOURCES
        api.c
        crc32.c
        crctab.c
        crypt.c
        explode.c
        extract.c
        fileio.c
        globals.c
        inflate.c
        list.c
        match.c
        win32/nt.c
        process.c
        ttyio.c
        unreduce.c
        unshrink.c
        win32/win32.c
        windll/windll.c
        zipinfo.c
    )
    list(TRANSFORM INFOZIP_UNZIP_SOURCES PREPEND "${INFOZIP_SOURCE}/")
    add_library(INFOZIP_UNZIP_5_4 STATIC EXCLUDE_FROM_ALL ${INFOZIP_UNZIP_SOURCES})
    target_compile_options(INFOZIP_UNZIP_5_4 PRIVATE /nologo /W3 /GX /O2 /Zp4)
    target_compile_definitions(INFOZIP_UNZIP_5_4 PRIVATE
        NDEBUG WIN32 _WINDOWS WINDLL USE_EF_UT_TIME DLL UNZIPLIB
    )
    target_include_directories(INFOZIP_UNZIP_5_4 PRIVATE
        "${INFOZIP_SOURCE}"
        "${INFOZIP_SOURCE}/windll"
        "${INFOZIP_SOURCE}/win32"
    )

    add_library(SREXT_UNZIP_INFOZIP_ADAPTER OBJECT EXCLUDE_FROM_ALL
        src/srext_unzip/infozip_adapter.c
        src/srext_unzip/plugin.cpp
    )
    target_compile_options(SREXT_UNZIP_INFOZIP_ADAPTER PRIVATE
        /nologo /W3 /GX /O2 /Zp4
    )
    target_compile_definitions(SREXT_UNZIP_INFOZIP_ADAPTER PRIVATE
        NDEBUG WIN32 _WINDOWS WINDLL USE_EF_UT_TIME DLL UNZIPLIB
    )
    target_include_directories(SREXT_UNZIP_INFOZIP_ADAPTER PRIVATE
        include
        "${INFOZIP_SOURCE}"
        "${INFOZIP_SOURCE}/windll"
        "${INFOZIP_SOURCE}/win32"
    )
    add_dependencies(INFOZIP_UNZIP_5_4 SREXT_UNZIP_INFOZIP_ADAPTER)

    set(SR_UNZIP_IMPORT_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/sr-unzip.lib")
    add_custom_command(
        OUTPUT "${SR_UNZIP_IMPORT_LIBRARY}"
        COMMAND lib.exe /nologo /machine:ix86
            "/def:${CMAKE_CURRENT_SOURCE_DIR}/src/srext_unzip/sr-unzip-imports.def"
            "/out:${SR_UNZIP_IMPORT_LIBRARY}"
        DEPENDS src/srext_unzip/sr-unzip-imports.def
        VERBATIM
    )
    add_custom_target(surrender_unzip_import_library
        DEPENDS "${SR_UNZIP_IMPORT_LIBRARY}")

    find_program(CVTRES_EXECUTABLE cvtres.exe REQUIRED)
    set(SREXT_UNZIP_RESOURCE_RES
        "${CMAKE_CURRENT_BINARY_DIR}/srEXT_Unzip_resource.res")
    set(SREXT_UNZIP_RESOURCE_OBJECT
        "${CMAKE_CURRENT_BINARY_DIR}/srEXT_Unzip_resource.obj")
    add_custom_command(
        OUTPUT "${SREXT_UNZIP_RESOURCE_OBJECT}"
        COMMAND "${CMAKE_RC_COMPILER}"
            "/fo${SREXT_UNZIP_RESOURCE_RES}"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/srext_unzip/srEXT_Unzip.rc"
        COMMAND "${CVTRES_EXECUTABLE}" /nologo /machine:ix86
            "/out:${SREXT_UNZIP_RESOURCE_OBJECT}"
            "${SREXT_UNZIP_RESOURCE_RES}"
        DEPENDS src/srext_unzip/srEXT_Unzip.rc
        VERBATIM
    )
    set_source_files_properties("${SREXT_UNZIP_RESOURCE_OBJECT}" PROPERTIES
        EXTERNAL_OBJECT TRUE
        GENERATED TRUE
    )

    set(SREXT_UNZIP_INFOZIP_SOURCES ${INFOZIP_UNZIP_SOURCES})
    list(REMOVE_ITEM SREXT_UNZIP_INFOZIP_SOURCES
        "${INFOZIP_SOURCE}/api.c"
        "${INFOZIP_SOURCE}/windll/windll.c")

    # The original image places the Info-ZIP object sequence before Sir-Tech's
    # narrowed WinDLL front end, memory wrapper, and SurRender plug-in object.
    add_library(SREXT_UNZIP SHARED
        src/srext_unzip/api_subset.c
        ${SREXT_UNZIP_INFOZIP_SOURCES}
        src/srext_unzip/windll_subset.c
        src/srext_unzip/infozip_adapter.c
        src/srext_unzip/plugin.cpp
        src/srext_unzip/srEXT_Unzip.def
        "${SREXT_UNZIP_RESOURCE_OBJECT}"
    )
    add_dependencies(SREXT_UNZIP surrender_unzip_import_library)
    target_compile_options(SREXT_UNZIP PRIVATE /nologo /W3 /GX /O2 /Zp4)
    target_compile_definitions(SREXT_UNZIP PRIVATE
        NDEBUG WIN32 _WINDOWS WINDLL USE_EF_UT_TIME DLL
    )
    target_include_directories(SREXT_UNZIP PRIVATE
        include
        "${INFOZIP_SOURCE}"
        "${INFOZIP_SOURCE}/windll"
        "${INFOZIP_SOURCE}/win32"
    )
    target_link_libraries(SREXT_UNZIP PRIVATE "${SR_UNZIP_IMPORT_LIBRARY}")
    target_link_options(SREXT_UNZIP PRIVATE
        /DEBUGTYPE:CV
        /OPT:REF
        /OPT:NOICF
        "/MAP:${CMAKE_CURRENT_BINARY_DIR}/srEXT_Unzip.map"
    )
    set_target_properties(SREXT_UNZIP PROPERTIES
        OUTPUT_NAME srEXT_Unzip
        PREFIX ""
        PDB_NAME srEXT_Unzip
    )
endif()

function(wiz8_add_fid_seed TARGET_NAME SOURCE_ROOT)
    set(options)
    set(one_value_args)
    set(multi_value_args SOURCES OPTIONS DEFINITIONS INCLUDES)
    cmake_parse_arguments(SEED "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
    set(seed_sources ${SEED_SOURCES})
    list(TRANSFORM seed_sources PREPEND "${SOURCE_ROOT}/")
    add_library(${TARGET_NAME} OBJECT EXCLUDE_FROM_ALL ${seed_sources})
    target_compile_options(${TARGET_NAME} PRIVATE ${SEED_OPTIONS})
    target_compile_definitions(${TARGET_NAME} PRIVATE ${SEED_DEFINITIONS})
    target_include_directories(${TARGET_NAME} PRIVATE ${SEED_INCLUDES})
    add_dependencies(fid-seeds ${TARGET_NAME})
endfunction()

if(WIZ8_BUILD_FID_SEEDS)
    add_custom_target(fid-seeds)
    set(fid_seed_count 0)

    if(FID_ZLIB_SOURCE)
        wiz8_collect_zlib_units(ZLIB_UNITS "${FID_ZLIB_SOURCE}")
        foreach(seed_variant release_md_o2 release_md_o2_g6 release_md_o1)
            if(seed_variant STREQUAL release_md_o1)
                set(seed_optimization /O1)
            else()
                set(seed_optimization /O2)
            endif()
            set(seed_options /nologo /MD ${seed_optimization})
            if(seed_variant STREQUAL release_md_o2_g6)
                list(APPEND seed_options /G6)
            endif()
            wiz8_add_fid_seed(fid_zlib_1_0_4_${seed_variant} "${FID_ZLIB_SOURCE}"
                SOURCES ${ZLIB_UNITS}
                OPTIONS ${seed_options}
                INCLUDES "${FID_ZLIB_SOURCE}"
            )
            math(EXPR fid_seed_count "${fid_seed_count} + 1")
        endforeach()
    endif()

    if(FID_JPEG_SOURCE)
        wiz8_collect_ijg_units(IJG_JPEG_UNITS "${FID_JPEG_SOURCE}")
        foreach(seed_variant release_md_o2 release_md_o1)
            if(seed_variant STREQUAL release_md_o1)
                set(seed_optimization /O1)
            else()
                set(seed_optimization /O2)
            endif()
            wiz8_add_fid_seed(fid_ijg_jpeg_6_${seed_variant} "${FID_JPEG_SOURCE}"
                SOURCES ${IJG_JPEG_UNITS}
                OPTIONS /nologo /MD ${seed_optimization}
                INCLUDES
                    "${CMAKE_CURRENT_SOURCE_DIR}/config/fid-overlays/ijg-jpeg-6"
                    "${FID_JPEG_SOURCE}"
            )
            math(EXPR fid_seed_count "${fid_seed_count} + 1")
        endforeach()
    endif()

    if(FID_INFOZIP_SOURCE)
        wiz8_collect_infozip_units(INFOZIP_UNITS "${FID_INFOZIP_SOURCE}")
        foreach(seed_variant upstream_release_mt_o2 target_runtime_md_o2)
            if(seed_variant STREQUAL upstream_release_mt_o2)
                set(seed_runtime /MT)
            else()
                set(seed_runtime /MD)
            endif()
            wiz8_add_fid_seed(fid_infozip_unzip_5_4_${seed_variant} "${FID_INFOZIP_SOURCE}"
                SOURCES ${INFOZIP_UNITS}
                OPTIONS /nologo /Zp4 ${seed_runtime} /W3 /GX /O2
                DEFINITIONS NDEBUG WIN32 _WINDOWS WINDLL USE_EF_UT_TIME DLL
                INCLUDES
                    "${FID_INFOZIP_SOURCE}"
                    "${FID_INFOZIP_SOURCE}/windll"
                    "${FID_INFOZIP_SOURCE}/win32"
            )
            math(EXPR fid_seed_count "${fid_seed_count} + 1")
        endforeach()
    endif()

    if(fid_seed_count EQUAL 0)
        message(FATAL_ERROR "WIZ8_BUILD_FID_SEEDS requires at least one FID source path")
    endif()
endif()

if(WIZ8_BUILD_TOOLCHAIN_PROBE)
    add_executable(fid_toolchain_probe EXCLUDE_FROM_ALL docker/msvc600/probes/rich_probe.cpp)
    target_compile_options(fid_toolchain_probe PRIVATE /nologo /O2 /MD)
    set_target_properties(fid_toolchain_probe PROPERTIES OUTPUT_NAME rich_probe)
    add_custom_target(fid-probe DEPENDS fid_toolchain_probe)
endif()
