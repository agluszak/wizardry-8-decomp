cmake_minimum_required(VERSION 3.26)

cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH WIZ8_REPOSITORY)
set(CMAKE_CURRENT_SOURCE_DIR "${WIZ8_REPOSITORY}")
include("${WIZ8_REPOSITORY}/src/wiz8/sources.cmake")

message(STATUS "WIZ8 source inventory is complete and uniquely classified")
