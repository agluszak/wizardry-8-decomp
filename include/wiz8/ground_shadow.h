#pragma once

#include "surrender/srGERD.h"
#include "surrender/srNode.h"


/* The retained registry name, class id, concrete vtable, allocation size and
   clone/factory slots identify this first-party srNode subclass. */
// VTABLE: WIZ8 0x005ed3c4
class stGroundShadow
    : public srClassSupport<stGroundShadow, srNode, false, 0x10010> {
public:
    static const char* sGetClassName() { return "stGroundShadow"; }
    stGroundShadow(srNode* parent);                 /* 0x004D61B0 */
    stGroundShadow(const stGroundShadow& other);    /* 0x004D6430 */

protected:
    virtual ~stGroundShadow() override;                      /* 0x004D6370 */

public:
    virtual srClass* vInstance() override;                   /* 0x004D6BF0 */
    virtual void traverse(TraverseInfo& info) override;      /* 0x004D6540 */
    virtual void process(
        const ProcessInfo& info, e_processType type) override; /* 0x004D6640 */

    /* GrCycle's 0x004A7470 stores the navigator's facing angle here with
       fstp, which is what types it. */
    float angle_138;
    int value_13c;
    int value_140;
    unsigned char unknown_144[4];

private:
    void renderGroundShadow(srGERD* renderer);      /* 0x004D66A0 */
};

static_assert((sizeof(stGroundShadow) == 0x148), "stGroundShadow_must_be_0x148");
