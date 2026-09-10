/* Modified for the Wizardry 8 reconstruction, 2026-09-10.
   Consolidate the reconstructed platform declarations at their source owner.
   Distributed under the accompanying SFI Source Code license agreement. */
#ifndef __SGP_
#define __SGP_

#include "local.h"
#include "types.h"
#include "timer.h"
#include "debug.h"

#if defined( JA2 ) || defined( UTIL )
#include "video.h"
#else
#include "video2.h"
#endif

#ifndef JA2
#include "input.h"
#include "memman.h"
#include "fileman.h"
#include "dbman.h"
#include "soundman.h"
#include "pcx.h"
#include "line.h"
#include "gameloop.h"
#include "font.h"
#include "english.h"
#include "Mutex Manager.h"
#include "vobject.h"
#include "Random.h"
#include "shading.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern BOOLEAN		gfProgramIsRunning; // Turn this to FALSE to exit program
extern UINT32			giStartMem;
extern CHAR8			gzCommandLine[100];	// Command line given
extern UINT8			gbPixelDepth;				// GLOBAL RUN-TIME SETTINGS
extern BOOLEAN		gfDontUseDDBlits;		// GLOBAL FOR USE OF DD BLITTING

#if !defined(JA2) && !defined(UTILS)
extern BOOLEAN		gfLoadAtStartup;
extern CHAR8		*gzStringDataOverride;
extern BOOLEAN		gfUsingBoundsChecker;
extern BOOLEAN		gfCapturingVideo;

#endif

extern HINSTANCE ghInstance;
extern BOOLEAN gfApplicationActive;
extern BOOLEAN gfGameInitialized;
extern BOOLEAN gfIgnoreMessages;
extern UINT32 guiMouseWheelMsg;
extern CHAR8 gzErrorMsg[2048];

INT32 FAR PASCAL WindowProcedure(HWND window, UINT16 message, WPARAM wparam, LPARAM lparam);
BOOLEAN InitializeStandardGamingPlatform(HINSTANCE instance, int show_command);
void ShutdownStandardGamingPlatform(void);
void ProcessCommandLine(CHAR8* command_line);
void GetRuntimeSettings(void);

// function prototypes
void SGPExit(void);
void ShutdownWithErrorBox(const CHAR8 *pcMessage);

#ifdef __cplusplus
}
#endif

#endif
