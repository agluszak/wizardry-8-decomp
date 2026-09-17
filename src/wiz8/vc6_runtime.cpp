/*
 * Linker- and CRT-owned functions in the canonical executable. As in
 * imperialism-decomp, LIBRARY markers give reccmp address ownership without
 * pretending these bodies are first-party recovered source.
 */

// LIBRARY: WIZ8 0x00401000
// __WinMainCRTStartup

// LIBRARY: WIZ8 0x0040115e
// _XcptFilter

// LIBRARY: WIZ8 0x00401164
// _initterm

// LIBRARY: WIZ8 0x004011c0
// _except_handler3

// LIBRARY: WIZ8 0x005e1c30
// __aulldiv

// LIBRARY: WIZ8 0x00401180
// _onexit

// LIBRARY: WIZ8 0x004011ac
// atexit

// LIBRARY: WIZ8 0x005e1c10
// ??3@YAXPAX@Z

// LIBRARY: WIZ8 0x005e1c1c
// free_import_thunk

// LIBRARY: WIZ8 0x005e1ca0
// __allmul

// LIBRARY: WIZ8 0x005e1ce0
// operator_new_import_thunk

// LIBRARY: WIZ8 0x005e1cf0
// __alldiv

// LIBRARY: WIZ8 0x005e1da0
// __alloca_probe

// LIBRARY: WIZ8 0x005e1dd0
// __aullshr

// LIBRARY: WIZ8 0x005e1def
// ??_L@YGXPAXIHP6EX0@Z1@Z

// LIBRARY: WIZ8 0x005e1e71
// ??_M@YGXPAXIHP6EX0@Z@Z

// LIBRARY: WIZ8 0x005e1ef1
// ?__ArrayUnwind@@YGXPAXIHP6EX0@Z@Z

// LIBRARY: WIZ8 0x0040116a
// _cfltcvt_init

// LIBRARY: WIZ8 0x004028c0
// _wcsnicmp

/* zlib 1.0.4 corpus in Wiz8.exe. Names come from docs/libraries/zlib-1.0.4.md;
   the source-oracle gate also treats the whole 0x00415910-0x0041A7ED span as
   library-owned even when an interior helper is still unnamed here. */

// LIBRARY: WIZ8 0x00415910
// inflateReset

// LIBRARY: WIZ8 0x00415960
// _inflateEnd

// LIBRARY: WIZ8 0x00415ad0
// _inflateInit_

// LIBRARY: WIZ8 0x00415af0
// _inflate

// LIBRARY: WIZ8 0x00415f60
// deflate_stored

// LIBRARY: WIZ8 0x004165c0
// deflate_slow

// LIBRARY: WIZ8 0x00417810
// adler32

// LIBRARY: WIZ8 0x00417940
// zcalloc

// LIBRARY: WIZ8 0x00417960
// zcfree

// SYNTHETIC: WIZ8 0x004023a0
// NoOp (compiler-folded empty)
