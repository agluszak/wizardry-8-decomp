#pragma once

/* Engine Code\GrObject.cpp owns these two declarations. They were split out of
   the combined grcycle.h so GrObject.cpp, Navigator.cpp and GrCycle.cpp stop
   sharing one header; grcycle.h remains as an include-only umbrella. */

#include "wiz8/vector.h"

struct W8ItemRep;

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
    W8ItemRep* m_pRep;                   /* 0x14: typed by Engine Code\Item.cpp */
};                                      /* 0x18 */
