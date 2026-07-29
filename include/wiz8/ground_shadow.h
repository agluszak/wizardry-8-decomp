#pragma once

#include "surrender/srGERD.h"
#include "surrender/srHeap.h"
#include "surrender/srNode.h"

/* The retained registry name, class id, two vtables, allocation size, and
   clone/factory slots identify this first-party srNode subclass. */
class stGroundShadow : public srNode {
public:
    stGroundShadow(srNode* parent);                 /* 0x004D61B0 */
    stGroundShadow(const stGroundShadow& other);    /* 0x004D6430 */

    virtual const char* getClassName() const override;       /* 0x004D69B0 */
    virtual unsigned long getClassID() const override;       /* 0x004D69A0 */
    virtual srRegistry::ClassNode* getClassNode() const override; /* 0x004D69C0 */

protected:
    virtual ~stGroundShadow() override;                      /* 0x004D6370 */

public:
    virtual srClass* vInstance() override;                   /* 0x004D6BF0 */
    virtual srNode* vslot7() override;                       /* 0x004D6A30 */
    virtual void traverse(TraverseInfo& info) override;      /* 0x004D6540 */
    virtual void process(
        const ProcessInfo& info, e_processType type) override; /* 0x004D6640 */

    void* operator new(unsigned int size)
    {
        return srHeap.allocate(size);
    }

    int field_138;
    int value_13c;
    int value_140;
    unsigned char unknown_144[4];

private:
    void renderGroundShadow(srGERD* renderer);      /* 0x004D66A0 */
};

static_assert((sizeof(stGroundShadow) == 0x148), "stGroundShadow_must_be_0x148");

extern "C" char g_stGroundShadowClassName[];
