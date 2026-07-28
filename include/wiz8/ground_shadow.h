#pragma once

#include "surrender/srHeap.h"
#include "surrender/srNode.h"

/* The retained registry name, class id, two vtables, allocation size, and
   clone/factory slots identify this first-party srNode subclass. */
class stGroundShadow : public srNode {
public:
    stGroundShadow(srNode* parent);                 /* 0x004D61B0 */

    virtual const char* getClassName() const;       /* 0x004D69B0 */
    virtual unsigned long getClassID() const;       /* 0x004D69A0 */
    virtual srRegistry::ClassNode* getClassNode() const; /* 0x004D69C0 */

protected:
    virtual ~stGroundShadow();                      /* 0x004D6370 */

public:
    virtual srClass* vInstance();                   /* 0x004D6BF0 */
    virtual srNode* vslot7();                       /* 0x004D6A30 */

    void* operator new(unsigned int size)
    {
        return srHeap.allocate(size);
    }

    int field_138;
    int value_13c;
    int value_140;
    unsigned char unknown_144[4];
};

typedef char stGroundShadow_must_be_0x148[
    (sizeof(stGroundShadow) == 0x148) ? 1 : -1];

extern "C" char g_stGroundShadowClassName[];
