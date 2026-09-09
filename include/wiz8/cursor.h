#ifndef WIZ8_CURSOR_H
#define WIZ8_CURSOR_H

#include "Types.h"

struct W8ScreenPoint;
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
void GetScreenPoint004284F0(W8ScreenPoint* point);
void UpdateHeldItemCursor(void);
void ClearHeldItemDisplay(void);
void SetCurrentCursor(int cursor);

#endif
