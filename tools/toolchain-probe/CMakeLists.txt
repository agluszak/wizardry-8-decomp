cmake_minimum_required(VERSION 3.20)
project(wiz8_toolchain_probe LANGUAGES CXX)

add_executable(fid_toolchain_probe EXCLUDE_FROM_ALL ../../docker/msvc600/probes/rich_probe.cpp)
target_compile_options(fid_toolchain_probe PRIVATE /nologo /O2 /MD)
set_target_properties(fid_toolchain_probe PROPERTIES OUTPUT_NAME rich_probe)

add_executable(lifecycle_probe EXCLUDE_FROM_ALL ../../docker/msvc600/probes/lifecycle_probe.cpp)
target_compile_options(lifecycle_probe PRIVATE /nologo /O2 /MD /GR-)
set_target_properties(lifecycle_probe PROPERTIES OUTPUT_NAME lifecycle_probe)

add_custom_target(fid-probe DEPENDS fid_toolchain_probe lifecycle_probe)
