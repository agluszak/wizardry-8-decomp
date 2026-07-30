#pragma once

#include "surrender/srScene.h"

// VTABLE: WIZ8 0x005ebe14
class W8Camera005EBE14 : public srCamera {
public:
    explicit W8Camera005EBE14(srNode* parent) : srCamera(parent) {}
};

static_assert(sizeof(W8Camera005EBE14) == 0x188,
              "W8Camera005EBE14_must_be_0x188");
