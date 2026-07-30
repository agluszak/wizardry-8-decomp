#pragma once

#include "surrender/srScene.h"

// VTABLE: WIZ8 0x005ebe48
class W8Scene005EBE48 : public srScene {
public:
    explicit W8Scene005EBE48(srNode* parent) : srScene(parent) {}

    const char* getClassName() const override;     /* 0x0042A0D0 */
    unsigned long getClassID() const override;     /* 0x0042A0C0 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x0042A0E0 */
    srNode* clone() override;                      /* 0x0042A150 */

    void ClearOverlayState()
    {
        int index;
        for (index = 0; index != 6; ++index) {
            overlay_state_[index] = 0;
        }
    }
};

static_assert(sizeof(W8Scene005EBE48) == 0x190,
              "W8Scene005EBE48_must_be_0x190");
