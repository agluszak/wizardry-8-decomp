#pragma once

#include "surrender/srTexture.h"

/* Wizardry's instantiable wrapper presents as SurRender's canonical
   srTextureMap class and supplies the first-party registry and clone slots. */
// VTABLE: WIZ8 0x005ebeec
class W8TextureMap005EBEEC : public srTextureMap {
public:
    explicit W8TextureMap005EBEEC(srColorSurfaceIFace* surface)
        : srTextureMap(surface) {}

    const char* getClassName() const override;              /* 0x00429BF0 */
    unsigned long getClassID() const override;              /* 0x00429BE0 */
    srRegistry::ClassNode* getClassNode() const override;   /* 0x00429C00 */
    srTextureIFace* clone() override;                       /* 0x00429CA0 */

protected:
    virtual ~W8TextureMap005EBEEC() override {}
};

static_assert(sizeof(W8TextureMap005EBEEC) == 0x5c,
              "W8TextureMap005EBEEC_must_be_0x5c");
