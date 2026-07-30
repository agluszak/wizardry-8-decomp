#pragma once

#include "surrender/srColorSurface.h"

/* Wizardry's instantiable wrapper presents as SurRender's canonical
   srColorSurface class and supplies the first-party registry and clone slots. */
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

    const char* getClassName() const override;              /* 0x00429A50 */
    unsigned long getClassID() const override;              /* 0x00429A40 */
    srRegistry::ClassNode* getClassNode() const override;   /* 0x00429A60 */
    srColorSurfaceIFace* clone() override;                  /* 0x00429AD0 */

protected:
    virtual ~W8ColorSurface005EBD10() override {}
};

static_assert(sizeof(W8ColorSurface005EBD10) == 0x5c,
              "W8ColorSurface005EBD10_must_be_0x5c");
