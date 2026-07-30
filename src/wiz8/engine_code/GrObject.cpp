#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "wiz8/grcycle.h"

#include <stdlib.h>

#define GROBJECT_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\GrObject.cpp"

// FUNCTION: WIZ8 0x004b6900
W8GrObject::W8GrObject()
{
    unknown_004 = 0;
    unknown_008 = -1;
    m_pAI = 0;
    m_plsSoundEvents = 0;
}

/* Engine Code\GrObject.cpp. The element type is unproven, so it is named for
   the specialization vtable. The member name m_plsSoundEvents comes from the
   canonical assertion in this translation unit. */

// VTABLE: WIZ8 0x005ed094
// class W8GrowableVector<W8VectorElement005ED094*>

// SYNTHETIC: WIZ8 0x004b6dc0
// W8GrowableVector<W8VectorElement005ED094*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004b6de0
// W8GrowableVector<W8VectorElement005ED094*>::~W8GrowableVector<W8VectorElement005ED094*>

/* Creates the list on first use and appends one event to it. Only one argument
   reaches this from its three call sites, each of which builds the event with
   0x004D57A0 immediately before the call - the same pointer is both the thing
   null-checked on entry and the thing stored, which is why the decompiler
   splits it into two parameters.

   Add's own result is discarded: the growth-failure path and the success path
   both leave this returning the same value, and only a null event returns
   zero. Preserved as found. */
// FUNCTION: WIZ8 0x004b6bd0
unsigned char W8GrObject::AddSoundEvent(W8VectorElement005ED094* pse)
{
    if (!pse) {
        return 0;
    }
    if (!m_plsSoundEvents) {
        m_plsSoundEvents = new W8GrowableVector<W8VectorElement005ED094*>();
        if (!m_plsSoundEvents) {
            srAssertFail("m_plsSoundEvents", GROBJECT_CPP, 0x8b, 0);
        }
    }
    m_plsSoundEvents->Add(pse);
    return 1;
}
