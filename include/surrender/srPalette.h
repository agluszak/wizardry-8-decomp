#pragma once

#include "srColorSurface.h"
#include "srTypeRegistry.h"

/* SR.DLL owns palette storage and behavior. Its exported constructors and the
   allocation in Wizardry's TGA loader prove a 0x28-byte srClass-derived
   object. The executable emits the inline srClassSupport registry and clone
   slots, which is why its local vtable mixes imported srPalette methods with
   the 0x2900 class-support methods recovered in stTextureFile.cpp. */
class srPalette
    : public srClassSupport<srPalette, srClass, 0, 0x2900> {
public:
    static SR_DLL_IMPORT const char* sGetClassName();
    static SR_DLL_IMPORT srPalette* findMatchingPalette(
        const srARGB* colors, long color_count);

    SR_DLL_IMPORT srPalette(srARGB* colors, long color_count);
    SR_DLL_IMPORT srPalette(const srPalette& other);
    SR_DLL_IMPORT srPalette& operator=(const srPalette& other);

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT srClass* vInstance() override;

protected:
    virtual SR_DLL_IMPORT ~srPalette() override;

private:
    unsigned char palette_state_18[0x10];
};

static_assert(sizeof(srPalette) == 0x28, "srPalette_must_be_0x28");
