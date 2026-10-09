#pragma once

#include "Types.h"

#include <wchar.h>

static_assert(sizeof(wchar_t) == 2, "Wizardry wide text requires 16-bit code units");
static_assert(sizeof(UINT16) == sizeof(wchar_t), "SGP wide text must preserve Wizardry code units");

/* The released SGP interfaces spell byte strings as UINT8* and wide strings
   as UINT16*. Wizardry uses char* and Win32/VC6 wchar_t* for the same storage.
   Keep those ABI-only reinterpretations at one boundary instead of repeating
   casts throughout recovered game code. */
inline UINT8* Wiz8ToSgpText(char* text)
{
    // reinterpret-ok: SGP byte-text ABI uses UINT8* for char storage
    return reinterpret_cast<UINT8*>(text);
}

inline UINT8* Wiz8ToSgpText(const char* text)
{
    // reinterpret-ok: SGP byte-text ABI uses mutable UINT8* for input text
    return reinterpret_cast<UINT8*>(const_cast<char*>(text));
}

inline CHAR16* Wiz8ToSgpWideText(wchar_t* text)
{
    // reinterpret-ok: SGP CHAR16* wide-text ABI; Win32 wchar_t is 16-bit
    return reinterpret_cast<CHAR16*>(text);
}

inline CHAR16* Wiz8ToSgpWideText(const wchar_t* text)
{
    // reinterpret-ok: SGP CHAR16* wide-text ABI; Win32 wchar_t is 16-bit
    return const_cast<CHAR16*>(reinterpret_cast<const CHAR16*>(text));
}
