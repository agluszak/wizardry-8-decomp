#include "wiz8/bink_video.h"

#include "DirectDraw Calls.h"

#include <stddef.h>
#include <string.h>

typedef char BINK_FrameRects_at_0x34[
    offsetof(BINK, FrameRects) == 0x34 ? 1 : -1];

extern "C" {

extern int GetMilesDigitalDriver0040A8A0(void);
extern void* LockPrimarySurface(int* pitch);
extern void UnlockPrimarySurface(void);
extern void MarkScreenRectDirty(int left, int top, int right, int bottom, int flags);
extern void NoOp(int result, int line, const char* source);

// FUNCTION: WIZ8 0x005e2f90
W8BinkVideo::W8BinkVideo()
    : m_handle(0), m_value_04(0)
{
}

// FUNCTION: WIZ8 0x005e2fa0
W8BinkVideo::~W8BinkVideo()
{
    if (m_handle != 0) {
        BinkSetVolume(m_handle, 0);
        BinkPause(m_handle, 1);
        Sleep(500);
        BinkClose(m_handle);
    }
}

// FUNCTION: WIZ8 0x005e2fe0
unsigned char W8BinkVideo::Open(const char* path, int flags)
{
    BinkSetSoundSystem(BinkOpenMiles, GetMilesDigitalDriver0040A8A0());
    m_handle = BinkOpen(path, flags);
    return m_handle != 0;
}

// FUNCTION: WIZ8 0x005e3020
unsigned char W8BinkVideo::UpdateFrame()
{
    if (m_handle != 0 && !BinkWait(m_handle)) {
        BinkDoFrame(m_handle);
        if (m_target != 0) {
            CopyFrameToTargetSurface();
        } else {
            CopyFrameToPrimarySurface();
        }
        if (m_handle->FrameNum < m_handle->Frames - 1) {
            BinkNextFrame(m_handle);
        } else {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005e3070
unsigned char W8BinkVideo::CopyFrameToPrimarySurface()
{
    int pitch;
    void* pixels = LockPrimarySurface(&pitch);
    if (pixels == 0) {
        return 0;
    }

    BinkCopyToBuffer(
        m_handle, pixels, pitch, 0x1e0, 0, 0, BINKSURFACE555);
    UnlockPrimarySurface();
    int count = BinkGetRects(m_handle, 0);
    for (int index = 0; index < count; ++index) {
        const BINKRECT& rect = m_handle->FrameRects[index];
        MarkScreenRectDirty(rect.Left, rect.Top,
                            rect.Left + rect.Width, rect.Top + rect.Height, 0);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005e3100
unsigned char W8BinkVideo::CopyFrameToTargetSurface()
{
    if (m_target == 0) {
        return 0;
    }

    int surface_type = BinkDDSurfaceType(m_target);
    if (surface_type == -1) {
        return 0;
    }

    DDSURFACEDESC description;
    memset(&description, 0, sizeof(description));
    description.dwSize = sizeof(description);
    HRESULT result = m_target->Lock(0, &description, DDLOCK_WAIT, 0);
    while (result == DDERR_SURFACELOST) {
        result = m_target->Restore();
        if (result != DD_OK) {
            NoOp(result, 220, "C:\\Projects\\Wizardry 8\\Engine Code\\Bink.cpp");
            return 0;
        }
        result = m_target->Lock(0, &description, DDLOCK_WAIT, 0);
    }

    BinkCopyToBuffer(m_handle, description.lpSurface, description.lPitch,
                     m_handle->Height, 0, 0, surface_type);
    m_target->Unlock(description.lpSurface);
    return 1;
}

// FUNCTION: WIZ8 0x005e31e0
void W8BinkVideo::SetTarget(IDirectDrawSurface2* target)
{
    if (target != 0) {
        DDBLTFX effects;
        m_target = target;
        effects.dwFillColor = 0;
        DDBltSurface(target, 0, 0, 0, DDBLT_COLORFILL, &effects);
    }
}

}
