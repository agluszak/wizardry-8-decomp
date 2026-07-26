#ifndef WIZ8_SR_API_H
#define WIZ8_SR_API_H

/*
 * Proven source-side call surface. The DLL export is variadic and spells the
 * line as long, but every recovered call uses four arguments and VC6 emits the
 * canonical bodies only with this fixed-arity int declaration. Both are the
 * same 32-bit cdecl call ABI; CMake aliases the import symbol, not the body.
 */
__declspec(dllimport) void __cdecl srAssertFail(
    const char* expression,
    const char* source_path,
    int line,
    const char* message);

#endif
