/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Distributed under the accompanying SFI Source Code license agreement. */
#ifndef	__DirectX_Common_H__
#define	__DirectX_Common_H__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

	// local functions
	void  DirectXAttempt ( INT32 iErrorCode, INT32 nLine, char *szFilename );
	void  DirectXAssert ( BOOLEAN fValue, INT32 nLine, char *szFilename );
	void  DirectXZeroMem ( void* pMemory, int nSize );

	#undef	ATTEMPT
	#define	ATTEMPT(x)	DirectXAttempt ((x),__LINE__,__FILE__)

	#undef	ZEROMEM
	#define	ZEROMEM(x)	DirectXZeroMem ( (void*)&(x), sizeof(x) )

	#undef	DEBUGMSG
	#define	DEBUGMSG(x)	OutputDebugString(x)

#ifdef __cplusplus
}
#endif

#endif
