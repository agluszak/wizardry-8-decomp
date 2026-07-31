#pragma once

#include "surrender/srScene.h"

// VTABLE: WIZ8 0x005ebe14
class W8Camera : public srCamera {
public:
    explicit W8Camera(srNode* parent) : srCamera(parent) {}
};

static_assert(sizeof(W8Camera) == 0x188,
              "W8Camera_must_be_0x188");
