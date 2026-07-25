cmake_minimum_required(VERSION 3.20)

project(wizardry8_decomp LANGUAGES C CXX)

option(WIZ8_BUILD_DECOMP "Configure recovered Wizardry binaries" ON)
option(WIZ8_BUILD_FID_SEEDS "Configure source-built FID seed objects" OFF)
option(WIZ8_BUILD_TOOLCHAIN_PROBE "Configure the VC6 Rich-header probe" OFF)

set(FID_ZLIB_SOURCE "" CACHE PATH "Pinned zlib source tree used for FID seeds")
set(FID_JPEG_SOURCE "" CACHE PATH "Pinned IJG JPEG source tree used for FID seeds")
set(FID_INFOZIP_SOURCE "" CACHE PATH "Pinned Info-ZIP source tree used for FID seeds")
set(INFOZIP_SOURCE "" CACHE PATH "Pinned Info-ZIP UnZip 5.4 source tree")
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
    wiz8_collect_ijg_units(IJG_JPEG_UNITS "${IJG_JPEG_SOURCE}")
    set(IJG_JPEG_SOURCES ${IJG_JPEG_UNITS})
    list(TRANSFORM IJG_JPEG_SOURCES PREPEND "${IJG_JPEG_SOURCE}/")
    set(IJG_STDIO_SOURCES
        "${IJG_JPEG_SOURCE}/jdatadst.c"
        "${IJG_JPEG_SOURCE}/jdatasrc.c"
    )
    set_source_files_properties(${IJG_STDIO_SOURCES} PROPERTIES
        COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/src/srext_jpegimporter/ijg_stdio_override.h"
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
        src/srext_jpegimporter/srEXT_JPEGImporter.def
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
            "project: '${RECCMP_PROJECT_DIR_HOST}'\ntargets:\n  SREXT_JPEGIMPORTER:\n    path: '${RECCMP_BUILD_DIR_HOST}/srEXT_JPEGImporter.dll'\n    pdb: '${RECCMP_BUILD_DIR_HOST}/srEXT_JPEGImporter.pdb'\n  SREXT_UNZIP:\n    path: '${RECCMP_BUILD_DIR_HOST}/srEXT_Unzip.dll'\n    pdb: '${RECCMP_BUILD_DIR_HOST}/srEXT_Unzip.pdb'\n"
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
