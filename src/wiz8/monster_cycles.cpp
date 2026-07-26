#include "gameplay_boundaries.h"

extern __declspec(dllimport) void srAssertFail(
    const char* expression,
    const char* source_path,
    int line,
    const char* message);

/* Engine Code\Monster.cpp. CYCLE_NUM_UNIQUE and the method name both come from
   the canonical assertion at line 960, whose message reads
   "GetNumSubsPerCycle() -> Invalid cycle num.". The element count and stride
   agree with the reviewed constructor: 27 entries of 0x10 bytes at 0xAC ends at
   0x25C, exactly where Monster's second subobject array begins. */
#define CYCLE_NUM_UNIQUE 27

struct W8MonsterCycle {
    unsigned char unknown_00[4];
    unsigned char ubNumSubs;                /* 0x04 */
    unsigned char unknown_05[11];
};                                          /* 0x10 */

struct W8Monster {
    unsigned char unknown_000[0xa4];
    signed char m_bCurrentCycle;            /* 0x0a4: substituted for the -1 sentinel */
    unsigned char unknown_0a5[7];
    W8MonsterCycle m_cycles[CYCLE_NUM_UNIQUE]; /* 0x0ac .. 0x25c */

    unsigned char GetNumSubsPerCycle(signed char bCycle);
};

// FUNCTION: WIZ8 0x004BFAB0
unsigned char W8Monster::GetNumSubsPerCycle(signed char bCycle)
{
    if (bCycle >= CYCLE_NUM_UNIQUE) {
        srAssertFail(
            "bCycle < CYCLE_NUM_UNIQUE",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0x3c0,
            "GetNumSubsPerCycle() -> Invalid cycle num.");
    }
    if (bCycle == -1) {
        bCycle = m_bCurrentCycle;
    }
    return m_cycles[bCycle].ubNumSubs;
}
