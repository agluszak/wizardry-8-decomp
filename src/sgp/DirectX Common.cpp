/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Add matching markers for retained SGP functions and globals.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Annotate the retail addresses of the DirectDraw interface GUIDs this unit defines.
   Formatting normalized for the Wizardry 8 reconstruction, 2026-10-06.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "types.h"
#include <objbase.h>
#include <initguid.h>

#include "types.h"
#include <ddraw.h>
#include "DirectX Common.h"
#include <windows.h>
#include "debug.h"

// With initguid.h this unit defines the ddraw.h GUIDs; retail keeps them in
// .rdata in declaration order.
// GLOBAL: WIZ8 0x005ebb78
// IID_IDirectDraw2

// GLOBAL: WIZ8 0x005ebba8
// IID_IDirectDrawSurface2

// FUNCTION: WIZ8 0x004146e0
void DirectXZeroMem(void* pMemory, int nSize)
{
    memset(pMemory, 0, nSize);
}

void DirectXAttempt(INT32 iErrorCode, INT32 nLine, char* szFilename) {}
