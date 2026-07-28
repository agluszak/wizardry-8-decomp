#ifndef WIZ8_SGP_VIDEO2_H
#define WIZ8_SGP_VIDEO2_H

/* The released SGP sources select video2.h for Wizardry.  The public surface
   consumed by Font, vobject and himage is the same video-manager contract
   published in video.h; Wizardry's first-party adapter supplies the bodies. */
#define InvalidateRegion SgpReleasedFourArgumentInvalidateRegion
#include "video.h"
#undef InvalidateRegion

enum { INVAL_SRC_TRANS = 1 };
void InvalidateRegion(INT32 left, INT32 top, INT32 right, INT32 bottom,
                      UINT32 flags);

#endif
