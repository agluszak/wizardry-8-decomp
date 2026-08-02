cmake_minimum_required(VERSION 3.20)
project(wiz8_recovery_fixture LANGUAGES CXX)

add_executable(recovery_lifecycle EXCLUDE_FROM_ALL ../../tests/recovery/lifecycle.cpp)
target_compile_options(recovery_lifecycle PRIVATE /nologo /O2 /MD /GR-)
set_target_properties(recovery_lifecycle PROPERTIES OUTPUT_NAME lifecycle_probe)

add_custom_target(recovery-lifecycle DEPENDS recovery_lifecycle)
