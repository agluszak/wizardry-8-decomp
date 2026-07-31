#pragma once

#include "surrender/srColorSurface.h"

// VTABLE: WIZ8 0x005ebd10
class W8ColorSurface : public srColorSurface {
public:
    W8ColorSurface(
        srPixelConvert::e_surfaceType type,
        unsigned long width,
        unsigned long height)
        : srColorSurface(type, width, height) {}

    W8ColorSurface(
        srPixelConvert::e_surfaceType type,
        void* data,
        unsigned long width,
        unsigned long height,
        unsigned long pitch)
        : srColorSurface(type, data, width, height, pitch) {}

protected:
    virtual ~W8ColorSurface() override {}
};

static_assert(sizeof(W8ColorSurface) == 0x5c,
              "W8ColorSurface_must_be_0x5c");
