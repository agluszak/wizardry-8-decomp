/* Modified for the Wizardry 8 reconstruction, 2026-10-03.
   Reconstruct Wizardry startup, shared shutdown, and fatal-error handling.
   Collapse the released JA2, utility, and precompiled-header branches to the Wizardry build.
   Drop the unused exception-handling include.
   Remove released functions that are neither retained in the Wizardry 8 retail image nor referenced by retained code.
   Keep gfGameInitialized set across shutdown, as retail does.
   Annotate the retail addresses of the startup and shutdown statics.
   Distributed under the accompanying SFI Source Code license agreement. */
#include "types.h"
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "sgp.h"
#include "RegInst.h"
#include "vobject.h"
#include "font.h"
#include "local.h"
#include "Fileman.h"
#include "input.h"
#include "Random.h"
#include "gameloop.h"
#include "soundman.h"
#include "Button System.h"
#include "mousesystem.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/sgp_bridge.h"       // for MoveTimer() [Wizardry specific]

#include "input.h"
#include "zmouse.h"




#include "dbt.h"


#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif



// Prototype Declarations




// Should the game immediately load the quick save at startup?
// GLOBAL: WIZ8 0x006505a0
BOOLEAN						gfLoadAtStartup=FALSE;
// GLOBAL: WIZ8 0x006505a1
BOOLEAN						gfUsingBoundsChecker=FALSE;
// GLOBAL: WIZ8 0x006505a4
CHAR8						*gzStringDataOverride=NULL;
// GLOBAL: WIZ8 0x006505a8
BOOLEAN						gfCapturingVideo = FALSE;


// GLOBAL: WIZ8 0x006f062c
HINSTANCE					ghInstance;



// Global Variable Declarations
#ifdef WINDOWED_MODE
RECT				rcWindow;
#endif

// moved from header file: 24mar98:HJH
// GLOBAL: WIZ8 0x006f0624
UINT32		giStartMem;

// GLOBAL: WIZ8 0x006f0620
UINT32		guiMouseWheelMsg;			// For mouse wheel messages

// GLOBAL: WIZ8 0x006f0630
BOOLEAN gfApplicationActive;
// GLOBAL: WIZ8 0x006f0628
BOOLEAN gfProgramIsRunning;
// GLOBAL: WIZ8 0x006505a9
BOOLEAN gfGameInitialized = FALSE;
// GLOBAL: WIZ8 0x006505aa
BOOLEAN	gfDontUseDDBlits	= FALSE;

// There were TWO of them??!?! -- DB
//CHAR8		gzCommandLine[ 100 ];
CHAR8		gzCommandLine[100];		// Command line given

// GLOBAL: WIZ8 0x006505ac
CHAR8		gzErrorMsg[2048]="";
// GLOBAL: WIZ8 0x00650dac
BOOLEAN	gfIgnoreMessages=FALSE;

// GLOBAL VARIBLE, SET TO DEFAULT BUT CAN BE CHANGED BY THE GAME IF INIT FILE READ
// GLOBAL: WIZ8 0x005ff450
UINT8		gbPixelDepth = PIXEL_DEPTH;

// FUNCTION: WIZ8 0x004011e0
INT32 FAR PASCAL WindowProcedure(HWND hWindow, UINT16 Message, WPARAM wParam, LPARAM lParam)
{
	// GLOBAL: WIZ8 0x00650db0
	static int fRestore = FALSE;

  if(gfIgnoreMessages)
		return(DefWindowProc(hWindow, Message, wParam, lParam));

	// ATE: This is for older win95 or NT 3.51 to get MOUSE_WHEEL Messages
	if ( Message == guiMouseWheelMsg )
	{
      QueueEvent(MOUSE_WHEEL, wParam, lParam);
			return( 0L );
	}

	switch(Message)
  {
		case WM_MOUSEWHEEL:
			{
				QueueEvent(MOUSE_WHEEL, wParam, lParam);
				break;
			}

		case WM_MOUSEMOVE:
			break;

		case WM_SIZING:
		{
			LPRECT	lpWindow;
			INT32		iWidth, iHeight, iX, iY;
			BOOLEAN fWidthByHeight=FALSE, fHoldRight=FALSE;

			lpWindow = (LPRECT) lParam;

			iWidth=lpWindow->right-lpWindow->left;
			iHeight=lpWindow->bottom-lpWindow->top;
			iX = (lpWindow->left + lpWindow->right)/2;
			iY = (lpWindow->top + lpWindow->bottom)/2;

			switch(wParam)
			{
				case WMSZ_BOTTOMLEFT:
					fHoldRight=TRUE;
				case WMSZ_BOTTOM:
				case WMSZ_BOTTOMRIGHT:
					if(iHeight < SCREEN_HEIGHT)
					{
						lpWindow->bottom=lpWindow->top+SCREEN_HEIGHT;
						iHeight=SCREEN_HEIGHT;
					}
					fWidthByHeight=TRUE;
				break;

				case WMSZ_TOPLEFT:
					fHoldRight=TRUE;
				case WMSZ_TOP:
				case WMSZ_TOPRIGHT:
					if(iHeight < SCREEN_HEIGHT)
					{
						lpWindow->top=lpWindow->bottom-SCREEN_HEIGHT;
						iHeight=SCREEN_HEIGHT;
					}
					fWidthByHeight=TRUE;
					break;

				case WMSZ_LEFT:
					if(iWidth < SCREEN_WIDTH)
					{
						lpWindow->left=lpWindow->right-SCREEN_WIDTH;
						iWidth = SCREEN_WIDTH;
					}
					break;

				case WMSZ_RIGHT:
					if(iWidth < SCREEN_WIDTH)
					{
						lpWindow->right=lpWindow->left+SCREEN_WIDTH;
						iWidth = SCREEN_WIDTH;
					}
			}

			// Calculate width as a factor of height
			if(fWidthByHeight)
			{
				iWidth = iHeight * SCREEN_WIDTH / SCREEN_HEIGHT;
//				lpWindow->left = iX - iWidth/2;
//				lpWindow->right = iX + iWidth / 2;
				if(fHoldRight)
					lpWindow->left = lpWindow->right - iWidth;
				else
					lpWindow->right = lpWindow->left + iWidth;
			}
			else // Calculate height as a factor of width
			{
				iHeight = iWidth * SCREEN_HEIGHT / SCREEN_WIDTH;
//				lpWindow->top = iY - iHeight/2;
//				lpWindow->bottom = iY + iHeight/2;
				lpWindow->bottom = lpWindow->top + iHeight;
			}


/*
			switch(wParam)
			{
				case WMSZ_BOTTOM:
				case WMSZ_BOTTOMLEFT:
				case WMSZ_BOTTOMRIGHT:
					if(iHeight < SCREEN_HEIGHT)
						lpWindow->bottom=lpWindow->top+SCREEN_HEIGHT;
			}

			switch(wParam)
			{
				case WMSZ_TOP:
				case WMSZ_TOPLEFT:
				case WMSZ_TOPRIGHT:
					if(iHeight < SCREEN_HEIGHT)
						lpWindow->top=lpWindow->bottom-SCREEN_HEIGHT;
			}

			switch(wParam)
			{
				case WMSZ_BOTTOMLEFT:
				case WMSZ_LEFT:
				case WMSZ_TOPLEFT:
					if(iWidth < SCREEN_WIDTH)
						lpWindow->left=lpWindow->right-SCREEN_WIDTH;
			}

			switch(wParam)
			{
				case WMSZ_BOTTOMRIGHT:
				case WMSZ_RIGHT:
				case WMSZ_TOPRIGHT:
					if(iWidth < SCREEN_WIDTH)
						lpWindow->right=lpWindow->left+SCREEN_WIDTH;
			}
*/
		}
		break;

    case WM_SIZE:
		{
			UINT16 nWidth = LOWORD(lParam);  // width of client area
			UINT16 nHeight = HIWORD(lParam); // height of client area

			if(nWidth && nHeight)
			{
				switch(wParam)
				{
					case SIZE_MAXIMIZED:
						VideoFullScreen(TRUE);
						break;

					case SIZE_RESTORED:
						VideoResizeWindow();
						break;
				}
			}
		}
		break;

    case WM_MOVE:
		{
			INT32 xPos = (INT32)LOWORD(lParam);    // horizontal position
			INT32 yPos = (INT32)HIWORD(lParam);    // vertical position
		}
		break;

    case WM_ACTIVATEAPP:
      switch(wParam)
      {
        case TRUE: // We are restarting DirectDraw
          if (fRestore == TRUE)
          {
						if(!VideoInspectorIsEnabled())
						{
	            RestoreVideoManager();
		          RestoreVideoSurfaces();	// Restore any video surfaces
						}

	          MoveTimer(TIMER_RESUME);
            gfApplicationActive = TRUE;
          }
          break;
        case FALSE: // We are suspending direct draw
						if(!VideoInspectorIsEnabled())
							SuspendVideoManager();
          // suspend movement timer, to prevent timer crash if delay becomes long
          // * it doesn't matter whether the 3-D engine is actually running or not, or if it's even been initialized
          // * restore is automatic, no need to do anything on reactivation
          MoveTimer(TIMER_SUSPEND);

          gfApplicationActive = FALSE;
          fRestore = TRUE;
          break;
      }
      break;

    case WM_CREATE:
			break;

    case WM_DESTROY:
			ShutdownStandardGamingPlatform();
      ShowCursor(TRUE);
      PostQuitMessage(0);
      break;

		case WM_SETFOCUS:
			if(!VideoInspectorIsEnabled())
				RestoreVideoManager();
			gfApplicationActive=TRUE;
//			RestrictMouseToXYXY(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

			break;

		case WM_KILLFOCUS:
			if(!VideoInspectorIsEnabled())
				SuspendVideoManager();

			gfApplicationActive=FALSE;
			FreeMouseCursor();
			// Set a flag to restore surfaces once a WM_ACTIVEATEAPP is received
			fRestore = TRUE;
			break;



    default
    : return DefWindowProc(hWindow, Message, wParam, lParam);
  }
  return 0L;
}



// FUNCTION: WIZ8 0x00401570
BOOLEAN InitializeStandardGamingPlatform(HINSTANCE hInstance, int sCommandShow)
{
	FontTranslationTable *pFontTable;

	// now required by all (even JA2) in order to call ShutdownSGP
	atexit(SGPExit);

	// First, initialize the registry keys.
	InitializeRegistryKeys( "Wizardry8", "Wizardry8key" );

	// For rendering DLLs etc.
	AddSubdirectoryToPath("DLL");

	// Second, read in settings
	GetRuntimeSettings( );

	// Initialize the Debug Manager - success doesn't matter
	InitializeDebugManager();

	// Now start up everything else.
	RegisterDebugTopic(TOPIC_SGP, "Standard Gaming Platform");

  // this one needs to go ahead of all others (except Debug), for MemDebugCounter to work right...
	FastDebugMsg("Initializing Memory Manager");
	// Initialize the Memory Manager
	if (InitializeMemoryManager() == FALSE)
	{ // We were unable to initialize the memory manager
		FastDebugMsg("FAILED : Initializing Memory Manager");
		return FALSE;
	}


	FastDebugMsg("Initializing File Manager");
	// Initialize the File Manager
	if (InitializeFileManager(NULL) == FALSE)
	{ // We were unable to initialize the file manager
		FastDebugMsg("FAILED : Initializing File Manager");
		return FALSE;
	}

	FastDebugMsg("Initializing Containers Manager");
  InitializeContainers();

	FastDebugMsg("Initializing Input Manager");
	// Initialize the Input Manager
	if (InitializeInputManager() == FALSE)
	{ // We were unable to initialize the input manager
		FastDebugMsg("FAILED : Initializing Input Manager");
		return FALSE;
	}

	FastDebugMsg("Initializing Video Manager");
	// Initialize DirectDraw (DirectX 2)
	if (InitializeVideoManager(hInstance, (UINT16) sCommandShow, (void *) WindowProcedure) == FALSE)
	{ // We were unable to initialize the video manager
		FastDebugMsg("FAILED : Initializing Video Manager");
		return FALSE;
	}

	// Initialize Video Object Manager
	FastDebugMsg("Initializing Video Object Manager");
	if ( !InitializeVideoObjectManager( ) )
	{
		FastDebugMsg("FAILED : Initializing Video Object Manager");
		return FALSE;
	}

	// Initialize Video Surface Manager
	FastDebugMsg("Initializing Video Surface Manager");
	if ( !InitializeVideoSurfaceManager( ) )
	{
		FastDebugMsg("FAILED : Initializing Video Surface Manager");
		return FALSE;
	}


  // Make sure we start up our local clock (in milliseconds)
  // We don't need to check for a return value here since so far its always TRUE
  InitializeClockManager();  // must initialize after VideoManager, 'cause it uses ghWindow

	// Create font translation table (store in temp structure)
	pFontTable = CreateEnglishTransTable( );
	if ( pFontTable == NULL )
	{
		return( FALSE );
	}

	// Initialize Font Manager
	FastDebugMsg("Initializing the Font Manager");
	// Init the manager and copy the TransTable stuff into it.
	if ( !InitializeFontManager( 8, pFontTable ) )
	{
		FastDebugMsg("FAILED : Initializing Font Manager");
		return FALSE;
	}
	// Don't need this thing anymore, so get rid of it (but don't de-alloc the contents)
	MemFree( pFontTable );

	FastDebugMsg("Initializing Sound Manager");
	// Initialize the Sound Manager (DirectSound)
	if (InitializeSoundManager() == FALSE)
	{ // We were unable to initialize the sound manager
		FastDebugMsg("FAILED : Initializing Sound Manager");
		return FALSE;
	}

	FastDebugMsg("Initializing Random");
  // Initialize random number generator
  InitializeRandom(); // no Shutdown

	FastDebugMsg("Initializing Game Manager");
	// Initialize the Game
	if (InitializeGame() == FALSE)
	{ // We were unable to initialize the game
		FastDebugMsg("FAILED : Initializing Game Manager");
		return FALSE;
	}

	// Register mouse wheel message
	guiMouseWheelMsg = RegisterWindowMessage( MSH_MOUSEWHEEL );

	gfGameInitialized = TRUE;

	return TRUE;
}


void ShutdownStandardGamingPlatform(void)
{
	// GLOBAL: WIZ8 0x00650db4
	static BOOLEAN Reenter = FALSE;

	//
	// Prevent multiple reentry into this function
	//

	if (Reenter == FALSE)
	{
		Reenter = TRUE;
	}
	else
	{
		return;
	}

	//
	// Shut down the different components of the SGP
	//


	if (gfGameInitialized)
	{
		ShutdownGame();
	}


	ShutdownButtonSystem();
	MSYS_Shutdown();

  ShutdownSoundManager();

	DestroyEnglishTransTable( );    // has to go before ShutdownFontManager()
  ShutdownFontManager();

  ShutdownClockManager();   // must shutdown before VideoManager, 'cause it uses ghWindow

#ifdef SGP_VIDEO_DEBUGGING
	PerformVideoInfoDumpIntoFile( "SGPVideoShutdownDump.txt", FALSE );
#endif

	ShutdownVideoSurfaceManager();
  ShutdownVideoObjectManager();
  ShutdownVideoManager();

  ShutdownInputManager();
  ShutdownContainers();
  ShutdownFileManager();

#ifdef EXTREME_MEMORY_DEBUGGING
	DumpMemoryInfoIntoFile( "ExtremeMemoryDump.txt", FALSE );
#endif

  ShutdownMemoryManager();  // must go last (except for Debug), for MemDebugCounter to work right...

	//
  // Make sure we unregister the last remaining debug topic before shutting
  // down the debugging layer
  UnRegisterDebugTopic(TOPIC_SGP, "Standard Gaming Platform");

  ShutdownDebugManager();
}


// FUNCTION: WIZ8 0x00401670
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    MSG message;
    HWND existing;

    existing = FindWindowExA(NULL, NULL, "Wizardry 8", "Wizardry 8");
    if (existing) {
        SetForegroundWindow(existing);
        ShowWindow(existing, 9);
        return 0;
    }
    ghInstance = hInstance;
    ProcessCommandLine(lpCmdLine);
    giStartMem = MemGetFree() >> 10;
    if (!FileExists(VideoGetConfigFile())) {
        _spawnl(0, "3DSetup.EXE", "3DSetup.EXE", VideoGetConfigFile(), NULL);
    }
    if (!FileExists(VideoGetConfigFile())) {
        return 0;
    }
    if (!CheckCdPresent()) {
        return 0;
    }
    ShowCursor(FALSE);
    if (!InitializeStandardGamingPlatform(hInstance, nShowCmd)) {
        return 0;
    }
    gfApplicationActive = 1;
    gfProgramIsRunning = 1;
    do {
        if (PeekMessageA(&message, NULL, 0, 0, 0)) {
            if (GetMessageA(&message, NULL, 0, 0) == 0) {
                return message.wParam;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        } else if (gfApplicationActive == 0) {
            WaitMessage();
        } else {
            GameLoop();
            gfSGPInputReceived = 0;
        }
    } while (gfProgramIsRunning);
    PostQuitMessage(0);
    return message.wParam;
}

//Do not place code in between WinMain and Handled WinMain







// FUNCTION: WIZ8 0x004017f0
void SGPExit(void)
{
	// GLOBAL: WIZ8 0x00650db5
	static BOOLEAN fAlreadyExiting = FALSE;
	BOOLEAN fUnloadScreens = TRUE;


	// helps prevent heap crashes when multiple assertions occur and call us
	if ( fAlreadyExiting )
	{
		return;
	}

	fAlreadyExiting = TRUE;
	gfProgramIsRunning = FALSE;
	ShutdownSoundManager();

// Wizardry only
	if (gfGameInitialized)
	{
// ARM: if in DEBUG mode & we've ShutdownWithErrorBox, don't unload screens and release data structs to permit easier debugging
#ifdef _DEBUG
		if (gfIgnoreMessages)
		{
			fUnloadScreens = FALSE;
		}
#endif
		GameloopExit(fUnloadScreens);
	}

	ShutdownStandardGamingPlatform();
  ShowCursor(TRUE);
	if(strlen(gzErrorMsg))
  {
		MessageBox(NULL, gzErrorMsg, "Error", MB_OK | MB_ICONERROR  );
  }

	VideoDumpMemoryLeaks();

}



// FUNCTION: WIZ8 0x004018c0
void GetRuntimeSettings( )
{
	// Runtime settings - for now use INI file - later use registry
	STRING512				ExeDir;
	STRING512				INIFile;

	// Get Executable Directory
	GetExecutableDirectory( ExeDir );
	// Adjust Current Dir
	sprintf( INIFile, "%s\\sgp.ini", ExeDir );

	gbPixelDepth = GetPrivateProfileInt( "SGP", "PIXEL_DEPTH", PIXEL_DEPTH, INIFile );

}

// FUNCTION: WIZ8 0x00401920
void ShutdownWithErrorBox(const CHAR8 *pcMessage)
{
	strncpy(gzErrorMsg, pcMessage, 2047);
	gzErrorMsg[2047]='\0';
	gfIgnoreMessages=TRUE;

	exit(0);
}


// FUNCTION: WIZ8 0x00401950
void ProcessCommandLine(CHAR8 *pCommandLine)
{
CHAR8 cSeparators[]="\t =";
CHAR8	*pCopy=NULL, *pToken;

	pCopy=(CHAR8 *)MemAlloc(strlen(pCommandLine) + 1);

	Assert(pCopy);
	if(!pCopy)
		return;

	memcpy(pCopy, pCommandLine, strlen(pCommandLine)+1);

	pToken=strtok(pCopy, cSeparators);
	while(pToken)
	{
		if(!_strnicmp(pToken, "/NOSOUND", 8))
		{
			SoundEnableSound(FALSE);
		}
		else if(!_strnicmp(pToken, "/INSPECTOR", 10))
		{
			VideoInspectorEnable();
		}
		else if(!_strnicmp(pToken, "/VIDEOCFG", 9))
		{
			pToken=strtok(NULL, cSeparators);
			VideoSetConfigFile(pToken);
		}
		else if(!_strnicmp(pToken, "/LOAD", 5))
		{
			gfLoadAtStartup=TRUE;
		}
		else if(!_strnicmp(pToken, "/WINDOW", 7))
		{
			VideoFullScreen(FALSE);
		}
		else if(!_strnicmp(pToken, "/BC", 7))
		{
			gfUsingBoundsChecker = TRUE;
		}
		else if(!_strnicmp(pToken, "/CAPTURE", 7))
		{
			gfCapturingVideo = TRUE;
		}
		else if(!_strnicmp(pToken, "/NOOCT", 6))
		{
			NoOct();
		}
		else if(!_strnicmp(pToken, "/STRINGDATA", 11))
		{
			pToken=strtok(NULL, cSeparators);
			gzStringDataOverride = (CHAR8 *)MemAlloc(strlen(pToken) + 1);
			strcpy(gzStringDataOverride, pToken);
		}

		pToken=strtok(NULL, cSeparators);
	}

	MemFree(pCopy);
}




