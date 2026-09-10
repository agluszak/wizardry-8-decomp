#pragma once

#include "surrender/srMath.h"

class W8Monster;
class stParticle;

#pragma pack(push, 1)
struct W8WorldCursorState {
    /* 0x00: detached with the world when the cursor hides. */
    W8Monster* monster_00;
    /* 0x04: deactivated with a zero when the cursor hides. */
    stParticle* particle_04;
    unsigned char unknown_08[0x20];
    /* 0x28: read back by the path-visualization update as a world point. */
    srVector3T<float> position_28;
    unsigned char unknown_34[0xc];
    unsigned char visible_40;
    unsigned char unknown_41[0x9f];
};
#pragma pack(pop)

static_assert(sizeof(W8WorldCursorState) == 0xe0,
              "W8WorldCursorState_size");

extern W8WorldCursorState* g_world_cursor_0065ba8c;
unsigned char IsWorldCursorVisible(void);
void GetWorldCursorPosition00490BF0(srVector3T<float>* position);
