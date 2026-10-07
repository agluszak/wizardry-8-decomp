/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Add matching markers for retained SGP functions and globals.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Formatting normalized for the Wizardry 8 reconstruction, 2026-10-06.
   Remove inactive code and decorative comment banners, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
// Filename :	RegInst.c
//	Purpose :	registry routines
// Modification history :
//		02dec96:HJH				- Creation
//				Includes

#include "types.h"
#include "RegInst.h"
#include "WCheck.h"
//				Defines

#define REG_KEY_SIZE 50
//				Variables

// INI strings are not localized
static const TCHAR szSoftware[] = _T("Software");

// GLOBAL: WIZ8 0x00650eac
static CHAR gszRegistryKey[REG_KEY_SIZE];
// GLOBAL: WIZ8 0x00650f14
static CHAR gszAppName[REG_KEY_SIZE];
// GLOBAL: WIZ8 0x00650ee0
static CHAR gszProfileName[REG_KEY_SIZE];
//				Functions

// FUNCTION: WIZ8 0x0040f020
BOOLEAN InitializeRegistryKeys(STR lpszAppName, STR lpszRegistryKey)
{
    CHECKF(lpszAppName != NULL);
    CHECKF(lpszRegistryKey != NULL);
    //CHECKF(gpszRegistryKey == NULL);
    //CHECKF(gpszAppName == NULL);
    //CHECKF(gpszProfileName == NULL);

    // Note: this will leak the original gpszProfileName, but it
    //  will be freed when the application exits.  No assumptions
    //  can be made on how gpszProfileName was allocated.

    strcpy(gszAppName, lpszAppName);
    strcpy(gszRegistryKey, lpszRegistryKey);
    strcpy(gszProfileName, gszAppName);

    return (TRUE);
}
