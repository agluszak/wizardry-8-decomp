/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Include the declarations used by the Wizardry build explicitly.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Formatting normalized for the Wizardry 8 reconstruction, 2026-10-06.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "VObject.h"

// JA2

//**************************************************************************
//
// Filename :	debug.c
//
//	Purpose :	debug manager implementation
//
// Modification history :
//
//		xxxxx96:LH				- Creation
//		xxnov96:HJH				- made it work
//
//**************************************************************************

// Because we're in a library, define SGP_DEBUG here - the client may not always
// use the code to write text, because the header switches on the define
#define SGP_DEBUG

#include "types.h"
#include <windows.h>
#include <ddeml.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "WCheck.h"
#include "TopicIDs.h"
#include "TopicOps.h"
#include "WizShare.h"

//Kris addition

// CJC added
#ifndef _NO_DEBUG_TXT
#include "fileman.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// GLOBAL: WIZ8 0x00650de4
BOOLEAN gfRecordToFile = FALSE;
// GLOBAL: WIZ8 0x005ff538
BOOLEAN gfRecordToDebugger = TRUE;

// moved from header file: 24mar98:HJH
UINT32 guiProfileStart, guiExecutions, guiProfileTime;
INT32 giProfileCount;

// Had to move these outside the ifdef SGP_DEBUG below, because
// they are required for the String() function, which is NOT a
// debug-mode only function, it's used in release-mode as well! -- DB

// GLOBAL: WIZ8 0x006ef440
UINT8 gubAssertString[128];

#define MAX_MSG_LENGTH2 512
// GLOBAL: WIZ8 0x006ee440
UINT8 gbTmpDebugString[8][MAX_MSG_LENGTH2];
// GLOBAL: WIZ8 0x00650dec
UINT8 gubStringIndex = 0;

#ifdef SGP_DEBUG

//**************************************************************************
//
//				Defines
//
//**************************************************************************

#define BUFSIZE 100
#define TIMER_TIMEOUT 1000

//**************************************************************************
//
//				Variables
//
//**************************************************************************

UINT16 TOPIC_MEMORY_MANAGER = INVALID_TOPIC;
UINT16 TOPIC_FILE_MANAGER = INVALID_TOPIC;
UINT16 TOPIC_DATABASE_MANAGER = INVALID_TOPIC;
UINT16 TOPIC_GAME = INVALID_TOPIC;
UINT16 TOPIC_SGP = INVALID_TOPIC;
UINT16 TOPIC_VIDEO = INVALID_TOPIC;
UINT16 TOPIC_INPUT = INVALID_TOPIC;
UINT16 TOPIC_STACK_CONTAINERS = INVALID_TOPIC;
UINT16 TOPIC_LIST_CONTAINERS = INVALID_TOPIC;
UINT16 TOPIC_QUEUE_CONTAINERS = INVALID_TOPIC;
UINT16 TOPIC_PRILIST_CONTAINERS = INVALID_TOPIC;
UINT16 TOPIC_HIMAGE = INVALID_TOPIC;
UINT16 TOPIC_ORDLIST_CONTAINERS = INVALID_TOPIC;
UINT16 TOPIC_3DENGINE = INVALID_TOPIC;
UINT16 TOPIC_VIDEOOBJECT = INVALID_TOPIC;
UINT16 TOPIC_FONT_HANDLER = INVALID_TOPIC;
UINT16 TOPIC_VIDEOSURFACE = INVALID_TOPIC;
UINT16 TOPIC_MOUSE_SYSTEM = INVALID_TOPIC;
UINT16 TOPIC_BUTTON_HANDLER = INVALID_TOPIC;
UINT16 TOPIC_MUTEX = INVALID_TOPIC;
UINT16 TOPIC_JA2 = INVALID_TOPIC;
UINT16 TOPIC_BLIT_QUEUE = INVALID_TOPIC;
UINT16 TOPIC_JA2OPPLIST = INVALID_TOPIC;
UINT16 TOPIC_JA2AI = INVALID_TOPIC;

UINT32 guiTimerID = 0;
UINT8 guiDebugLevels[NUM_TOPIC_IDS]; // don't change this, Luis!!!!

// GLOBAL: WIZ8 0x006ed040
BOOLEAN gfDebugTopics[MAX_TOPICS_ALLOTED];
// GLOBAL: WIZ8 0x006ed440
UINT16* gpDbgTopicPtrs[MAX_TOPICS_ALLOTED];

// remove debug .txt file
void RemoveDebugText(void);

STRING512 gpcDebugLogFileName;

#ifdef __cplusplus
}
#endif

//**************************************************************************
//
//				Functions
//
//**************************************************************************

//**************************************************************************
//
// DbgGetLogFileName
//
//
//
// Parameter List :
// Return Value :
// Modification history :
//
//		xxjun98:CJC		-> creation
//
//**************************************************************************
BOOLEAN DbgGetLogFileName(STRING512 pcName)
{
    // use the provided buffer to get the directory name, then tack on
    // "\debug.txt"
#ifndef _NO_DEBUG_TXT
    if (!GetExecutableDirectory(pcName)) {
        return (FALSE);
    }

    if (strlen(pcName) > (512 - strlen("\\debug.txt") - 1)) {
        // no room!
        return (FALSE);
    }

    strcat(pcName, "\\debug.txt");
#endif

    return (TRUE);
}

//**************************************************************************
//
// DbgInitialize
//
//
//
// Parameter List :
// Return Value :
// Modification history :
//
//		xxnov96:HJH		-> creation
//
//**************************************************************************

// FUNCTION: WIZ8 0x00404b00
BOOLEAN DbgInitialize(void)
{
    INT32 iX;

    for (iX = 0; iX < MAX_TOPICS_ALLOTED; iX++) {
        gpDbgTopicPtrs[iX] = NULL;
    }

    DbgClearAllTopics();

    gfRecordToFile = TRUE;
    gfRecordToDebugger = TRUE;
    gubAssertString[0] = '\0';

#ifndef _NO_DEBUG_TXT
    if (!DbgGetLogFileName(gpcDebugLogFileName)) {
        return (FALSE);
    }
    // clear debug text file out
    RemoveDebugText();
#endif

    return (TRUE);
}

//**************************************************************************
//
// DbgShutdown
//
//
//
// Parameter List :
// Return Value :
// Modification history :
//
//		xxnov96:HJH		-> creation
//
//**************************************************************************

void DbgShutdown(void)
{
    DbgMessageReal((UINT16)(-1), CLIENT_SHUTDOWN, 0, "SGP Going Down");
}

//**************************************************************************
//
// DbgTopicRegistration
//
//
// Parameter List :
// Return Value :
// Modification history :
//
//		June 97: BR		-> creation
//
//**************************************************************************

void DbgTopicRegistration(UINT8 ubCmd, UINT16* usTopicID, CHAR8* zMessage)
{
    UINT16 usIndex, usUse;
    BOOLEAN fFound;

    if (usTopicID == NULL)
        return;

    if (ubCmd == TOPIC_REGISTER) {
        usUse = INVALID_TOPIC;
        fFound = FALSE;
        for (usIndex = 0; usIndex < MAX_TOPICS_ALLOTED && !fFound; usIndex++) {
            if (!gfDebugTopics[usIndex]) {
                fFound = TRUE;
                usUse = usIndex;
            }
        }

        gfDebugTopics[usUse] = TRUE;
        *usTopicID = usUse;
        gpDbgTopicPtrs[usUse] = usTopicID;
        DbgMessageReal(usUse, TOPIC_MESSAGE, DBG_LEVEL_0, zMessage);
    } else if (ubCmd == TOPIC_UNREGISTER) {
        if (*usTopicID >= MAX_TOPICS_ALLOTED)
            return;

        DbgMessageReal(*usTopicID, TOPIC_MESSAGE, DBG_LEVEL_0, zMessage);
        gfDebugTopics[*usTopicID] = FALSE;

        if (gpDbgTopicPtrs[*usTopicID] != NULL) {
            gpDbgTopicPtrs[*usTopicID] = NULL;
        }

        *usTopicID = INVALID_TOPIC;
    }
}

// *************************************************************************
// Clear the debug txt file out to prevent it from getting huge
//
//
// *************************************************************************

void RemoveDebugText(void)
{
    DeleteFile(gpcDebugLogFileName);
}

//**************************************************************************
//
// DbgClearAllTopics
//
//
// Parameter List :
// Return Value :
// Modification history :
//
//		June 97: BR		-> creation
//
//**************************************************************************

void DbgClearAllTopics(void)
{
    UINT16 usIndex;

    for (usIndex = 0; usIndex < MAX_TOPICS_ALLOTED; usIndex++) {
        gfDebugTopics[usIndex] = FALSE;
        if (gpDbgTopicPtrs[usIndex] != NULL) {
            *gpDbgTopicPtrs[usIndex] = INVALID_TOPIC;
            gpDbgTopicPtrs[usIndex] = NULL;
        }
    }
}

//**************************************************************************
//
// DbgMessageReal
//
//
//
// Parameter List :
// Return Value :
// Modification history :
//
//		xxnov96:HJH		-> creation
//
//**************************************************************************

void DbgMessageReal(UINT16 uiTopicId, UINT8 uiCommand, UINT8 uiDebugLevel, CHAR* strMessage)
{
#ifndef _NO_DEBUG_TXT
    FILE* OutFile;
#endif

    // Check for a registered topic ID
    if (uiTopicId < MAX_TOPICS_ALLOTED && gfDebugTopics[uiTopicId]) {
        OutputDebugString(strMessage);
        OutputDebugString("\n");

//add _NO_DEBUG_TXT to your SGP preprocessor definitions to avoid this f**king huge file from
//slowly growing behind the scenes!!!!
#ifndef _NO_DEBUG_TXT
        if ((OutFile = fopen(gpcDebugLogFileName, "a+t")) != NULL) {
            fprintf(OutFile, "%s\n", strMessage);
            fclose(OutFile);
        }
#endif
    }
}

//**************************************************************************
//
// DbgSetDebugLevel
//
//
//
// Parameter List :
// Return Value :
// Modification history :
//
//		11nov96:HJH		-> creation
//
//**************************************************************************

//**************************************************************************
//
// DbgFailedAssertion
//
//
//
// Parameter List :
// Return Value :
// Modification history :
//
//		xxnov96:HJH		-> creation
//
//**************************************************************************

///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// Wiz8 compatible debug messaging

void _DebugMessage(UINT8* pString, UINT32 uiLineNum, UINT8* pSourceFile)
{
    UINT8 ubOutputString[512];
#ifndef _NO_DEBUG_TXT
    FILE* DebugFile;
#endif

    //
    // Build the output string
    //

    sprintf((char*)ubOutputString, "{ %ld } %s [Line %d in %s]\n", GetTickCount(), pString,
            uiLineNum, pSourceFile);

    //
    // Output to debugger
    //

    if (gfRecordToDebugger) {
        OutputDebugString((char*)ubOutputString);
    }

    //
    // Record to file if required
    //

#ifndef _NO_DEBUG_TXT
    if (gfRecordToFile) {
        if ((DebugFile = fopen(gpcDebugLogFileName, "a+t")) != NULL) {
            fputs((char*)ubOutputString, DebugFile);
            fclose(DebugFile);
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////
// This func is used by Assert()
void _Null(void) {}

extern HVOBJECT FontObjs[25];

void _FailMessage(UINT8* pString, UINT32 uiLineNum, UINT8* pSourceFile)
{
    UINT8 ubOutputString[512];
    BOOLEAN fDone = FALSE;

#ifndef _NO_DEBUG_TXT
    FILE* DebugFile;
#endif

    // Build the output string
    sprintf((char*)ubOutputString, "{ %ld } Assertion Failure: %s [Line %d in %s]\n",
            GetTickCount(), pString, uiLineNum, pSourceFile);
    if (pString)
        sprintf((char*)gubAssertString, (char*)pString);
    // Output to debugger
    if (gfRecordToDebugger) {
        OutputDebugString((char*)ubOutputString);
        if (pString) { //tag on the assert message
            OutputDebugString((char*)gubAssertString);
        }
    }
    // Record to file if required
#ifndef _NO_DEBUG_TXT
    if (gfRecordToFile) {
        if ((DebugFile = fopen(gpcDebugLogFileName, "a+t")) != NULL) {
            fputs((char*)ubOutputString, DebugFile);
            if (pString) { //tag on the assert message
                fputs((char*)gubAssertString, DebugFile);
            }
            fclose(DebugFile);
        }
    }
#endif
    exit(0);
}

#endif

// This is NOT a _DEBUG only function! It is also needed in
// release mode builds. -- DB
// FUNCTION: WIZ8 0x00404b50
UINT8* String(const char* String, ...)
{

    va_list ArgPtr;
    UINT8 usIndex;

    // Record string index. This index is used since we live in a multitasking environment.
    // It is still not bulletproof, but it's better than a single string
    usIndex = gubStringIndex++;
    if (gubStringIndex == 8) { // reset string pointer
        gubStringIndex = 0;
    }

    va_start(ArgPtr, String);
    vsprintf((char*)gbTmpDebugString[usIndex], String, ArgPtr);
    va_end(ArgPtr);

    return gbTmpDebugString[usIndex];
}
