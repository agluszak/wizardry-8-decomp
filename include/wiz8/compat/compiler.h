#pragma once

#include <stddef.h>

// Keep modern structural annotations visible to the lint compiler while
// making them syntax-neutral for the C++98 matching toolchain.
#if !defined(WIZ8_CLANG_LINT)
#define override
#endif

/* Keep source-level layout contracts in standard static_assert form while
   retaining the VC6 matching compiler.  The line-numbered typedef makes each
   assertion independent even when several appear in one scope.  C TUs keep the
   shim under lint too: C11 static_assert needs <assert.h>, which the pinned
   VC98 headers do not provide. */
#if !defined(WIZ8_CLANG_LINT) || !defined(__cplusplus)
#define WIZ8_COMPAT_JOIN_INNER(left, right) left##right
#define WIZ8_COMPAT_JOIN(left, right) WIZ8_COMPAT_JOIN_INNER(left, right)
#define static_assert(condition, message)                                                          \
    typedef char WIZ8_COMPAT_JOIN(wiz8_static_assertion_at_line_, __LINE__)[(condition) ? 1 : -1]
#endif

/* VC6 treats wchar_t as an unsigned-short typedef.  Clang normally makes it
   a distinct built-in type even for a Windows target; the lint lane disables
   that built-in and recreates the legacy ABI spelling before VC6 headers are
   parsed. */
#if defined(WIZ8_CLANG_LINT) && !defined(_WCHAR_T_DEFINED)
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

/* Base-subobject offset proofs. A secondary base can move while the total
   size stays identical, so sizeof alone does not pin it, and offsetof cannot
   name a base subobject directly, so each form anchors on a member instead:

   - W8_ASSERT_BASE_OFFSET uses a member the base itself declares: in the
     derived type its offsetof is the base offset plus the member's own
     offset, so the difference pins the base.
   - W8_ASSERT_BASE_END uses the first member declared in the derived class:
     a secondary base at the end of the base area must stop exactly where
     that member begins.
   - W8_ASSERT_BASE_TAIL covers a derived class that adds no members: the
     object ends at the base offset plus the base's extent.

   Virtual bases have no fixed offset - the vbtable carries it - so none of
   these forms apply; a virtual base position is proven by retail vbtable
   evidence instead.

   The expected offset arrives through W8BaseOffset<N>::value rather than a
   bare literal on purpose: the source indexer records any sizeof+integer
   comparison as a proven size for the sizeof'd type, so the literal must not
   appear inside the assertion expression. */
#ifdef __cplusplus
template <int offset> struct W8BaseOffset {
    enum { value = offset };
};
#define W8_ASSERT_BASE_OFFSET(derived, base, member, offset)                                       \
    static_assert(offsetof(derived, member) - offsetof(base, member) ==                            \
                      W8BaseOffset<offset>::value,                                                 \
                  #derived " " #base " base must stay at " #offset)
#define W8_ASSERT_BASE_END(derived, base, member, offset)                                          \
    static_assert(offsetof(derived, member) - sizeof(base) == W8BaseOffset<offset>::value,         \
                  #derived " " #base " base must stay at " #offset)
#define W8_ASSERT_BASE_TAIL(derived, base, offset)                                                 \
    static_assert(sizeof(derived) - sizeof(base) == W8BaseOffset<offset>::value,                   \
                  #derived " " #base " base must stay at " #offset)
#endif
