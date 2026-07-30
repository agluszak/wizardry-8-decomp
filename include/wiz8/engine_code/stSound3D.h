#pragma once

#include "surrender/srNode.h"

class stSound3D : public srNode {
public:
    const char* getClassName() const override;     /* 0x004AF3E0 */
    unsigned long getClassID() const override;     /* 0x004AF3D0 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x004AF3F0 */
    unsigned char IsPlaying004AEC70();                   /* 0x004AEC70 */

    int unknown_138;
    int sound_handle_13c;
    int value_140;
    float value_144;
    char* sound_name_148;
    unsigned char flag_14c;
    unsigned char unknown_14d[3];
};

static_assert(sizeof(stSound3D) == 0x150, "stSound3D_must_be_0x150");
