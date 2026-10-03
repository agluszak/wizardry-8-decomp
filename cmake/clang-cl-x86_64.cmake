# Diagnostic width change only: retain VC6 headers, source and layout proofs.
# This is deliberately not a linked or runnable x64 product toolchain.
set(WIZ8_ANALYSIS_X64 ON CACHE INTERNAL "Audit Win64 ABI assumptions" FORCE)
include("${CMAKE_CURRENT_LIST_DIR}/clang-cl-i686.cmake")
