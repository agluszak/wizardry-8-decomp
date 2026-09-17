cmake_minimum_required(VERSION 3.20)
project(wiz8_clang_tidy_plugin LANGUAGES CXX)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

add_definitions(${LLVM_DEFINITIONS})
add_library(wiz8-clang-tidy MODULE RedundantScalarCastCheck.cpp)
target_compile_features(wiz8-clang-tidy PRIVATE cxx_std_17)
target_include_directories(wiz8-clang-tidy SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_link_libraries(wiz8-clang-tidy PRIVATE clangASTMatchers clangTidy)
set_target_properties(wiz8-clang-tidy PROPERTIES PREFIX "")

install(TARGETS wiz8-clang-tidy LIBRARY DESTINATION lib)
