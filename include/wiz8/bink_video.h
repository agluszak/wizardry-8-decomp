#ifndef WIZ8_BINK_VIDEO_H
#define WIZ8_BINK_VIDEO_H

#include "bink.h"
#include "wiz8/wiz8_windows.h"

/* First-party owner around the closed Bink middleware handle. Engine
   Code\Bink.cpp is named by the retained failure path in its surface copy. */
class W8BinkVideo {
public:
    W8BinkVideo();
    ~W8BinkVideo();

    unsigned char Open(const char* path, int flags);
    unsigned char UpdateFrame();
    unsigned char CopyFrameToPrimarySurface();
    unsigned char CopyFrameToTargetSurface();
    void SetTarget(IDirectDrawSurface2* target);

private:
    HBINK m_handle;                  /* 0x00 */
    int m_value_04;                  /* 0x04: constructor clears; use unresolved */
    IDirectDrawSurface2* m_target;   /* 0x08 */
};

static_assert(sizeof(W8BinkVideo) == 0x0c, "W8BinkVideo_must_be_0x0c");

#endif
