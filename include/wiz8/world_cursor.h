#pragma once

#include "surrender/srMath.h"

class W8Monster;
class stParticle;
class srNode;

#pragma pack(push, 1)
struct W8WorldCursorState {
    /* 0x00: detached with the world when the cursor hides. */
    W8Monster* monster_00;
    /* 0x04: deactivated with a zero when the cursor hides. */
    stParticle* particle_04;
    unsigned char unknown_08;
    unsigned char flag_09;
    unsigned char unknown_0a[0x1a];
    /* 0x24: the light node the complete teardown removes from the world. */
    srNode* light_24;
    /* 0x28: read back by the path-visualization update as a world point. */
    srVector3T<float> position_28;
    unsigned char unknown_34[0xc];
    unsigned char visible_40;
    unsigned char unknown_41[0x0b];
    /* 0x4c: the camera-distance value restored to the shared slot when the
       cursor is destroyed. */
    float value_4c;
    unsigned char unknown_50[0x90];
};
#pragma pack(pop)

static_assert(sizeof(W8WorldCursorState) == 0xe0,
              "W8WorldCursorState_size");

extern W8WorldCursorState* g_world_cursor_0065ba8c;
bool IsWorldCursorVisible(void);
void GetWorldCursorPosition00490BF0(srVector3T<float>* position);
void SetWorldCursorNodesVisible0048ED70(unsigned char visible);
unsigned char SelectWorldCursorNode0048EFC0(void);
void HideWorldCursor00490B90(void);
void ReleaseWorldCursor004909C0(void);
void ReleaseWorldCursorNodes0048DB30(void);
