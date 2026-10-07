/* Modified for the Wizardry 8 reconstruction: 2026-10-06, 2026-10-07.
   Distributed under the accompanying SFI Source Code license agreement. */
#ifndef __VIDEO_PRIVATE_
#define __VIDEO_PRIVATE_

// ***********************************************************************
//
// PRIVATE, INTERNAL Header used by other SGP Internal modules
//
// Allows direct access to underlying Direct Draw Implementation
//
// ***********************************************************************

LPDIRECTDRAW2 GetDirectDraw2Object();
LPDIRECTDRAWSURFACE2 GetPrimarySurfaceInterface();
LPDIRECTDRAWSURFACE2 GetBackbufferInterface();

BOOLEAN SetDirectDraw2Object(LPDIRECTDRAW2 pDirectDraw);
BOOLEAN SetPrimarySurfaceInterface(LPDIRECTDRAWSURFACE2 pSurface);
BOOLEAN SetBackbufferInterface(LPDIRECTDRAWSURFACE2 pSurface);

#endif
