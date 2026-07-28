#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include "surrender/srTimer.h"

extern srTimer* g_shared_timer_base;

/* Cleans its own argument, so it is __stdcall and not the cdecl the
   decompiler assumes. */
extern "C" int __stdcall Function4C4660(int query);

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

// FUNCTION: WIZ8 0x004C5710
bool MonsterHasPendingCycle(W8Monster* monster)
{
    return monster->m_cycles[18].runtime->pending_cycle != -1;
}

/* Cycle 17's third state byte is preserved by ActivateMonster while the live
   engine object is rebuilt, then restored into the replacement. */
// FUNCTION: WIZ8 0x004C57F0
unsigned char MonsterGetCycle17State(W8Monster* monster)
{
    return monster->m_cycles[17].bytes.state_02;
}

// FUNCTION: WIZ8 0x004C5800
void MonsterSetCycle17State(W8Monster* monster, unsigned char state)
{
    monster->m_cycles[17].bytes.state_02 = state;
}

// FUNCTION: WIZ8 0x004C5820
unsigned char MonsterGetRuntimeFlag5BC(W8Monster* monster)
{
    return monster->m_cycles[18].runtime->flag_5bc;
}

// FUNCTION: WIZ8 0x004C5840
void MonsterSetRuntimeFlag5BC(W8Monster* monster, unsigned char flag)
{
    monster->m_cycles[18].runtime->flag_5bc = flag;
}

/* Cycle 18's pointee carries the scale at +0x5f0. Both accessors reach it the
   same way - through the pointer at the cycle's +0x0c, which 0x004E60B0 also
   reads a byte from - so the pointee is a shared engine object rather than
   anything the cycle owns. It is not modelled: only this one field is known. */
// FUNCTION: WIZ8 0x004C5780
float MonsterGetScale(W8Monster* monster)
{
    return monster->m_cycles[18].runtime->scale;
}

// FUNCTION: WIZ8 0x004C57A0
void MonsterSetScale(W8Monster* monster, float scale)
{
    monster->m_cycles[18].runtime->scale = scale;
}

// FUNCTION: WIZ8 0x004C57C0
void MonsterGetScaleRange(W8Monster* monster, float* minimum, float* maximum)
{
    W8MonsterCycleRuntime* runtime = monster->m_cycles[18].runtime;

    *minimum = runtime->minimum_scale;
    *maximum = runtime->maximum_scale;
}

/* Returns the previous animation state and timestamps every update through the
   recovered shared SurRender timer. */
// FUNCTION: WIZ8 0x004C5A00
unsigned char MonsterSetAnimating(W8Monster* monster, unsigned char animating)
{
    if (monster != 0) {
        W8MonsterCycleRuntime* runtime = monster->m_cycles[18].runtime;
        unsigned char previous = runtime->animating;

        runtime->animating = animating;
        runtime->animation_timestamp =
            g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        return previous;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004C59E0
unsigned char MonsterIsAnimating(W8Monster* monster)
{
    if (monster != 0) {
        return monster->m_cycles[18].runtime->animating;
    }
    return 0;
}

/* Cycle 19 bit 5 blocks pending-cycle changes. Otherwise the request is stored
   as the signed low byte in cycle 18's runtime record. */
// FUNCTION: WIZ8 0x004C5AA0
void MonsterSetPendingCycle(W8Monster* monster, int cycle)
{
    if (monster != 0 && ((monster->m_cycles[19].flags_00 >> 5) & 1) == 0) {
        monster->m_cycles[18].runtime->pending_cycle = (signed char)cycle;
    }
}

// FUNCTION: WIZ8 0x004C5E40
void MonsterSetRuntimeBehaviour(W8Monster* monster, signed char behaviour)
{
    if (monster != 0) {
        if (behaviour < 1 || behaviour > 3) {
            srAssertFail(
                "bBehaviour >= BEHAVIOUR_FIRST && bBehaviour <= BEHAVIOUR_LAST",
                "..\\Engine Code\\Include\\AnimRep.hpp",
                0x87,
                0);
        }
        monster->m_cycles[18].runtime->behaviour = behaviour;
    }
}

// FUNCTION: WIZ8 0x004C5EE0
unsigned char MonsterHasCycle19Flag3(W8Monster* monster)
{
    if (monster != 0) {
        return (monster->m_cycles[19].flags_00 >> 3) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004C6160
void MonsterSetStateA0(W8Monster* monster, unsigned char state)
{
    if (monster != 0) {
        monster->member_18.state_a0 = state;
    }
}

/* Named by the MonsterManager assertions. A null monster answers -1 rather than
   forwarding, which is how the callers tell "no monster" from a real result. */
// FUNCTION: WIZ8 0x004C5B40
int MonsterQuery(W8Monster* monster, int query)
{
    if (monster != NULL) {
        return Function4C4660(query);
    }
    return -1;
}

// FUNCTION: WIZ8 0x004CA4C0
unsigned char W8Monster::IsDying()
{
    return Function4C4660(6) == 0x15 ||
           m_cycles[18].runtime->pending_cycle == 0x15;
}
