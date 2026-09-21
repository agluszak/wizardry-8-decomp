#pragma once

#include "surrender/srMath.h"

template <class T> class W8GrowableVector;

/* Engine Code\SoundEvent.cpp. The unit's assertion-backed interval starts at
   the factory below. */

/* Event-kind bits: the mls script keywords SOUND_FRAME, SOUND_CYCLE and
   SOUND_FOOTSTEP map to these values (Monster.cpp loader), and
   UpdateSoundEvents iterates the caller's mask one bit at a time, so a kind
   is both a classification and a single-bit mask. */
enum W8SoundEventKind {
    W8_SOUND_EVENT_FRAME = 0x1,
    W8_SOUND_EVENT_CYCLE = 0x2,
    W8_SOUND_EVENT_FOOTSTEP = 0x100,
};

/* One queued sound: Engine Code\SoundEvent.cpp's whole lifecycle for it. The
   original concrete class name is unknown, so this descriptive name is used
   throughout; only the factory's and assertions' `pSndEvent` and
   m_pacWaveName spellings are original. The layout below is established by the
   unit itself: the factory at 0x004D57A0 allocates 0x38 and initialises every
   field, and the destructor at 0x004D5770 releases the name and the handle.
   GrObject.cpp calls these `pse`/`m_plsSoundEvents` at its own sites. Field
   semantics come from the mls script commands in MonsterReadAllCycles
   (0x004C0300): `pitch`, `volume`, `probability` and `footstep_vol`. */
class W8SoundEvent {
public:
    /* In the class body because 0x004D57A0 expands it: VC6 at /Ob1 inlines
       only what is marked inline or defined here, and the retail factory has
       no call to a constructor. */
    W8SoundEvent()
    {
        kind = 0;
        cycle = -1;
        frame = -1;
        subcycle = 0;
        m_pacWaveName = 0;
        pitch = 0;
        sound_handle = -1;
        probability = 0x64;
        location_id = 0;
        volume_max = 0;
        volume_min = 0;
        falloff = 12500.0f;
        footstep_volume = 10;
        footstep_combat_volume = 30;
    }

    ~W8SoundEvent(); /* 0x004D5770 */

    /* Play this event at one position through the 3D sound boundary. The
       cycle is the animation cycle that triggered it; the remaining call-site
       values are accepted but not needed by this body. */
    unsigned char Play(unsigned int mask, const srVector3T<float>* position, int cycle,
                       unsigned int frame, int subcycle); /* 0x004D5A10 */

    int kind;                  /* 0x00: W8SoundEventKind bit */
    int cycle;                 /* 0x04: bound animation cycle, -1 = any */
    int frame;                 /* 0x08: bound animation frame */
    int subcycle;              /* 0x0c: bound subcycle (script value minus one) */
    char* m_pacWaveName;       /* 0x10: owned */
    int pitch;                 /* 0x14: mls `pitch` command */
    int volume_min;            /* 0x18: mls `volume` low bound */
    int volume_max;            /* 0x1c: mls `volume` high bound */
    int sound_handle;          /* 0x20: live SGP voice id, -1 when silent */
    unsigned char probability; /* 0x24: mls `probability` percent */
    unsigned char looping;     /* 0x25: script `LOOP`: loops and owns the voice */
    unsigned char unknown_026[2];
    int location_id;            /* 0x28: tracked monster's location id */
    float falloff;              /* 0x2c: audible range; mls `sound_falloff` scaled */
    int footstep_volume;        /* 0x30: base volume for step/tracked sounds */
    int footstep_combat_volume; /* 0x34: raised volume while the tracked monster fights */
};

static_assert(sizeof(W8SoundEvent) == 0x38, "W8SoundEvent_must_be_0x38");

W8SoundEvent* CreateSoundEvent(int kind, int cycle, int frame, int subcycle, const char* wave_name,
                               unsigned char looping); /* 0x004D57A0 */

unsigned char UpdateSoundEvents(W8GrowableVector<W8SoundEvent*>* events,
                                const srVector3T<float>* position, unsigned int event_mask,
                                int cycle, unsigned int frame, int subcycle); /* 0x004D5890 */

/* Reports the ground height, surface and material at one position through the
   level's game data: settles a probe with SettleToGround and reads the hit
   W8GDSurface's footstep_surface_3c/footstep_material_3d. Engine Code\
   GameData.cpp owns it; only this unit's player consumes it, so its seam
   lives here. */
float GetGroundSurfaceInfo(const srVector3T<float>* position, char* surface,
                           char* material); /* 0x00420CA0 */
