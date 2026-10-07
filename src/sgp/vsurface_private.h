/* Modified for the Wizardry 8 reconstruction: 2026-10-03, 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
#ifndef __VSURFACE_PRIVATE_
#define __VSURFACE_PRIVATE_

#include <windows.h>
#include <ddraw.h>
#include "vsurface.h"

#ifdef __cplusplus
extern "C" {
#endif

// ***********************************************************************
//
// PRIVATE, INTERNAL Header used by other SGP Internal modules
//
// Allows direct access to underlying Direct Draw Implementation
//
// ***********************************************************************

HVSURFACE CreateVideoSurfaceFromDDSurface(LPDIRECTDRAWSURFACE2 lpDDSurface);

#ifdef __cplusplus
}
#endif

#endif
