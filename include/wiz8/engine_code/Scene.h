#pragma once

#include "surrender/srScene.h"

// VTABLE: WIZ8 0x005ebe48
class W8Scene : public srScene {
public:
    explicit W8Scene(srNode* parent) : srScene(parent) {}

    void ClearOverlayState()
    {
        ambient_light_174.x = 0.0f;
        ambient_light_174.y = 0.0f;
        ambient_light_174.z = 0.0f;
        fog_color_180.x = 0.0f;
        fog_color_180.y = 0.0f;
        fog_color_180.z = 0.0f;
    }
};

static_assert(sizeof(W8Scene) == 0x190,
              "W8Scene_must_be_0x190");
