#pragma once

#include "surrender/srNode.h"
#include "wiz8/geometry.h"
#include "wiz8/vector.h"
#include "soundman.h"

#include <stdlib.h>
#include <string.h>

class stSound3D;
extern W8GrowableVector<stSound3D*> g_sound3d_instances_65be40;

class stSound3D : public srClassSupport<stSound3D, srNode, 0, 0x1000b> {
public:
    static const char* sGetClassName() { return "stSound3D"; }
    stSound3D(const char* sound_name, srNode* parent); /* 0x004AE6D0 */
    virtual ~stSound3D() override;
    virtual srClass* vInstance() override;
    unsigned char IsPlaying004AEC70();                   /* 0x004AEC70 */
    unsigned char Play004AEBF0(unsigned char value_1, unsigned char value_2);
    void BuildSoundOptions004AECC0(
        const W8Position* listener, SOUND3DPARMS* options);

    int unknown_138;
    int sound_handle_13c;
    int value_140;
    float value_144;
    char* sound_name_148;
    unsigned char flag_14c;
    unsigned char unknown_14d[3];

    /* srClassSupport::clone expands this class-specific assignment in the
       header-owned template body at 0x004AF460. The source name is duplicated
       rather than shared, while a live sound handle is deliberately not. */
    stSound3D& operator=(const stSound3D& other)
    {
        srNode::operator=(other);

        const char* source_name = other.sound_name_148;
        if (sound_name_148 != 0) {
            free(sound_name_148);
            sound_name_148 = 0;
        }
        if (source_name != 0) {
            sound_name_148 =
                static_cast<char*>(malloc(strlen(source_name) + 1));
            strcpy(sound_name_148, source_name);
        }

        sound_handle_13c = -1;
        value_144 = other.value_144;
        value_140 = other.value_140;
        flag_14c = other.flag_14c;
        g_sound3d_instances_65be40.Add(this);
        return *this;
    }
};

static_assert(sizeof(stSound3D) == 0x150, "stSound3D_must_be_0x150");
