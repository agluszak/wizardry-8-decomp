#pragma once

#include "Types.h"

/* SGP text and file APIs spell narrow text as UINT8* (their released source
   uses raw byte pointers for ASCII strings and paths). Wizardry recovers the
   same storage as char*. One documented boundary helper avoids repeating the
   SGP narrow-text conversion at every LoadFontFile / filename call site. */
inline UINT8* Wiz8ToSgpNarrowText(char* text)
{
    // reinterpret-ok: SGP UINT8* narrow-text ABI
    return reinterpret_cast<UINT8*>(text);
}

inline UINT8* Wiz8ToSgpNarrowText(const char* text)
{
    // reinterpret-ok: SGP UINT8* narrow-text ABI
    return const_cast<UINT8*>(reinterpret_cast<const UINT8*>(text));
}

/* SGP's String() returns its rotating buffer as UINT8*; Wizardry passes that
   storage on as ordinary narrow text. */
inline const char* SgpToWiz8NarrowText(const UINT8* text)
{
    // reinterpret-ok: SGP UINT8* narrow-text ABI
    return reinterpret_cast<const char*>(text);
}
