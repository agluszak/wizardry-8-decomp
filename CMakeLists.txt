cmake_minimum_required(VERSION 3.20)

project(wizardry8_decomp LANGUAGES C CXX)

if(NOT MSVC)
    message(FATAL_ERROR "The matching build requires the pinned MSVC 6 container")
endif()

enable_language(RC)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "/O2 /MD /Z7 /DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /MD /GX /GR- /Z7 /DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
    "/DEBUG /INCREMENTAL:NO /BASE:0x10000000" CACHE STRING "" FORCE)

include(cmake/Product.cmake)
