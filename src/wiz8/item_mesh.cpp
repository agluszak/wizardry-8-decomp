#include "gameplay_boundaries.h"

/* Engine Code\Item.cpp. Both member names come from the canonical assertion
   expression "m_pRep->m_psrMesh" at line 614; the offsets come from the bodies
   that dereference them. The psr prefix is the original's Hungarian coding for
   a SurRender object. */
struct W8ItemRep {
    unsigned char unknown_00[0x64];
    void* m_psrMesh;                        /* 0x64 */
};

struct W8Item {
    unsigned char unknown_00[0x14];
    W8ItemRep* m_pRep;                      /* 0x14 */

    void* GetMesh();
};

// FUNCTION: WIZ8 0x0049FB20
void* W8Item::GetMesh()
{
    return m_pRep->m_psrMesh;
}
