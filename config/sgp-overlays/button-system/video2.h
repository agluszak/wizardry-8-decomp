#ifndef WIZ8_SGP_VIDEO2_H
#define WIZ8_SGP_VIDEO2_H

#define InvalidateRegion SgpReleasedFourArgumentInvalidateRegion
#include "video.h"
#undef InvalidateRegion

enum { INVAL_SRC_TRANS = 1 };
void InvalidateRegion(INT32 left, INT32 top, INT32 right, INT32 bottom,
                      UINT32 flags);

#endif
