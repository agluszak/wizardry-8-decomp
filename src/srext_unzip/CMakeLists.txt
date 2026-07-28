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

    # Verification-only compilation of the adapter pair. SREXT_UNZIP consumes
    # these sources directly in original link order; it does not consume these
    # probe objects.
    add_library(SREXT_UNZIP_ADAPTER_PROBE OBJECT EXCLUDE_FROM_ALL
        src/srext_unzip/infozip_adapter.c
        src/srext_unzip/plugin.cpp
    )
    target_compile_options(SREXT_UNZIP_ADAPTER_PROBE PRIVATE
        /nologo /W3 /GX /O2 /Zp4
    )
    target_compile_definitions(SREXT_UNZIP_ADAPTER_PROBE PRIVATE
        NDEBUG WIN32 _WINDOWS WINDLL USE_EF_UT_TIME DLL UNZIPLIB
    )
    target_include_directories(SREXT_UNZIP_ADAPTER_PROBE PRIVATE
        include
        "${INFOZIP_SOURCE}"
        "${INFOZIP_SOURCE}/windll"
        "${INFOZIP_SOURCE}/win32"
    )
    add_dependencies(INFOZIP_UNZIP_5_4 SREXT_UNZIP_ADAPTER_PROBE)

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
