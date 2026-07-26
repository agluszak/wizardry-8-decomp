#ifndef WIZ8_RECOVERED_SGP_ALL_H
#define WIZ8_RECOVERED_SGP_ALL_H

/* Minimal product PCH surface required by DirectDraw Calls.c. */
#include "types.h"
#include "DirectX Common.h"
#include "DirectDraw Calls.h"
#include <ddraw.h>
#include "debug.h"
#include "video_private.h"
#include <string.h>

/* Declared by sgp.h in JA2; the unreleased Wizardry PCH exposed it too. */
extern BOOLEAN gfDontUseDDBlits;

#endif
