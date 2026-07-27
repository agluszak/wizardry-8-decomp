#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* Engine Code\Monster.cpp. CYCLE_NUM_UNIQUE and the method name both come from
   the canonical assertion at line 960, whose message reads
   "GetNumSubsPerCycle() -> Invalid cycle num.". The element count and stride
   agree with the reviewed constructor: 27 entries of 0x10 bytes at 0xAC ends at
   0x25C, exactly where Monster's second subobject array begins. */
// FUNCTION: WIZ8 0x004BFAB0
unsigned char W8Monster::GetNumSubsPerCycle(signed char bCycle)
{
    if (bCycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle < CYCLE_NUM_UNIQUE",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0x3c0,
            "GetNumSubsPerCycle() -> Invalid cycle num.");
    }
    if (bCycle == -1) {
        bCycle = member_18.m_bCurrentCycle;
    }
    return m_cycles[bCycle].ubNumSubs;
}

/* Cycle 18's pointee carries the scale at +0x5f0. Both accessors reach it the
   same way - through the pointer at the cycle's +0x0c, which 0x004E60B0 also
   reads a byte from - so the pointee is a shared engine object rather than
   anything the cycle owns. It is not modelled: only this one field is known. */
// FUNCTION: WIZ8 0x004C5780
float MonsterGetScale(W8Monster* monster)
{
    return *(float*)(monster->m_cycles[18].unknown_0c + 0x5f0);
}

// FUNCTION: WIZ8 0x004C57A0
void MonsterSetScale(W8Monster* monster, float scale)
{
    *(float*)(monster->m_cycles[18].unknown_0c + 0x5f0) = scale;
}
