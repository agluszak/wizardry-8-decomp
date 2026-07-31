#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "wiz8/grcycle.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/SoundEvent.h"

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

/* Nothing is shared with the source. The AI record goes through the tagged
   dispatcher, and each sound event is rebuilt from the four leading values and
   the wave name rather than pointer-copied - so the copy owns its own events
   and its own name storage.

   The representation at +0x14 is the exception: it is neither copied nor
   cleared. Whatever the allocation held stays, because the caller that copies a
   W8GrObject is the one that owns the representation. */
// FUNCTION: WIZ8 0x004b69a0
W8GrObject::W8GrObject(const W8GrObject& other)
{
    unknown_004 = other.unknown_004;
    unknown_008 = other.unknown_008;
    if (other.m_pAI != 0) {
        m_pAI = CloneAIRecord004A91C0(other.m_pAI);
    }
    else {
        m_pAI = 0;
    }
    if (other.m_plsSoundEvents != 0) {
        int count;
        int index;

        m_plsSoundEvents = new W8GrowableVector<W8VectorElement005ED094*>();
        if (m_plsSoundEvents == 0) {
            srAssertFail("m_plsSoundEvents", GROBJECT_CPP, 0x42, 0);
        }
        count = other.m_plsSoundEvents->GetCount();
        for (index = 0; index < count; ++index) {
            W8VectorElement005ED094* pse =
                *other.m_plsSoundEvents->GetAt(index);

            m_plsSoundEvents->Add(CreateSoundEvent004D57A0(
                pse->value_000, pse->value_004, pse->value_008,
                pse->value_00c, pse->m_pacWaveName, 0));
        }
    }
    else {
        m_plsSoundEvents = 0;
    }
}

/* The sound events are owned outright and destroyed here, the list with them.
   The AI record is released with plain free rather than through either PathAI
   helper, which is asymmetric with the tagged clone the copy constructor uses
   but is what the body does. */
// FUNCTION: WIZ8 0x004b6b60
W8GrObject::~W8GrObject()
{
    if (m_pAI != 0) {
        free(m_pAI);
    }
    if (m_plsSoundEvents != 0) {
        int count = m_plsSoundEvents->GetCount();
        int index;

        for (index = 0; index < count; ++index) {
            delete *m_plsSoundEvents->GetAt(index);
        }
        delete m_plsSoundEvents;
    }
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
