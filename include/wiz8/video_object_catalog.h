#ifndef WIZ8_VIDEO_OBJECT_CATALOG_H
#define WIZ8_VIDEO_OBJECT_CATALOG_H

#include "vobject.h"

#pragma pack(push, 1)
struct W8VideoObjectSlot {
    int first_frame;
    short y_offset;
    unsigned char unknown_06[2];
};

struct W8VideoFrame {
    char path[0x30];
    int mode;
    unsigned char loaded;
    unsigned char unknown_35[3];
    unsigned int handle;
};
#pragma pack(pop)

extern W8VideoObjectSlot g_video_slots_6448c8[494];
extern W8VideoFrame g_video_frames_62c430[1658];

unsigned int GetCatalogVideoObjectHandle(int object, int frame);
short GetCatalogVideoObjectYOffset(int object);
/* 0x00549420: load the catalog frame and return its HVOBJECT; optional out
   receives the slot y_offset used as ConfigureVObjButton's base frame. */
HVOBJECT GetCatalogVideoObject(int object, int frame, int* y_offset_out);
void GetCatalogImageSize(int object, int frame, int image, short* width, short* height);
/* 0x00549700: the image's own offset inside its frame, read from the locked
   surface rather than the ETRLE subregion table. */
void GetCatalogImagePosition00549700(int object, int frame, int image, short* x, short* y);
void DrawCatalogImage(int target, int object, int frame, short image, int left, int top, int mode,
                      int flags);
void InvalidateCatalogImageRect(int object, int frame, int image, int left, int top, int flags);
void DrawCatalogImageAndInvalidate(int target, int object, int frame, int image, int left, int top,
                                   int mode, int flags);
unsigned char BlitCatalogSurfaceRectTo16BPP(int target, int left, int top, int right, int bottom,
                                            int object, int source_x, int source_y);

/* 0x004039C0: find the surface's cache node, release the frame handle and
   unlink/free the node. */
unsigned char ReleaseVideoSurface(unsigned int surface);
void EnsureCatalogFrameLoaded(int object, int frame);
unsigned short* CopyCatalogImagePalette16BPP(int object, int frame);
/* 0x005498A0 / 0x00549950: lock a catalog frame's pixel buffer (pitch out)
   and release the lock (unrecovered pair). */
void* Function5498A0(unsigned int frame, unsigned int image, long* pitch);
void Function549950(unsigned int frame, unsigned int image);

void ReleaseLoadedVideoFrames(void);

#endif
