set(INFOZIP_SOURCE "" CACHE PATH "Pinned Info-ZIP UnZip 5.4 source tree")

# The release-5.4 windll/visualc/lib source order, with api.c and
# windll/windll.c replaced by the recovered Sir-Tech subsets below.
set(SREXT_UNZIP_INFOZIP_SOURCES
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
    zipinfo.c
)
list(TRANSFORM SREXT_UNZIP_INFOZIP_SOURCES PREPEND "${INFOZIP_SOURCE}/")

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

if(NOT WIZ8_ANALYSIS_BUILD)
    wiz8_add_import_library(
        SREXT_UNZIP_SR
        sr-unzip-imports.def
    )

    wiz8_compile_resource(SREXT_UNZIP_RESOURCE_OBJECT srEXT_Unzip.rc)

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
    add_dependencies(SREXT_UNZIP ${SREXT_UNZIP_SR_IMPORT_TARGET})
    target_compile_options(SREXT_UNZIP PRIVATE /nologo /W3 /Z7 /GX /O2 /Zp4 /MD)
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
        /DEBUG /INCREMENTAL:NO /BASE:0x10000000
        /DEBUGTYPE:CV
        /OPT:REF
        /OPT:NOICF
        "/MAP:${CMAKE_BINARY_DIR}/srEXT_Unzip.map"
    )
    set_target_properties(SREXT_UNZIP PROPERTIES OUTPUT_NAME srEXT_Unzip)

endif()
