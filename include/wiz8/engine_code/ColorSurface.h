#pragma once

#include "surrender/srColorSurface.h"

// VTABLE: WIZ8 0x005ebd10
class W8ColorSurface005EBD10 : public srColorSurface {
public:
    W8ColorSurface005EBD10(
        srPixelConvert::e_surfaceType type,
        unsigned long width,
        unsigned long height)
        : srColorSurface(type, width, height) {}

    W8ColorSurface005EBD10(
        srPixelConvert::e_surfaceType type,
        void* data,
        unsigned long width,
        unsigned long height,
        unsigned long pitch)
        : srColorSurface(type, data, width, height, pitch) {}

protected:
    virtual ~W8ColorSurface005EBD10() override {}
};

static_assert(sizeof(W8ColorSurface005EBD10) == 0x5c,
              "W8ColorSurface005EBD10_must_be_0x5c");
