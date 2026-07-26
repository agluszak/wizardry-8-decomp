#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/* Engine Code\GrCycle.cpp. BEHAVIOUR_FIRST and BEHAVIOUR_LAST come from the
   canonical assertion at line 1598; the body bounds-checks against 1 and 3, so
   the enum runs 1..3. The stored-to object is whatever GrCycle's primary vtable
   slot 9 returns; only the byte it writes at +0x70 is established here.
   Slots 0..8 are declared solely to place slot 9 at vtable offset 0x24, which
   is what the canonical virtual call uses. */
#define BEHAVIOUR_FIRST 1
#define BEHAVIOUR_LAST  3

struct W8GrCycleTarget {
    unsigned char unknown_00[0x70];
    signed char m_bBehaviour;               /* 0x70 */
};

struct W8GrCycle {
    virtual void vslot0() = 0;
    virtual void vslot1() = 0;
    virtual void vslot2() = 0;
    virtual void vslot3() = 0;
    virtual void vslot4() = 0;
    virtual void vslot5() = 0;
    virtual void vslot6() = 0;
    virtual void vslot7() = 0;
    virtual void vslot8() = 0;
    virtual W8GrCycleTarget* vslot9() = 0;

    void SetBehaviour(signed char bBehaviour);
};

// FUNCTION: WIZ8 0x004A8460
void W8GrCycle::SetBehaviour(signed char bBehaviour)
{
    W8GrCycleTarget* target = vslot9();

    if (bBehaviour < BEHAVIOUR_FIRST || bBehaviour > BEHAVIOUR_LAST) {
        srAssertFail(
            "(bBehaviour >= BEHAVIOUR_FIRST) && (bBehaviour <= BEHAVIOUR_LAST)",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
            0x63e,
            0);
    }
    target->m_bBehaviour = bBehaviour;
}
