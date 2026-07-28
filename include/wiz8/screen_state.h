#pragma once

/* The current and pending screen records begin at the two globals whose first
   dwords the reviewed setters address directly. One storage object preserves
   that identity instead of mirroring the id into a synthetic runtime record. */
struct W8ScreenStateRuntime {
    int id;
    unsigned char rest[0x94];
};

union W8ScreenStateStorage {
    int id;
    W8ScreenStateRuntime state;
};

extern "C" W8ScreenStateStorage g_screen_state_0068ec78;
extern "C" W8ScreenStateStorage g_dword_68ed10;

typedef char W8ScreenStateRuntime_must_be_0x98[
    sizeof(W8ScreenStateRuntime) == 0x98 ? 1 : -1];
