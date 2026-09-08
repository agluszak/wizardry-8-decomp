#pragma once

#pragma pack(push, 1)
struct W8WorldCursorState {
    unsigned char unknown_00[0x40];
    unsigned char visible_40;
    unsigned char unknown_41[0x9f];
};
#pragma pack(pop)

static_assert(sizeof(W8WorldCursorState) == 0xe0,
              "W8WorldCursorState_size");

extern W8WorldCursorState* g_world_cursor_0065ba8c;
unsigned char IsWorldCursorVisible(void);
