#pragma once

// Keep modern structural annotations visible to the lint compiler while
// making them syntax-neutral for the C++98 matching toolchain.
#if __cplusplus < 201103L
#define override
#endif

/* VC6 treats wchar_t as an unsigned-short typedef.  Clang normally makes it
   a distinct built-in type even for a Windows target; the lint lane disables
   that built-in and recreates the legacy ABI spelling before VC6 headers are
   parsed. */
#if defined(WIZ8_CLANG_LINT) && !defined(_WCHAR_T_DEFINED)
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
