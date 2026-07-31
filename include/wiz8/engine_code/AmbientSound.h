#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/game_timer.h"

struct W8AmbientSoundConfig0047A790 {
    char match_name[0x80];
};

/* Engine Code\AmbientSound.cpp. The allocation and release pair establishes
   the complete 0x12c-byte object and four members used by its lifetime. */
class W8AmbientSound {
public:
    W8AmbientSound();                    /* 0x00479040 */
    ~W8AmbientSound();                   /* 0x0047A780 */

    void ApplyPosition00479350(const srVector3T<float>* position);
    void SetState00479970(int state);
    void Update0047A310();
    W8AmbientSound* FindNextMatching0047A260(
        const char* match_name, W8AmbientSound* previous);

    char* pacSoundName;                  /* 0x000: assertion-backed at 0x47A790 */
    W8AmbientSoundConfig0047A790 config_004; /* 0x004 */
    unsigned char flag_84;
    unsigned char unknown_085[3];
    srVector3T<float> vector_88;
    int value_94;
    int value_98;
    unsigned int value_9c;
    unsigned int value_a0;
    int value_a4;
    int value_a8;
    int value_ac;
    int value_b0;
    int value_b4;
    unsigned char flag_b8;
    unsigned char flag_b9;
    unsigned char unknown_0ba[2];
    int sound_handle_bc;
    int sound_handle_c0;
    unsigned char flag_c4;
    unsigned char flag_c5;
    unsigned char unknown_0c6[2];
    srVector3T<float> vector_c8;
    srVector3T<float> vector_d4;
    srVector3T<float> vector_e0;
    int value_ec;
    srVector3T<float> vector_f0;
    srVector3T<float> vector_fc;
    W8GameTimer timer_108;
};

static_assert(sizeof(W8AmbientSound) == 0x12c,
              "W8AmbientSound_must_be_0x12c");
