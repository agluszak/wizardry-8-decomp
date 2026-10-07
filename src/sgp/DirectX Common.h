/* Modified for the Wizardry 8 reconstruction: 2026-10-03, 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
#ifndef __DirectX_Common_H__
#define __DirectX_Common_H__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// local functions
void DirectXAttempt(INT32 iErrorCode, INT32 nLine, char* szFilename);
void DirectXAssert(BOOLEAN fValue, INT32 nLine, char* szFilename);
void DirectXZeroMem(void* pMemory, int nSize);

#undef ATTEMPT
#define ATTEMPT_AT(x, line) DirectXAttempt((x), (line), __FILE__)
#define ATTEMPT(x) ATTEMPT_AT((x), __LINE__)

#undef ZEROMEM
#define ZEROMEM(x) DirectXZeroMem((void*)&(x), sizeof(x))

#undef DEBUGMSG
#define DEBUGMSG(x) OutputDebugString(x)

#ifdef __cplusplus
}
#endif

#endif
