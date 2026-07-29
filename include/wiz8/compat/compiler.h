#pragma once

// Keep modern structural annotations visible to the lint compiler while
// making them syntax-neutral for the C++98 matching toolchain.
#if __cplusplus < 201103L
#define override

/* Keep source-level layout contracts in standard static_assert form while
   retaining the VC6 matching compiler.  The line-numbered typedef makes each
   assertion independent even when several appear in one scope. */
#define WIZ8_COMPAT_JOIN_INNER(left, right) left##right
#define WIZ8_COMPAT_JOIN(left, right) WIZ8_COMPAT_JOIN_INNER(left, right)
#define static_assert(condition, message)                                      \
    typedef char WIZ8_COMPAT_JOIN(wiz8_static_assertion_at_line_, __LINE__)[  \
        (condition) ? 1 : -1]
#endif

#define WIZ8_JOIN_DETAIL(left, right) left##right
#define WIZ8_JOIN(left, right) WIZ8_JOIN_DETAIL(left, right)
#if defined(WIZ8_CLANG_LINT)
#define WIZ8_ASSERT_SIZE(type, size) static_assert(sizeof(type) == (size), #type " size")
#else
#define WIZ8_ASSERT_SIZE(type, size) \
    typedef char WIZ8_JOIN(wiz8_size_assert_, __LINE__)[sizeof(type) == (size) ? 1 : -1]
#endif

/* VC6 treats wchar_t as an unsigned-short typedef.  Clang normally makes it
   a distinct built-in type even for a Windows target; the lint lane disables
   that built-in and recreates the legacy ABI spelling before VC6 headers are
   parsed. */
#if defined(WIZ8_CLANG_LINT) && !defined(_WCHAR_T_DEFINED)
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif
