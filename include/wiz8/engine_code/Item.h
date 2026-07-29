#pragma once

#include "surrender/srMath.h"
#include "surrender/srNode.h"

struct W8World;

/* Engine Code\Item.cpp. The assertion expressions establish the original
   m_pRep and m_psrMesh names; the bodies establish their offsets. */
struct W8ItemRep {
    unsigned char unknown_00[0x64];
    srNode* m_psrMesh;                    /* 0x64 */

    void GetLocation004B8890(srVector3T<float>* location);
    void GetRotation004B88F0(srMatrix3T<float>* rotation);
};

struct W8Item {
    unsigned char unknown_00[0x14];
    W8ItemRep* m_pRep;                    /* 0x14 */

    void DetachMesh0049FA30(W8World* world);
    void ApplyRepTransform0049FAA0();
    srNode* GetMesh();
};

static_assert(sizeof(W8ItemRep) == 0x68, "W8ItemRep_must_be_0x68");
static_assert(sizeof(W8Item) == 0x18, "W8Item_must_be_0x18");
