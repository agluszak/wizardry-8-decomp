#pragma once

#include "surrender/srTexture.h"

/* Zero-storage client subclass of srTextureMap. The local vtable below is
   real, but its registry and clone entries point at srClassSupport<srTextureMap,
   srTexture,0,0x2111> emissions inherited from srTextureMap; this class defines
   none of them. Do not reintroduce getClassName, getClassID, getClassNode or
   clone overrides here. */
// VTABLE: WIZ8 0x005ebeec
class W8TextureMap : public srTextureMap {
public:
    explicit W8TextureMap(srColorSurfaceIFace* surface)
        : srTextureMap(surface) {}

protected:
    virtual ~W8TextureMap() override {}
};

static_assert(sizeof(W8TextureMap) == 0x5c,
              "W8TextureMap_must_be_0x5c");
