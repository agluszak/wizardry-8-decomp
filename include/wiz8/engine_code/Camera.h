#pragma once

#include "surrender/srScene.h"

// VTABLE: WIZ8 0x005ebe14
class W8Camera005EBE14 : public srCamera {
public:
    explicit W8Camera005EBE14(srNode* parent) : srCamera(parent) {}

    const char* getClassName() const override;     /* 0x0042A020 */
    unsigned long getClassID() const override;     /* 0x0042A010 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x0042A030 */
    srNode* vslot7() override;                     /* 0x0042A0A0 */
};

static_assert(sizeof(W8Camera005EBE14) == 0x188,
              "W8Camera005EBE14_must_be_0x188");
