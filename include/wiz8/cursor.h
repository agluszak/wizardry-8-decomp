#ifndef WIZ8_CURSOR_H
#define WIZ8_CURSOR_H

#include "Types.h"
#include "wiz8/engine_code/Video2.h"
#include "surrender/srMath.h"

class srModelInstance;
class srScene;

extern srModelInstance* g_cursor_node_659694;
extern srScene* g_cursor_scene_659684;
unsigned char InitializeMouseCursorScene(void);

class srTextureIFace;

BOOLEAN SetMouseCursorFromVideoObject(UINT32 video_object, UINT16 region, INT16 offset_x,
                                      INT16 offset_y);
void BlitToMouseCursor(UINT32 video_object, UINT16 region, UINT16 x, UINT16 y);
void RefreshMouseCursorTexture(void);
BOOLEAN ResizeMouseCursorSurface(int width, int height);      /* 0x00427c90 */
void SetMouseCursorHotspot(short hotspot_x, short hotspot_y); /* 0x00427f00 */
void SetMouseCursorTexture(srTextureIFace* texture);          /* 0x00429170 */
void SyncSystemCursor(void);                                  /* 0x00428340 */

/* Indices into the 17-slot main-game cursor table that SetTargetCursor takes.
   The parameter stays int because retail mangles it as `H`. Only slots with
   recovered meaning are named. */
enum {
    W8_CURSOR_NONE = -1,
    W8_CURSOR_VALID_TARGET = 6,
    W8_CURSOR_INVALID_TARGET = 7,
    W8_CURSOR_TEXT_INPUT = 8,
    W8_CURSOR_MAP_LOAD = 9
};

extern int g_cursor_width_654ad0;
extern int g_cursor_height_654ad4;
extern int g_cursor_hotspot_x_6596bc;
extern int g_cursor_hotspot_y_6596c0;

void PositionMouseCursor(int x, int y, unsigned char reset_tick);
/* 0x00428520: whether the current cursor hotspot is inside the inclusive
   rectangle (left, top, right, bottom). */
bool IsCursorInRectangle(int left, int top, int right, int bottom);
bool IsCursorInsideViewport(void);
unsigned char GetCursorPositionInViewport(srVector3T<float>* position);
void UpdateHeldItemCursor(void);
void ClearHeldItemDisplay(void);
void SetItemCursor(int item_id); /* 0x0055F160 */

#endif
