# One definition of the compiler settings every first-party target uses, in
# both the VC6 product build and the clang-cl lint build. Keeping this in a
# single module is what stops the lint lane from drifting into a parallel
# approximation of the product build. `/G6` is a VC6-only code-generation
# flag, so it stays behind a compiler-id condition rather than leaking into
# clang-cl as a file name.
add_library(wiz8_compile_settings INTERFACE)
target_compile_options(wiz8_compile_settings INTERFACE
    /nologo /Z7
    "/FI${CMAKE_CURRENT_SOURCE_DIR}/include/wiz8/compat/compiler.h"
    /O2
    "$<$<COMPILE_LANG_AND_ID:C,MSVC>:/G6>"
    "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/G6>"
)
target_compile_definitions(wiz8_compile_settings INTERFACE
    NDEBUG NOMINMAX WIN32_LEAN_AND_MEAN
)
target_include_directories(wiz8_compile_settings INTERFACE
    include
    include/wiz8
    include/wiz8/engine_code
    include/wiz8/sgp-compat
    src/sgp
)
