#pragma once

/* Engine Code\SoundEvent.cpp. The unit's assertion-backed interval starts at
   the factory below. */

/* GrObject.cpp calls these sound events `pse`/`m_plsSoundEvents`; no stronger
   source witness for the concrete event class name is available yet. Its whole
   lifecycle belongs to Engine Code\SoundEvent.cpp, which is what establishes
   the layout below: the factory at 0x004D57A0 allocates 0x38 and initialises
   every field, and the destructor at 0x004D5770 releases the name and the
   handle. Only m_pacWaveName is named by original source, through
   SoundEvent.cpp's `pSndEvent->m_pacWaveName` assertion. */
class W8VectorElement005ED094 {
public:
    /* In the class body because 0x004D57A0 expands it: VC6 at /Ob1 inlines
       only what is marked inline or defined here, and the retail factory has
       no call to a constructor. */
    W8VectorElement005ED094()
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

    ~W8VectorElement005ED094();          /* 0x004D5770 */

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

static_assert(sizeof(W8VectorElement005ED094) == 0x38,
              "W8VectorElement005ED094_must_be_0x38");

W8VectorElement005ED094* CreateSoundEvent004D57A0(
    int value_000,
    int value_004,
    int value_008,
    int value_00c,
    const char* wave_name,
    unsigned char flag_025);
