#pragma once

#include "wiz8/sgp_private.h"

/* Keep sgp.c's retained startup body, but let the function-pointer references
   bind to Wizardry's product WindowProcedure and SGPExit overrides. A
   function-like macro renames only the released definitions, not the bare
   identifiers passed to InitializeVideoManager and atexit. */
#define WindowProcedure(hWindow, Message, wParam, lParam) \
    SgpReleasedWindowProcedure(hWindow, Message, wParam, lParam)
#define SGPExit(unused) SgpReleasedSGPExit(unused)
