#pragma once

class W8Monster;
class stParticle;

#pragma pack(push, 1)
struct W8WorldCursorState {
    /* 0x00: detached with the world when the cursor hides. */
    W8Monster* monster_00;
    /* 0x04: deactivated with a zero when the cursor hides. */
    stParticle* particle_04;
    unsigned char unknown_08[0x38];
    unsigned char visible_40;
    unsigned char unknown_41[0x9f];
};
#pragma pack(pop)

static_assert(sizeof(W8WorldCursorState) == 0xe0,
              "W8WorldCursorState_size");

extern W8WorldCursorState* g_world_cursor_0065ba8c;
unsigned char IsWorldCursorVisible(void);
