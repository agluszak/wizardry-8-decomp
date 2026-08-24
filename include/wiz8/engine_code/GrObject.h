#pragma once

/* Engine Code\GrObject.cpp owns these declarations. GrObject.cpp, Navigator.cpp
   and GrCycle.cpp have separate direct headers. */

#include "wiz8/vector.h"

class W8AnimRepBase005EC1D8;

/* The sound events GrObject.cpp calls `pse`/`m_plsSoundEvents`. The class
   itself belongs to Engine Code\SoundEvent.cpp, which owns its whole
   lifecycle; only the pointer is needed here. */
class W8VectorElement005ED094;

/* GrObject.cpp owns this base.  The original Item.cpp assertion
   `pMissile->GrObject::GetAI()` independently establishes the class name. */
class W8GrObject {
public:
    W8GrObject();                         /* 0x004B6900 */
    W8GrObject(const W8GrObject& other); /* 0x004B69A0 */
    virtual ~W8GrObject();                /* 0x004B6B60 */

    unsigned char AddSoundEvent(W8VectorElement005ED094* event);

public:
    unsigned char unknown_004;           /* 0x04 */
    unsigned char unknown_005[3];
    int unknown_008;                     /* 0x08 */
    void* m_pAI;                         /* 0x0c: GrObject::GetAI() assertion */
    W8GrowableVector<W8VectorElement005ED094*>* m_plsSoundEvents; /* 0x10 */
    /* +0x14 remains deliberately uninitialized and unowned by this base.
       Prop and Item construct and destroy their own representation here; both
       concrete payloads derive from this polymorphic animation root. */
    W8AnimRepBase005EC1D8* m_pRep;       /* 0x14 */
};                                      /* 0x18 */
