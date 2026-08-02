cmake_minimum_required(VERSION 3.20)
project(wiz8_recovery_fixture LANGUAGES CXX)

set(LIFECYCLE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/recovery/lifecycle.cpp"
    CACHE FILEPATH "Lifecycle fixture source to compile")
set(LIFECYCLE_OUTPUT_NAME lifecycle_probe CACHE STRING "Lifecycle fixture output name")

add_executable(recovery_lifecycle EXCLUDE_FROM_ALL "${LIFECYCLE_SOURCE}")
target_compile_options(recovery_lifecycle PRIVATE /nologo /O2 /MD /GR-)
set_target_properties(recovery_lifecycle PROPERTIES OUTPUT_NAME "${LIFECYCLE_OUTPUT_NAME}")

add_custom_target(recovery-lifecycle DEPENDS recovery_lifecycle)
