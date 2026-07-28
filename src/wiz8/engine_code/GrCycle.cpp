#include "wiz8/grcycle.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"

/* Engine Code\GrCycle.cpp. BEHAVIOUR_FIRST and BEHAVIOUR_LAST come from the
   canonical assertion at line 1598; the body bounds-checks against 1 and 3, so
   the enum runs 1..3. The stored-to object is whatever GrCycle's primary vtable
   slot 9 returns; only the byte it writes at +0x70 is established here.
   Slots 0..8 are declared solely to place slot 9 at vtable offset 0x24, which
   is what the canonical virtual call uses. */
#define BEHAVIOUR_FIRST 1
#define BEHAVIOUR_LAST  3

/* The pointer at W8GrCycle +0x1b0 owns this list. The construction path at
   0x004A8530 installs the growable-vector table 0x005ECED8 followed by the
   derived table 0x005ECED4, and the 44/30/17-byte destructor family proves
   the same empty-derived shape used by the other reviewed vector families.
   No source or debug witness names the element type, so it remains qualified
   by the derived vtable address. */
class W8VectorElement005ECED4;

class W8Vector005ECED4
    : public W8GrowableVector<W8VectorElement005ECED4*> {
public:
    W8Vector005ECED4();
    virtual ~W8Vector005ECED4();
};                                       /* 0x10 */

__forceinline W8Vector005ECED4::W8Vector005ECED4()
{
}

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

// FUNCTION: WIZ8 0x004A8530
void W8GrCycle::AddVectorElement005ECED4(W8VectorElement005ECED4* element)
{
    if (m_vector_1b0 == 0) {
        m_vector_1b0 = new W8Vector005ECED4();
    }
    m_vector_1b0->Add(element);
}

// FUNCTION: WIZ8 0x004A8F90
W8Vector005ECED4::~W8Vector005ECED4()
{
}
