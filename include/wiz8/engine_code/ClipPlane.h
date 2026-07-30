#pragma once

#include "surrender/srClipPlane.h"

// VTABLE: WIZ8 0x005ed180
class W8ClipPlane005ED180 : public srClipPlane {
public:
    explicit W8ClipPlane005ED180(srNode* parent) : srClipPlane(parent) {}

    const char* getClassName() const override;     /* 0x004BDF10 */
    unsigned long getClassID() const override;     /* 0x004BDF00 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x004BDF20 */
    srClass* clone() override;                      /* 0x004BDF90 */
};

static_assert(sizeof(W8ClipPlane005ED180) == 0x150,
              "W8ClipPlane005ED180_must_be_0x150");
