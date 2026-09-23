#pragma once

#include "surrender/srNode.h"

/* Engine Code\ReadLevel.cpp. The allocation sites at 0x0044F2A1 and
   0x004BA3E3 establish the complete size. The constructor/destructor pair
   registers the object as class 0x10007 and proves srNode as its base. */
// VTABLE: WIZ8 0x005ED0F0 stLevel
// VTABLE: WIZ8 0x005ED124 srClassSupport<stLevel, srNode, 0, 65543>
class stLevel : public srClassSupport<stLevel, srNode, false, 0x10007> {
public:
    static const char* sGetClassName()
    {
        return "stLevel";
    }

    explicit stLevel(srNode* parent);
    virtual ~stLevel() override;
    virtual srClass* vInstance() override;                                      /* 0x004BA3D0 */
    virtual void traverse(TraverseInfo& info) override;                         /* 0x004BA0E0 */
    virtual void process(const ProcessInfo& info, e_processType type) override; /* 0x004B9DD0 */

    unsigned long m_active;         /* 0x138; renderer exclusion mask */
    unsigned long m_positional_13c; /* 0x13c; submitted polygon count */
};

static_assert(sizeof(stLevel) == 0x140, "stLevel_must_be_0x140");
