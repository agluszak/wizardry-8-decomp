/* Modified for the Wizardry 8 reconstruction: 2026-10-03, 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "Cursor Control.h"

#include "Video2.h"

#include "WCheck.h"
// Cursor Database

BOOLEAN gfCursorDatabaseInit = FALSE;

CursorFileData* gpCursorFileDatabase;
CursorData* gpCursorDatabase;
INT16 gsGlobalCursorYOffset = 0;
INT16 gsCurMouseOffsetX = 0;
INT16 gsCurMouseOffsetY = 0;
UINT16 gsCurMouseHeight = 0;
UINT16 gsCurMouseWidth = 0;
UINT16 gusNumDataFiles = 0;
UINT32 guiExternVo;
UINT16 gusExternVoSubIndex;
UINT32 guiExtern2Vo;
UINT16 gusExtern2VoSubIndex;
UINT32 guiOldSetCursor = 0;
UINT32 guiDelayTimer = 0;

MOUSEBLT_HOOK gMouseBltOverride = NULL;
// Cursor Handlers

BOOLEAN SetCurrentCursorFromDatabase(UINT32 uiCursorIndex)
{
    return (0);
}
