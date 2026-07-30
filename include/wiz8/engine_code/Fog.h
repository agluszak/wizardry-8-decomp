#pragma once

#include "surrender/srFog.h"

/* Wizardry's instantiable wrapper presents as SurRender's canonical srFog
   class while supplying the first-party registry and clone slots. */
// VTABLE: WIZ8 0x005ec94c
class W8Fog005EC94C : public srFog {
public:
    explicit W8Fog005EC94C(srNode* parent) : srFog(parent) {}

    const char* getClassName() const override;             /* 0x00484710 */
    unsigned long getClassID() const override;             /* 0x00484700 */
    srRegistry::ClassNode* getClassNode() const override;  /* 0x00484720 */
    srClass* clone() override;                              /* 0x004847C0 */
};

static_assert(sizeof(W8Fog005EC94C) == 0x168,
              "W8Fog005EC94C_must_be_0x168");
