#pragma once

#include <stddef.h>

#include "surrender/srMath.h"
#include "surrender/srNode.h"
#include "wiz8/engine_code/AnimRep.hpp"
#include "wiz8/engine_code/GrObject.h"

struct W8World;
class Trigger;

/* Engine Code\Item.cpp. The assertion expressions establish the original
   m_pRep and m_psrMesh names; the bodies establish their offsets. */
/* This is the live world entity the item manager reaches through an item's
   owner, so the position and flag members the manager uses live here too. */
struct W8ItemRep : public W8AnimRepBase005EC1D8 {
    virtual ~W8ItemRep() override;
    srNode* m_psrMesh; /* 0x64 */
    unsigned char unknown_68[0x28];
    unsigned int flags; /* 0x90 */

    unsigned int SetFlags(unsigned int mask, bool enabled); /* 0x0049F310 */
};

struct W8Item : public W8GrObject {
    virtual ~W8Item() override;

    Trigger* trigger_018;
    int value_01c;

    void DetachMesh0049FA30(W8World* world);
    void ApplyRepTransform0049FAA0();
    void AttachMesh0049F900(W8World* world);
    void UpdateAnimation0049F730();
    void SetLocation0049F720(const srVector3T<float>* location);
    srNode* GetMesh();
};

/* The last proven members plus their widths are not a proven object extent.
   Allocation, array stride, enclosing member, and constructor evidence for
   either complete size is still missing. */
static_assert(offsetof(W8ItemRep, m_psrMesh) == 0x64, "W8ItemRep_m_psrMesh_offset");
static_assert(offsetof(W8ItemRep, flags) == 0x90, "W8ItemRep_flags_offset");
static_assert(offsetof(W8Item, trigger_018) == 0x18, "W8Item_trigger_offset");
static_assert(offsetof(W8Item, value_01c) == 0x1c, "W8Item_value_01c_offset");

unsigned char Function49F4A0(void* context, const char* name, void* out,
                             int value); /* 0x0049F4A0 */

void GetWorldItemBounds(float* lower, float* upper); /* 0x0049FB30 */
