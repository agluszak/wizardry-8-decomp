#include "wiz8/engine_code/SoundEvent.h"
#include "wiz8/sr_api.h"
#include "soundman.h"

#include <new>
#include <string.h>

#define SOUNDEVENT_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\SoundEvent.cpp"

/* Releases the two things a sound event owns: the copied wave name, and the
   playing handle, but only when the event was created with the flag that says
   it started one. */
// FUNCTION: WIZ8 0x004d5770
W8VectorElement005ED094::~W8VectorElement005ED094()
{
    if (m_pacWaveName != 0) {
        ::operator delete(m_pacWaveName);
    }
    if (flag_025 != 0 && handle_020 != -1) {
        SoundStop(handle_020);
    }
}

/* The wave name is copied into storage the event owns and its destructor
   releases. */
// FUNCTION: WIZ8 0x004d57a0
W8VectorElement005ED094* CreateSoundEvent004D57A0(
    int value_000,
    int value_004,
    int value_008,
    int value_00c,
    const char* wave_name,
    unsigned char flag_025)
{
    W8VectorElement005ED094* pSndEvent = new W8VectorElement005ED094();

    if (pSndEvent == 0) {
        srAssertFail(
            "pSndEvent", SOUNDEVENT_CPP, 0x70, "SoundEvent: Out of memory");
    }
    pSndEvent->value_000 = value_000;
    pSndEvent->value_004 = value_004;
    pSndEvent->value_008 = value_008;
    pSndEvent->value_00c = value_00c;
    pSndEvent->m_pacWaveName =
        static_cast<char*>(::operator new(strlen(wave_name) + 1));
    pSndEvent->flag_025 = flag_025;
    if (pSndEvent->m_pacWaveName == 0) {
        srAssertFail(
            "pSndEvent->m_pacWaveName", SOUNDEVENT_CPP, 0x79,
            "SoundEvent: Out of memory");
    }
    strcpy(pSndEvent->m_pacWaveName, wave_name);
    return pSndEvent;
}
