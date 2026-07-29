#pragma once

#include "surrender/srMath.h"
#include "surrender/srNode.h"
#include "wiz8/grcycle.h"

struct W8World;
struct W8Position;

/* Engine Code\Item.cpp. The assertion expressions establish the original
   m_pRep and m_psrMesh names; the bodies establish their offsets. */
/* This is the live world entity the item manager reaches through an item's
   owner, so the position and flag members the manager uses live here too. */
struct W8ItemRep {
    virtual ~W8ItemRep();
    srVector3T<float> position;           /* 0x04 */
    unsigned char unknown_10[0x54];
    srNode* m_psrMesh;                    /* 0x64 */
    unsigned char unknown_68[0x28];
    unsigned int flags;                   /* 0x90 */

    void GetLocation004B8890(srVector3T<float>* location);
    void GetRotation004B88F0(srMatrix3T<float>* rotation);
    void SetLocation004B8850(const W8Position* location);
    unsigned int SetFlags(unsigned int mask, unsigned char enabled); /* 0x0049F310 */
};

struct W8Item : public W8GrCycleBase004B6900 {
    virtual ~W8Item() override;

    int value_018;
    int value_01c;

    void DetachMesh0049FA30(W8World* world);
    void ApplyRepTransform0049FAA0();
    void Function49F720(const W8Position* location);
    srNode* GetMesh();
};

/* Both sizes are the last proven member plus its width, not a proven extent. */
static_assert(sizeof(W8ItemRep) == 0x94, "W8ItemRep_must_be_0x94");
static_assert(sizeof(W8Item) == 0x20, "W8Item_must_be_0x20");
