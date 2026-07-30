#pragma once

#include "surrender/srNode.h"

/* Engine Code\ReadLevel.cpp. The allocation sites at 0x0044F2A1 and
   0x004BA3E3 establish the complete size. The constructor/destructor pair
   registers the object as class 0x10007 and proves srNode as its base. */
class stLevel
    : public srClassSupport<stLevel, srNode, false, 0x10007> {
public:
    static const char* sGetClassName() { return "stLevel"; }

    explicit stLevel(srNode* parent);
    virtual ~stLevel() override;


    unsigned long m_active;                                  /* 0x138 */
    unsigned long m_positional_13c;                          /* 0x13c */
};

static_assert(sizeof(stLevel) == 0x140, "stLevel_must_be_0x140");
