#pragma once

#include "surrender/srNode.h"
#include "wiz8/geometry.h"
#include "wiz8/vector.h"
#include "soundman.h"

#include <stdlib.h>
#include <string.h>

class stSound3D;
extern W8GrowableVector<stSound3D*> g_sound3d_instances_65be40;

/* General positional-audio node, recovered in src/wiz8/engine_code/Spells.cpp.
   That placement is provisional: the original-TU evidence has a gap over its
   0x004AE6D0..0x004AEFD0 span, bounded by Spells.cpp (up to 0x004ADB20) below
   and OctBuildPreTree.cpp (from 0x004B19F0) above, so a dedicated audio
   translation unit is at least as likely; no body carries a source-path
   string to settle it. Move the bodies when the gap is attributed. */
class stSound3D : public srClassSupport<stSound3D, srNode, 0, 0x1000b> {
public:
    static const char* sGetClassName()
    {
        return "stSound3D";
    }
    stSound3D(const char* sound_name, srNode* parent); /* 0x004AE6D0 */
    virtual ~stSound3D() override;
    virtual srClass* vInstance() override;
    bool IsPlaying(); /* 0x004AEC70 */
    /* Stops the live SGP voice and clears sound_handle to -1. */
    void Stop(); /* 0x004AEC90 */
    /* loop forces the SGP voice to loop forever (AIL count 0); auto_release
       makes the update pass release the node once playback ends. */
    unsigned char Play(unsigned char loop, unsigned char auto_release); /* 0x004AEBF0 */
    void BuildSoundOptions(const srVector3T<float>* listener,
                           SOUND3DPARMS* options); /* 0x004AECC0 */

    int unknown_138;            /* 0x138: ctor zeroes it; no retail reader found */
    int sound_handle;           /* 0x13c: live SGP voice id, -1 when silent */
    int volume;                 /* 0x140: base volume before distance/effects scale */
    float falloff;              /* 0x144: audible range in world units */
    char* wave_name;            /* 0x148: owned copy of the wave filename */
    unsigned char auto_release; /* 0x14c: release the node when playback ends */
    unsigned char unknown_14d[3];

    /* srClassSupport::clone expands this class-specific assignment in the
       header-owned template body at 0x004AF460. The source name is duplicated
       rather than shared, while a live sound handle is deliberately not. */
    stSound3D& operator=(const stSound3D& other)
    {
        srNode::operator=(other);

        const char* source_name = other.wave_name;
        if (wave_name != 0) {
            free(wave_name);
            wave_name = 0;
        }
        if (source_name != 0) {
            wave_name = static_cast<char*>(malloc(strlen(source_name) + 1));
            strcpy(wave_name, source_name);
        }

        sound_handle = -1;
        falloff = other.falloff;
        volume = other.volume;
        auto_release = other.auto_release;
        g_sound3d_instances_65be40.Add(this);
        return *this;
    }
};

static_assert(sizeof(stSound3D) == 0x150, "stSound3D_must_be_0x150");
