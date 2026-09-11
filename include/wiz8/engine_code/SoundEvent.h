#pragma once

#include "surrender/srMath.h"

template <class T>
class W8GrowableVector;

/* Engine Code\SoundEvent.cpp. The unit's assertion-backed interval starts at
   the factory below. */

/* One queued sound: Engine Code\SoundEvent.cpp's whole lifecycle for it. The
   original concrete class name is unknown, so this descriptive name is used
   throughout; only the factory's and assertions' `pSndEvent` and
   m_pacWaveName spellings are original. The layout below is established by the
   unit itself: the factory at 0x004D57A0 allocates 0x38 and initialises every
   field, and the destructor at 0x004D5770 releases the name and the handle.
   GrObject.cpp calls these `pse`/`m_plsSoundEvents` at its own sites. */
class W8SoundEvent {
public:
    /* In the class body because 0x004D57A0 expands it: VC6 at /Ob1 inlines
       only what is marked inline or defined here, and the retail factory has
       no call to a constructor. */
    W8SoundEvent()
    {
        value_000 = 0;
        value_004 = -1;
        value_008 = -1;
        value_00c = 0;
        m_pacWaveName = 0;
        value_014 = 0;
        handle_020 = -1;
        value_024 = 0x64;
        value_028 = 0;
        value_01c = 0;
        value_018 = 0;
        value_02c = 0x46435000;
        value_030 = 10;
        value_034 = 30;
    }

    ~W8SoundEvent();                     /* 0x004D5770 */

    /* Play this event at one position through the 3D sound boundary. The
       cycle is the animation cycle that triggered it; the remaining call-site
       values are accepted but not needed by this body. */
    unsigned char Play004D5A10(
        unsigned int mask,
        const srVector3T<float>* position,
        int cycle,
        unsigned int frame,
        int subcycle);

    int value_000;                       /* 0x00 */
    int value_004;                       /* 0x04: starts -1 */
    int value_008;                       /* 0x08: starts -1 */
    int value_00c;                       /* 0x0c */
    char* m_pacWaveName;                 /* 0x10: owned */
    int value_014;                       /* 0x14 */
    int value_018;                       /* 0x18 */
    int value_01c;                       /* 0x1c */
    int handle_020;                      /* 0x20: starts -1, released when flag_025 */
    unsigned char value_024;             /* 0x24: starts 0x64 */
    unsigned char flag_025;              /* 0x25 */
    unsigned char unknown_026[2];
    int value_028;                       /* 0x28 */
    unsigned int value_02c;              /* 0x2c: starts 0x46435000 */
    int value_030;                       /* 0x30: starts 10 */
    int value_034;                       /* 0x34: starts 30 */
};

static_assert(sizeof(W8SoundEvent) == 0x38,
              "W8SoundEvent_must_be_0x38");

W8SoundEvent* CreateSoundEvent004D57A0(
    int value_000,
    int value_004,
    int value_008,
    int value_00c,
    const char* wave_name,
    unsigned char flag_025);

unsigned char UpdateSoundEvents004D5890(
    W8GrowableVector<W8SoundEvent*>* events,
    const srVector3T<float>* position,
    unsigned int event_mask,
    int cycle,
    unsigned int frame,
    int subcycle);

/* Reports the ground surface and material at one position. Its original
   translation unit is not yet attributed; only this unit's player consumes
   it, so its seam lives here until that owner is established. */
float Function420CA0(
    const srVector3T<float>* position, char* surface, char* material);

