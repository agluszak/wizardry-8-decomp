#include "wiz8/bink_video.h"

#include "DirectDraw Calls.h"

#include <stddef.h>
#include <string.h>

struct W8BinkRect {
    int left;
    int top;
    int width;
    int height;
};

struct W8BinkHandle {
    int width;                       /* 0x00 */
    int height;                      /* 0x04 */
    unsigned int frames;             /* 0x08 */
    unsigned int frame_number;       /* 0x0c */
    unsigned char unknown_10[0x24];
    W8BinkRect dirty_rects[1];       /* 0x34 */
};

typedef char W8BinkHandle_dirty_rects_at_0x34[
    offsetof(W8BinkHandle, dirty_rects) == 0x34 ? 1 : -1];

extern "C" {

typedef void* (__stdcall *W8BinkSoundOpen)(unsigned long driver);

__declspec(dllimport) int __stdcall BinkDDSurfaceType(IDirectDrawSurface2* surface);
__declspec(dllimport) void __stdcall BinkCopyToBuffer(
    W8BinkHandle* handle, void* pixels, int pitch, int height,
    int destination_x, int destination_y, int surface_type);
__declspec(dllimport) int __stdcall BinkGetRects(W8BinkHandle* handle, int flags);
__declspec(dllimport) void __stdcall BinkPause(W8BinkHandle* handle, int paused);
__declspec(dllimport) void __stdcall BinkSetVolume(W8BinkHandle* handle, int volume);
__declspec(dllimport) void __stdcall BinkClose(W8BinkHandle* handle);
__declspec(dllimport) void __stdcall BinkSetSoundSystem(
    W8BinkSoundOpen open, unsigned long driver);
__declspec(dllimport) void* __stdcall BinkOpenMiles(unsigned long driver);
__declspec(dllimport) W8BinkHandle* __stdcall BinkOpen(const char* path, int flags);
__declspec(dllimport) void __stdcall BinkDoFrame(W8BinkHandle* handle);
__declspec(dllimport) int __stdcall BinkWait(W8BinkHandle* handle);
__declspec(dllimport) void __stdcall BinkNextFrame(W8BinkHandle* handle);

extern int GetMilesDigitalDriver0040A8A0(void);
extern void* LockPrimarySurface(int* pitch);
extern void UnlockPrimarySurface(void);
extern void MarkScreenRectDirty(int left, int top, int right, int bottom, int flags);
extern void NoOp(int result, int line, const char* source);

// FUNCTION: WIZ8 0x005E2F90
W8BinkVideo::W8BinkVideo()
    : m_handle(0), m_value_04(0)
{
}

// FUNCTION: WIZ8 0x005E2FA0
W8BinkVideo::~W8BinkVideo()
{
    if (m_handle != 0) {
        BinkSetVolume(m_handle, 0);
        BinkPause(m_handle, 1);
        Sleep(500);
        BinkClose(m_handle);
    }
}

// FUNCTION: WIZ8 0x005E2FE0
unsigned char W8BinkVideo::Open(const char* path, int flags)
{
    BinkSetSoundSystem(BinkOpenMiles, GetMilesDigitalDriver0040A8A0());
    m_handle = BinkOpen(path, flags);
    return m_handle != 0;
}

// FUNCTION: WIZ8 0x005E3020
unsigned char W8BinkVideo::UpdateFrame()
{
    if (m_handle != 0 && !BinkWait(m_handle)) {
        BinkDoFrame(m_handle);
        if (m_target != 0) {
            CopyFrameToTargetSurface();
        } else {
            CopyFrameToPrimarySurface();
        }
        if (m_handle->frame_number < m_handle->frames - 1) {
            BinkNextFrame(m_handle);
        } else {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005E3070
unsigned char W8BinkVideo::CopyFrameToPrimarySurface()
{
    int pitch;
    void* pixels = LockPrimarySurface(&pitch);
    if (pixels == 0) {
        return 0;
    }

    BinkCopyToBuffer(m_handle, pixels, pitch, 0x1e0, 0, 0, 9);
    UnlockPrimarySurface();
    int count = BinkGetRects(m_handle, 0);
    for (int index = 0; index < count; ++index) {
        const W8BinkRect& rect = m_handle->dirty_rects[index];
        MarkScreenRectDirty(rect.left, rect.top,
                            rect.left + rect.width, rect.top + rect.height, 0);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005E3100
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
                     m_handle->height, 0, 0, surface_type);
    m_target->Unlock(description.lpSurface);
    return 1;
}

// FUNCTION: WIZ8 0x005E31E0
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
