#pragma once

#include "surrender/srMath.h"

class W8Monster;
class stParticle;
class srNode;
class stLight;

#pragma pack(push, 1)
struct W8WorldCursorState {
    /* 0x00: detached with the world when the cursor hides. */
    W8Monster* monster_00;
    /* 0x04: deactivated with a zero when the cursor hides. */
    stParticle* particle_04;
    unsigned char unknown_08;
    unsigned char flag_09;
    unsigned char unknown_0a[0xa];
    /* 0x14: bumped once per cursor-monster group bind. */
    int value_14;
    /* 0x18: camera-relative offset the placement update derives from
       position_28. */
    srVector3T<float> offset_18;
    /* 0x24: the light node the complete teardown removes from the world. */
    stLight* light_24;
    /* 0x28: read back by the path-visualization update as a world point. */
    srVector3T<float> position_28;
    /* 0x34: seeded to the -1e7 sentinel by the placement update. */
    srVector3T<float> value_34;
    bool visible_40;
    /* 0x41: set by the placement update after writing position_28. */
    unsigned char flag_41;
    unsigned char unknown_42[0xa];
    /* 0x4c: seeded from and restored to g_cursor_saved_value_60ab44; both
       ends only copy the whole word. */
    int value_4c;
    /* 0x50: when clear the placement update stores position_28 minus the
       camera position into offset_18. */
    unsigned char flag_50;
    unsigned char unknown_51[0x3f];
    /* 0x90: four corner offsets the target resolver ground-probes one by
       one. */
    srVector3T<float> corner_offsets_90[4];
    /* 0xc0: selects the fixed offset pair over the corner-offset walk in the
       target resolver. */
    unsigned char flag_c0;
    unsigned char unknown_c1[3];
    /* 0xc4: first fixed probe offset used when flag_c0 is set. */
    srVector3T<float> offset_c4;
    /* 0xd0: second fixed probe offset used when flag_c0 is set. */
    srVector3T<float> offset_d0;
    unsigned char unknown_dc[4];
};
#pragma pack(pop)

static_assert(sizeof(W8WorldCursorState) == 0xe0, "W8WorldCursorState_size");

extern W8WorldCursorState* g_world_cursor_0065ba8c;
/* Build the 3D cursor, light, particle and initial camera-relative bounds. */
void InitializeWorldCursor00490210(void);
bool IsWorldCursorVisible(void);
void GetWorldCursorPosition00490BF0(srVector3T<float>* position);
void SetWorldCursorNodesVisible0048ED70(unsigned char visible);
bool SelectWorldCursorNode0048EFC0(void);
int GetWorldCursorNodeCount0048ED00(void);
void HideWorldCursor00490B90(void);
void ShowWorldCursor00490B10(void);
/* 0x00490C20: copies the cursor state vector at +0x28, or zero when there is
   no cursor. */
void GetWorldCursorAnchor00490C20(srVector3T<float>* position);
void ReleaseWorldCursor004909C0(void);
void ReleaseWorldCursorNodes0048DB30(void);
