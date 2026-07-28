#ifndef WIZ8_RECOVERED_SGP_ALL_H
#define WIZ8_RECOVERED_SGP_ALL_H

/* Explicit non-PCH surface used by the released Video Surface unit. */
#include "types.h"
#include "DirectDraw Calls.h"
#include "debug.h"
#include "himage.h"
#include "vsurface.h"
#include "vsurface_private.h"
#include "video_private.h"
#include "wcheck.h"
#include "vobject_blitters.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PIXEL_DEPTH 16

extern LPDIRECTDRAWSURFACE2 GetFrameBufferObject(void);
extern LPDIRECTDRAWSURFACE2 GetMouseBufferObject(void);
extern PTR LockFrameBuffer(UINT32* pitch);
extern void UnlockFrameBuffer(void);
extern PTR LockMouseBuffer(UINT32* pitch);
extern void UnlockMouseBuffer(void);

#endif
