#include "wiz8/sr_api.h"
#include "wiz8/vector.h"

#include <stdlib.h>

#define GROBJECT_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\GrObject.cpp"

/* Engine Code\GrObject.cpp. The element type is unproven, so it is named for
   the specialization vtable. The member name m_plsSoundEvents comes from the
   canonical assertion in this translation unit. */

/* Destroyed by a direct call to 0x004D5770 followed by operator delete, so the
   element type has a non-virtual destructor of its own. */
class W8VectorElement005ED094 {
public:
    ~W8VectorElement005ED094();          /* 0x004D5770 */
};

// VTABLE: WIZ8 0x005ed094
// class W8GrowableVector<W8VectorElement005ED094*>

class W8GrObject005ED090 {
public:
    unsigned char AddSoundEvent(W8VectorElement005ED094* pse);

    /* The destructor at 0x004B6920 belongs to this unit and is recovered -
       it frees the malloc buffer at +0x0c, destroys each list element with the
       non-virtual shape, then the list itself through its vtable - but it is
       only declared here. VC6 emits a class's vtable and destructor in the
       units that *construct* it, and nothing recovered so far constructs a
       W8GrObject005ED090, so defining the body would produce no object code to
       check it against. It lands with the constructor.

       Declared virtual because the image gives the class a vtable at
       0x005ED090, which is also what puts the members below where they are. */
    virtual ~W8GrObject005ED090();       /* 0x004B6920, not yet emitted */

protected:
    unsigned char unknown_004[0x8];
    /* Released with free, not operator delete, so this one is malloc storage
       and not a vector. */
    void* m_buffer_c;                    /* 0x0c */
    W8GrowableVector<W8VectorElement005ED094*>* m_plsSoundEvents; /* 0x10 */
};                                       /* 0x14 established */

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
unsigned char W8GrObject005ED090::AddSoundEvent(W8VectorElement005ED094* pse)
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
