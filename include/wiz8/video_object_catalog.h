#ifndef WIZ8_VIDEO_OBJECT_CATALOG_H
#define WIZ8_VIDEO_OBJECT_CATALOG_H

#include "vobject.h"
#include "vsurface.h"

struct W8VideoObjectSlot {
    int first_frame;
    short y_offset;
};

enum W8VideoStorageKind { W8_VIDEO_STORAGE_OBJECT = 0, W8_VIDEO_STORAGE_SURFACE = 1 };

struct W8VideoFrame {
    char path[0x30];
    W8VideoStorageKind storage_kind;
    bool loaded;
    unsigned int handle;
};

extern W8VideoObjectSlot g_video_slots[494];
extern W8VideoFrame g_video_frames[1658];

unsigned int GetCatalogVideoObjectHandle(int object, int frame);
short GetCatalogVideoObjectYOffset(int object);
/* 0x00549420: load the catalog frame and return its HVOBJECT; optional out
   receives the slot y_offset used as ConfigureVObjButton's base frame. */
HVOBJECT GetCatalogVideoObject(int object, int frame, int* y_offset_out);
void GetCatalogImageSize(int object, int frame, int image, short* width, short* height);
/* 0x00549700: the image's own offset inside its frame, read from the video object's
   ETRLE subregion table. */
void GetCatalogImagePosition(int object, int frame, int image, short* x, short* y);
void DrawCatalogImage(UINT32 target, int object, int frame, short image, int left, int top,
                      UINT32 mode, blt_fx* effects);
void InvalidateCatalogImageRect(int object, int frame, int image, int left, int top, int flags);
void DrawCatalogImageAndInvalidate(UINT32 target, int object, int frame, int image, int left,
                                   int top, UINT32 mode, blt_fx* effects);
unsigned char BlitCatalogSurfaceRectTo16BPP(UINT32 target, int left, int top, int right, int bottom,
                                            int object, int source_x, int source_y);

void EnsureCatalogFrameLoaded(int object, int frame);
unsigned short* CopyCatalogImagePalette16BPP(int object, int frame);
/* 0x005498A0 / 0x00549950: lock a surface-backed catalog frame's pixel
   buffer (pitch out), then release that lock. */
void* LockCatalogFrameSurface(unsigned int object, unsigned int frame, long* pitch);
void UnlockCatalogFrameSurface(unsigned int object, unsigned int frame);

void ReleaseLoadedVideoFrames(void);

#endif
