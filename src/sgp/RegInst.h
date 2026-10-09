/* Modified for the Wizardry 8 reconstruction: 2026-10-03, 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
//**************************************************************************
//
// Filename :	RegInst.h
//
//	Purpose :	prototypes for the registry stuff
//
// Modification history :
//
//		02dec96:HJH				- Creation
//
//**************************************************************************

#ifndef _RegInst_h
#define _RegInst_h

//**************************************************************************
//
//				Includes
//
//**************************************************************************

#include <windows.h>
#include <tchar.h>
#include <assert.h>

#include "Types.h"

//**************************************************************************
//
//				Defines
//
//**************************************************************************

//**************************************************************************
//
//				Typedefs
//
//**************************************************************************

//**************************************************************************
//
//				Function Prototypes
//
//**************************************************************************

#ifdef __cplusplus
extern "C" {
#endif

// call once per execution of application:
extern BOOLEAN InitializeRegistryKeys(STR strAppName, STR strRegistryKey);

// returns key for HKEY_CURRENT_USER\"Software"\RegistryKey\ProfileName
// creating it if it doesn't exist
// responsibility of the caller to call RegCloseKey() on the returned HKEY
// returns key for:
//      HKEY_CURRENT_USER\"Software"\RegistryKey\AppName\lpszSection
// creating it if it doesn't exist.
// responsibility of the caller to call RegCloseKey() on the returned HKEY
#ifdef __cplusplus
}
#endif

#endif
