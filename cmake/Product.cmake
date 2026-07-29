set(ZLIB_SOURCE "" CACHE PATH "Pinned zlib 1.0.4 source tree used by recovered game code")
set(INFOZIP_SOURCE "" CACHE PATH "Pinned Info-ZIP UnZip 5.4 source tree")
set(SGP_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sfi-sgp/sgp" CACHE PATH
    "Vendored SFI-licensed Standard Gaming Platform source tree")
if(NOT IJG_JPEG_SOURCE)
    message(FATAL_ERROR "IJG_JPEG_SOURCE must point at the pinned IJG release 6 tree")
endif()

add_library(wiz8_compile_settings INTERFACE)
target_compile_options(wiz8_compile_settings INTERFACE
    /nologo /O2 /G6 /MD
    "/FI${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/compat/compiler.h"
)
target_compile_definitions(wiz8_compile_settings INTERFACE NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(wiz8_compile_settings INTERFACE
    include
    include/wiz8/sgp-compat
    "${SGP_SOURCE}"
)

function(wiz8_enable_cpp_compat TARGET)
    target_compile_options(${TARGET} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:/FI${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/compat/compiler.h>"
    )
endfunction()

function(wiz8_add_import_library NAME DEF_FILE)
    string(TOLOWER "${NAME}" target_stem)
    set(import_library "${CMAKE_CURRENT_BINARY_DIR}/${target_stem}.lib")
    add_custom_command(
        OUTPUT "${import_library}"
        COMMAND lib.exe /nologo /machine:ix86
            "/def:${CMAKE_CURRENT_SOURCE_DIR}/${DEF_FILE}"
            "/out:${import_library}"
        DEPENDS "${DEF_FILE}"
        VERBATIM
    )
    add_custom_target(${target_stem}_import_library DEPENDS "${import_library}")
    set(${NAME}_IMPORT_LIBRARY "${import_library}" PARENT_SCOPE)
    set(${NAME}_IMPORT_TARGET "${target_stem}_import_library" PARENT_SCOPE)
endfunction()

function(wiz8_add_executable)
    cmake_parse_arguments(ARG "OPT_REF;CONSOLE" "TARGET;OUTPUT;MAP" "LIBRARIES;SOURCES" ${ARGN})
    if(ARG_CONSOLE)
        add_executable(${ARG_TARGET}
            $<TARGET_OBJECTS:wiz8_recovered_objects>
            ${ARG_SOURCES}
        )
        set(subsystem /SUBSYSTEM:CONSOLE,4.0)
    else()
        add_executable(${ARG_TARGET} WIN32
            $<TARGET_OBJECTS:wiz8_recovered_objects>
            ${ARG_SOURCES}
        )
        set(subsystem /SUBSYSTEM:WINDOWS,4.0)
    endif()
    if(TARGET WIZ8_ZLIB_WRAPPERS)
        target_sources(${ARG_TARGET} PRIVATE $<TARGET_OBJECTS:WIZ8_ZLIB_WRAPPERS>)
    endif()
    add_dependencies(${ARG_TARGET}
        ${WIZ8_SR_IMPORT_TARGET}
        ${WIZ8_MSS32_IMPORT_TARGET}
        ${WIZ8_BINKW32_IMPORT_TARGET}
    )
    target_link_libraries(${ARG_TARGET} PRIVATE
        wiz8_compile_settings
        ${ARG_LIBRARIES}
        "${WIZ8_SR_IMPORT_LIBRARY}"
        "${WIZ8_MSS32_IMPORT_LIBRARY}"
        "${WIZ8_BINKW32_IMPORT_LIBRARY}"
    )
    if(TARGET WIZ8_ZLIB_1_0_4)
        target_link_libraries(${ARG_TARGET} PRIVATE WIZ8_ZLIB_1_0_4)
    endif()
    if(ARG_OPT_REF)
        set(opt_ref /OPT:REF)
    else()
        set(opt_ref /OPT:NOREF)
    endif()
    target_link_options(${ARG_TARGET} PRIVATE
        /DEBUG /DEBUGTYPE:CV /INCREMENTAL:NO ${opt_ref} /OPT:NOICF
        /FORCE:UNRESOLVED /BASE:0x400000 /FILEALIGN:0x1000
        /OSVERSION:4.0 ${subsystem}
        /STACK:0x100000,0x1000 /HEAP:0x100000,0x1000
        "/MAP:${CMAKE_CURRENT_BINARY_DIR}/${ARG_MAP}"
    )
    set_target_properties(${ARG_TARGET} PROPERTIES
        OUTPUT_NAME "${ARG_OUTPUT}"
        PREFIX ""
        PDB_NAME "${ARG_OUTPUT}"
    )
endfunction()

function(wiz8_collect_zlib_units RESULT SOURCE_ROOT)
    file(GLOB units CONFIGURE_DEPENDS RELATIVE "${SOURCE_ROOT}" "${SOURCE_ROOT}/*.c")
    list(REMOVE_ITEM units example.c minigzip.c)
    list(SORT units)
    set(${RESULT} ${units} PARENT_SCOPE)
endfunction()

include(src/srext_jpegimporter/CMakeLists.txt)
include(src/surrender/CMakeLists.txt)
include(src/wiz8/CMakeLists.txt)
include(src/srext_unzip/CMakeLists.txt)
