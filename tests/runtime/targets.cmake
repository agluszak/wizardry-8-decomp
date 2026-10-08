# Runtime-only object variants and the console harness. Included from src/wiz8 so
# these targets share the historical product's link inputs and recovered objects.
add_library(wiz8_test_instrumented_objects OBJECT ${WIZ8_INSTRUMENTABLE_SOURCE_FILES})
target_link_libraries(wiz8_test_instrumented_objects PRIVATE wiz8_compile_settings)
target_compile_options(wiz8_test_instrumented_objects PRIVATE /Ob2)
target_compile_definitions(wiz8_test_instrumented_objects PRIVATE WIZ8_RUNTIME_TESTS=1)
target_include_directories(wiz8_test_instrumented_objects PRIVATE
    "${PROJECT_SOURCE_DIR}/tests/runtime/instrumentation")
wiz8_lint_target(wiz8_test_instrumented_objects)

set(WIZ8_RUNTIME_TEST_SOURCES
    "${PROJECT_SOURCE_DIR}/tests/runtime/game_thread_executor.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/runtime_case.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/instrumentation/runtime_instrumentation.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/gameplay_actions.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/combat_actions.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/character_actions.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/wiz8_runtime_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/oct_file_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/level_file_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/save_file_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/keyboard_menu_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/npc_dialogue_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/lock_device_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/mongen_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/sight_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/split_stack_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/audio_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/mouth_gap_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/party_movement_semantic_test.cpp"
    "${PROJECT_SOURCE_DIR}/tests/runtime/wiz8_crash_report.cpp"
)
add_library(wiz8_runtime_test_objects OBJECT ${WIZ8_RUNTIME_TEST_SOURCES})
target_link_libraries(wiz8_runtime_test_objects PRIVATE wiz8_compile_settings)
target_compile_definitions(wiz8_runtime_test_objects PRIVATE WIZ8_RUNTIME_TESTS=1)
target_include_directories(wiz8_runtime_test_objects PRIVATE
    "${PROJECT_SOURCE_DIR}/tests/runtime/instrumentation")
wiz8_lint_target(wiz8_runtime_test_objects)

if(NOT WIZ8_ANALYSIS_BUILD)
    add_executable(WIZ8_RUNTIME WIN32
        $<TARGET_OBJECTS:wiz8_recovered_objects>
        $<TARGET_OBJECTS:wiz8_recovered_instrumentable_objects>
        "${PROJECT_SOURCE_DIR}/tests/runtime/wiz8_crash_report.cpp"
    )
    wiz8_configure_executable(WIZ8_RUNTIME Wiz8Runtime)
    target_link_options(WIZ8_RUNTIME PRIVATE
        /DEBUG /DEBUGTYPE:CV /PDBTYPE:SEPT /INCREMENTAL:NO /OPT:REF /OPT:NOICF /FIXED
        /BASE:0x400000 /FILEALIGN:0x1000
        /OSVERSION:4.0 /SUBSYSTEM:WINDOWS,4.0
        /STACK:0x100000,0x1000 /HEAP:0x100000,0x1000
        "/MAP:${CMAKE_BINARY_DIR}/Wiz8Runtime.map" /MAPINFO:LINES
    )

    # The console harness selects the instrumented game objects and test driver.
    add_executable(WIZ8_RUNTIME_TEST
        $<TARGET_OBJECTS:wiz8_recovered_objects>
        $<TARGET_OBJECTS:wiz8_test_instrumented_objects>
        $<TARGET_OBJECTS:wiz8_runtime_test_objects>
    )
    wiz8_configure_executable(WIZ8_RUNTIME_TEST Wiz8RuntimeTest)
    add_dependencies(WIZ8_RUNTIME_TEST SURRENDER)
    target_link_options(WIZ8_RUNTIME_TEST PRIVATE
        /DEBUG /DEBUGTYPE:CV /PDBTYPE:SEPT /INCREMENTAL:NO /OPT:REF /OPT:NOICF /FIXED
        /BASE:0x400000 /FILEALIGN:0x1000
        /OSVERSION:4.0 /SUBSYSTEM:CONSOLE,4.0
        /STACK:0x100000,0x1000 /HEAP:0x100000,0x1000
        "/MAP:${CMAKE_BINARY_DIR}/Wiz8RuntimeTest.map" /MAPINFO:LINES
    )
endif()
