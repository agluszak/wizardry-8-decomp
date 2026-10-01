#pragma once

#include "Types.h"

#include <wchar.h>

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

inline UINT16* Wiz8ToSgpWideText(wchar_t* text)
{
    // reinterpret-ok: SGP UINT16* wide-text ABI; Win32 wchar_t is 16-bit
    return reinterpret_cast<UINT16*>(text);
}

inline UINT16* Wiz8ToSgpWideText(const wchar_t* text)
{
    // reinterpret-ok: SGP UINT16* wide-text ABI; Win32 wchar_t is 16-bit
    return const_cast<UINT16*>(reinterpret_cast<const UINT16*>(text));
}
