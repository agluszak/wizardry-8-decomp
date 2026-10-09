/* Modified for the Wizardry 8 reconstruction: 2026-10-03, 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "Types.h"
#include <objbase.h>
#include <initguid.h>

#include "Types.h"
#include <ddraw.h>
#include "DirectX Common.h"
#include <windows.h>
#include "DEBUG.H"

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
