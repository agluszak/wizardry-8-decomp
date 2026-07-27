#include "wiz8/sr_api.h"
#include "wiz8/vector.h"

#include <stdlib.h>

#define GROBJECT_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\GrObject.cpp"

/* Engine Code\GrObject.cpp. The sound-event list is the first growable vector
   whose *teardown* is recovered rather than only its construction, which is
   what makes it worth having: wiz8/vector.h records that some sites install one
   vtable and others install two, and the two-store sites were only ever proven
   from constructors. Here the whole family is present at once - the derived
   vector's deleting destructor at 0x004B6DC0, the instantiation's complete
   destructor at 0x004B6DE0, and the instantiation's own deleting destructor at
   0x004B6D90 - so the hierarchy has to explain three encodings, not one.

   What it has to explain is that 0x004B6DC0 sits in the derived class's vtable
   0x005ED094 and calls 0x004B6DE0, which stores the *base* vtable 0x005ED098.
   No store of 0x005ED094 survives anywhere in the teardown path, though the
   constructor plainly writes one. That is the derived destructor's own vptr
   store being dropped: it is immediately overwritten by the base destructor's
   store to the same address with nothing in between that could observe it, so
   the two collapse into the base's single store and the derived body becomes
   the base body. Construction cannot do that - the base constructor runs first
   and calls out to the allocator between the two stores - which is why the
   two-store shape is visible there and invisible here.

   The element type is unproven, so it is named for the vtable the image gives
   the vector, per the convention wiz8/vector.h sets out. The vector itself is
   named for the member it is assigned to, m_plsSoundEvents, which the canonical
   assertion at line 139 spells out. */

/* Destroyed by a direct call to 0x004D5770 followed by operator delete, so the
   element type has a non-virtual destructor of its own. */
class W8VectorElement005ED094 {
public:
    ~W8VectorElement005ED094();          /* 0x004D5770 */
};

class W8SoundEventVector005ED094 : public W8GrowableVector<W8VectorElement005ED094*> {
public:
    W8SoundEventVector005ED094();
    virtual ~W8SoundEventVector005ED094();
};                                       /* 0x10 */

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
    W8SoundEventVector005ED094* m_plsSoundEvents; /* 0x10 */
};                                       /* 0x14 established */

__forceinline W8SoundEventVector005ED094::W8SoundEventVector005ED094()
{
}

/* Empty, and the emptiness is the point: what the image contains for it is the
   base's body, reached by the compiler-generated deleting destructor at
   0x004B6DC0 with no vptr store of its own in between. */
// FUNCTION: WIZ8 0x004B6DE0
W8SoundEventVector005ED094::~W8SoundEventVector005ED094()
{
}

/* Creates the list on first use and appends one event to it. Only one argument
   reaches this from its three call sites, each of which builds the event with
   0x004D57A0 immediately before the call - the same pointer is both the thing
   null-checked on entry and the thing stored, which is why the decompiler
   splits it into two parameters.

   Add's own result is discarded: the growth-failure path and the success path
   both leave this returning the same value, and only a null event returns
   zero. Preserved as found. */
// FUNCTION: WIZ8 0x004B6BD0
unsigned char W8GrObject005ED090::AddSoundEvent(W8VectorElement005ED094* pse)
{
    if (!pse) {
        return 0;
    }
    if (!m_plsSoundEvents) {
        m_plsSoundEvents = new W8SoundEventVector005ED094();
        if (!m_plsSoundEvents) {
            srAssertFail("m_plsSoundEvents", GROBJECT_CPP, 0x8b, 0);
        }
    }
    m_plsSoundEvents->Add(pse);
    return 1;
}
