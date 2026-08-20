#ifndef WIZ8_SGP_COMPAT_VIDEO2_H
#define WIZ8_SGP_COMPAT_VIDEO2_H

/* Wizardry's unreleased video-manager surface extends released SGP video.h. */
#define InvalidateRegion SgpReleasedFourArgumentInvalidateRegion
#include "video.h"
#undef InvalidateRegion

enum {
    INVAL_SRC_TRANS = 1,
    PIXEL_DEPTH = 16,
    SCREEN_WIDTH = 640,
    SCREEN_HEIGHT = 480
};

void InvalidateRegion(INT32 left, INT32 top, INT32 right, INT32 bottom,
                      UINT32 flags);
BOOLEAN VideoIsFullScreen(void);
void VideoGetClientRect(RECT* rect);

#endif
