#pragma once

#include "surrender/srNode.h"

/* Engine Code\ReadLevel.cpp. The allocation sites at 0x0044F2A1 and
   0x004BA3E3 establish the complete size. The constructor/destructor pair
   registers the object as class 0x10007 and proves srNode as its base. */
class stLevel : public srNode {
public:
    explicit stLevel(srNode* parent);
    virtual ~stLevel() override;

    virtual const char* getClassName() const override;       /* 0x004BA1C0 */
    virtual unsigned long getClassID() const override;       /* 0x004BA1B0 */
    virtual srRegistry::ClassNode* getClassNode() const override; /* 0x004BA1D0 */

    unsigned long m_active;                                  /* 0x138 */
    unsigned long m_positional_13c;                          /* 0x13c */
};

static_assert(sizeof(stLevel) == 0x140, "stLevel_must_be_0x140");
