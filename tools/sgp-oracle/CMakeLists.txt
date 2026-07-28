cmake_minimum_required(VERSION 3.20)
project(wiz8_sgp_oracle LANGUAGES C CXX)

get_filename_component(WIZ8_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(SGP_SOURCE "${WIZ8_REPO_ROOT}/third_party/sfi-sgp/sgp" CACHE PATH
    "Vendored SFI-licensed Standard Gaming Platform source tree")
include("${WIZ8_REPO_ROOT}/cmake/SgpModel.cmake")
wiz8_load_sgp_model()

wiz8_add_sgp_group(WIZ8_SGP_RETAINED ${WIZ8_SGP_RETAINED_SOURCES})
wiz8_add_sgp_group(WIZ8_SGP_RUNTIME_SHARED ${WIZ8_SGP_RUNTIME_SHARED_SOURCES})
wiz8_add_sgp_group(WIZ8_SGP_PROBE_ONLY ${WIZ8_SGP_PROBE_ONLY_SOURCES})
add_custom_target(WIZ8_SGP_PROBES)
add_dependencies(WIZ8_SGP_PROBES
    WIZ8_SGP_RETAINED
    WIZ8_SGP_RUNTIME_SHARED
    WIZ8_SGP_PROBE_ONLY
)
