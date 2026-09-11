set(INFOZIP_SOURCE "" CACHE PATH "Pinned Info-ZIP UnZip 5.4 source tree")

if(INFOZIP_SOURCE)
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

    # The recovered Sir-Tech contributions stay separate object targets so the
    # canonical DLL link order is preserved around the pristine Info-ZIP block.
    add_library(wiz8_unzip_api_objects OBJECT api_subset.c)
    add_library(wiz8_unzip_windll_objects OBJECT windll_subset.c)
    add_library(wiz8_unzip_adapter_objects OBJECT
        infozip_adapter.c
        plugin.cpp
    )
    foreach(target IN ITEMS
        wiz8_unzip_api_objects
        wiz8_unzip_windll_objects
        wiz8_unzip_adapter_objects
    )
        wiz8_enable_cpp_compat(${target})
        target_compile_options(${target} PRIVATE
            /nologo /W3 /Z7 /O2 /Zp4 "$<$<CXX_COMPILER_ID:MSVC>:/GX>")
        target_compile_definitions(${target} PRIVATE
            NDEBUG WIN32 _WINDOWS WINDLL USE_EF_UT_TIME DLL
        )
        target_include_directories(${target} PRIVATE
            "${PROJECT_SOURCE_DIR}/include"
            "${INFOZIP_SOURCE}"
            "${INFOZIP_SOURCE}/windll"
            "${INFOZIP_SOURCE}/win32"
        )
        target_link_libraries(${target} PRIVATE wiz8_compile_settings)
        wiz8_lint_target(${target})
    endforeach()

    set(SREXT_UNZIP_INFOZIP_SOURCES ${INFOZIP_UNZIP_SOURCES})
    list(REMOVE_ITEM SREXT_UNZIP_INFOZIP_SOURCES
        "${INFOZIP_SOURCE}/api.c"
        "${INFOZIP_SOURCE}/windll/windll.c")

    if(WIZ8_BUILD_PRODUCTS)
        wiz8_add_import_library(
            SREXT_UNZIP_SR
            sr-unzip-imports.def
        )

        find_program(CVTRES_EXECUTABLE cvtres.exe REQUIRED)
        set(SREXT_UNZIP_RESOURCE_RES
            "${CMAKE_CURRENT_BINARY_DIR}/srEXT_Unzip_resource.res")
        set(SREXT_UNZIP_RESOURCE_OBJECT
            "${CMAKE_CURRENT_BINARY_DIR}/srEXT_Unzip_resource.obj")
        add_custom_command(
            OUTPUT "${SREXT_UNZIP_RESOURCE_OBJECT}"
            COMMAND "${CMAKE_RC_COMPILER}"
                "/fo${SREXT_UNZIP_RESOURCE_RES}"
                "${CMAKE_CURRENT_SOURCE_DIR}/srEXT_Unzip.rc"
            COMMAND "${CVTRES_EXECUTABLE}" /nologo /machine:ix86
                "/out:${SREXT_UNZIP_RESOURCE_OBJECT}"
                "${SREXT_UNZIP_RESOURCE_RES}"
            DEPENDS srEXT_Unzip.rc
            VERBATIM
        )
        set_source_files_properties("${SREXT_UNZIP_RESOURCE_OBJECT}" PROPERTIES
            EXTERNAL_OBJECT TRUE
            GENERATED TRUE
        )

        # The original image places the Info-ZIP object sequence before
        # Sir-Tech's narrowed WinDLL front end, memory wrapper, and SurRender
        # plug-in object.
        add_library(SREXT_UNZIP SHARED
            $<TARGET_OBJECTS:wiz8_unzip_api_objects>
            ${SREXT_UNZIP_INFOZIP_SOURCES}
            $<TARGET_OBJECTS:wiz8_unzip_windll_objects>
            $<TARGET_OBJECTS:wiz8_unzip_adapter_objects>
            srEXT_Unzip.def
            "${SREXT_UNZIP_RESOURCE_OBJECT}"
        )
        wiz8_enable_cpp_compat(SREXT_UNZIP)
        add_dependencies(SREXT_UNZIP ${SREXT_UNZIP_SR_IMPORT_TARGET})
        target_compile_options(SREXT_UNZIP PRIVATE /nologo /W3 /Z7 /GX /O2 /Zp4)
        target_compile_definitions(SREXT_UNZIP PRIVATE
            NDEBUG WIN32 _WINDOWS WINDLL USE_EF_UT_TIME DLL
        )
        target_include_directories(SREXT_UNZIP PRIVATE
            "${PROJECT_SOURCE_DIR}/include"
            "${INFOZIP_SOURCE}"
            "${INFOZIP_SOURCE}/windll"
            "${INFOZIP_SOURCE}/win32"
        )
        target_link_libraries(SREXT_UNZIP PRIVATE "${SREXT_UNZIP_SR_IMPORT_LIBRARY}")
        target_link_options(SREXT_UNZIP PRIVATE
            /DEBUGTYPE:CV
            /OPT:REF
            /OPT:NOICF
            "/MAP:${CMAKE_BINARY_DIR}/srEXT_Unzip.map"
        )
        set_target_properties(SREXT_UNZIP PROPERTIES
            OUTPUT_NAME srEXT_Unzip
            PREFIX ""
            PDB_NAME srEXT_Unzip
        )
        reccmp_add_target(SREXT_UNZIP)
    endif()
endif()
