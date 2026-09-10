#ifndef WIZ8_CURSOR_H
#define WIZ8_CURSOR_H

#include "Types.h"
#include "wiz8/engine_code/Video2.h"
#include "surrender/srMath.h"

class srModelInstance;

extern srModelInstance* g_cursor_node_659694;
unsigned char InitializeMouseCursorScene(void);

extern "C" {

BOOLEAN SetMouseCursorFromVideoObject(
    UINT32 video_object, UINT16 region, INT16 offset_x, INT16 offset_y);
void BlitToMouseCursor(
    UINT32 video_object, UINT16 region, UINT16 x, UINT16 y);
void RefreshMouseCursorTexture(void);
void Function00428340(void);
extern int g_cursor_width_654ad0;
extern int g_cursor_height_654ad4;
extern int g_cursor_hotspot_x_6596bc;
extern int g_cursor_hotspot_y_6596c0;
/* input.c owns this byte; both game and automap enable queued mouse motion. */
extern BOOLEAN gfTrackMousePos;

}

void PositionMouseCursor(int x, int y, unsigned char reset_tick);
/* 0x00428520: non-zero when the current cursor hotspot is inside the
   inclusive rectangle (left, top, right, bottom). */
unsigned char IsCursorInRectangle(int left, int top, int right, int bottom);
unsigned char IsCursorInsideViewport(void);
unsigned char GetCursorPositionInViewport(srVector3T<float>* position);
void UpdateHeldItemCursor(void);
void ClearHeldItemDisplay(void);
void SetCurrentCursor(int cursor);
void Function55F160(int item_id);

#endif
