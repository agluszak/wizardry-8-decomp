#pragma once

#include "Types.h"

#include <wchar.h>

/* SGP font APIs spell wide text as UINT16* (Win32/VC6 text is 16-bit). Wizardry
   recovers the same storage as wchar_t*. One documented boundary helper avoids
   repeating the SGP wide-text conversion at every mprintf / StringPixLength
   call site. */
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
