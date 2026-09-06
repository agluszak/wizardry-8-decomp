#ifndef WIZ8_SR_API_H
#define WIZ8_SR_API_H

/* Wizardry's recovered call surface for the SurRender DLL. */

typedef void (__cdecl *srAssertHandler)(
    const char* expression,
    const char* source_path,
    long line,
    const char* message);

/* These imports are declared once here so every first-party caller sees the
   same recovered SurRender ABI. */
__declspec(dllimport) int __cdecl srInit(void);
__declspec(dllimport) void __cdecl srAssertSetFunc(srAssertHandler handler);

/*
 * Fixed-arity on purpose, and the arity is load-bearing evidence. The DLL
 * export is variadic (?srAssertFail@@YAXPBD0J0ZZ), but VC6 SP5 refuses to
 * defer a pending inner-call stack cleanup across a call it believes is
 * variadic, and the canonical bodies that pass a String(...) result as the
 * message (CharacterPointerToPartySlot, RPCPtrToPCSlot, MonsterInfoFromID)
 * fold that cleanup across the assert call. The original translation units
 * therefore saw a fixed-arity declaration, and their import library mapped
 * its mangling onto the variadic export. The generated sr-assert-import.lib
 * reproduces that IAT-symbol/hint-name distinction without a code wrapper. The
 * line is long because the true ABI spells it long; int folds identically,
 * so only the arity is proven, not the width.
 */
__declspec(dllimport) void __cdecl srAssertFail(
    const char* expression,
    const char* source_path,
    long line,
    const char* message);

#endif
