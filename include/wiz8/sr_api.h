#ifndef WIZ8_SR_API_H
#define WIZ8_SR_API_H

/* Wizardry's recovered call surface for the SurRender DLL. */

typedef void(__cdecl* srAssertHandler)(const char* expression, const char* source_path, long line,
                                       const char* message);

/* These imports are declared once here so every first-party caller sees the
   same recovered SurRender ABI. */
__declspec(dllimport) int __cdecl srInit(void);
__declspec(dllimport) int __cdecl srExit(void);
__declspec(dllimport) void __cdecl srAssertSetFunc(srAssertHandler handler);

/*
 * The recovered consumer declaration has four fixed arguments. The provider
 * export is variadic (?srAssertFail@@YAXPBD0J0ZZ). The product build maps the
 * fixed-arity COFF reference to that export without a code wrapper. The original
 * consumer prototype and import-library construction remain unresolved.
 * abi-prototype-ok: the consumer spelling is intentionally fixed-arity while
 * the provider export is variadic (?srAssertFail@@YAXPBD0J0ZZ).
 */
__declspec(dllimport) void __cdecl srAssertFail(const char* expression, const char* source_path,
                                                long line, const char* message);

#endif
