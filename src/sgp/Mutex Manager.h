/* Modified for the Wizardry 8 reconstruction, 2026-10-04.
   Remove the unused local configuration include.
   Formatting normalized for the Wizardry 8 reconstruction, 2026-10-06.
   Distributed under the accompanying SFI Source Code license agreement. */
#ifndef __MUTEX_
#define __MUTEX_

#include <process.h>
#include "Types.h"

extern BOOLEAN InitializeMutexManager(void);
extern void ShutdownMutexManager(void);
extern BOOLEAN InitializeMutex(UINT32 uiMutexIndex, UINT8* ubMutexName);
extern BOOLEAN DeleteMutex(UINT32 uiMutexIndex);
extern BOOLEAN EnterMutex(UINT32 uiMutexIndex, INT32 nLine, char* szFilename);
extern BOOLEAN EnterMutexWithTimeout(UINT32 uiMutexIndex, UINT32 uiTimeout, INT32 nLine,
                                     char* szFilename);
extern BOOLEAN LeaveMutex(UINT32 uiMutexIndex, INT32 nLine, char* szFilename);

#endif
