/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Add matching markers for retained SGP functions and globals.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "types.h"
#include <objbase.h>
#include <initguid.h>

#include "types.h"
#include <ddraw.h>
#include "DirectX Common.h"
#include <windows.h>
#include "debug.h"

// FUNCTION: WIZ8 0x004146e0
void  DirectXZeroMem ( void* pMemory, int nSize )
{
	memset ( pMemory, 0, nSize );
}


void DirectXAttempt ( INT32 iErrorCode, INT32 nLine, char *szFilename )
{
}

