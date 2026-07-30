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
    class Quantizer;

    static SR_DLL_IMPORT const char* sGetClassName();
    static SR_DLL_IMPORT srPalette* findMatchingPalette(
        const srARGB* colors, long color_count);

    SR_DLL_IMPORT srPalette(srARGB* colors, long color_count);
    SR_DLL_IMPORT srPalette(const srPalette& other);
    SR_DLL_IMPORT srPalette& operator=(const srPalette& other);

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT srClass* vInstance() override;

    SR_DLL_IMPORT srARGB getColor(long index) const;
    SR_DLL_IMPORT const srARGB* getPaletteDataPtr();
    SR_DLL_IMPORT long getPaletteSize() const;
    SR_DLL_IMPORT int matchPalette(
        const srARGB* colors, long color_count) const;
    SR_DLL_IMPORT unsigned char quantize(const srARGB& color);
    SR_DLL_IMPORT void quantize(
        unsigned char* indices,
        const srARGB* colors,
        long color_count);
    SR_DLL_IMPORT void releaseQuantizer();
    SR_DLL_IMPORT void setColor(long index, const srARGB& color);
    SR_DLL_IMPORT void setColors(
        long destination_index,
        const srARGB* colors,
        long color_count);
    SR_DLL_IMPORT void update();

protected:
    virtual SR_DLL_IMPORT ~srPalette() override;

private:
    SR_DLL_IMPORT void updateQuantizer();

    unsigned long flags_18;
    srARGB* colors_1c;
    long color_count_20;
    Quantizer* quantizer_24;
};

static_assert(sizeof(srPalette) == 0x28, "srPalette_must_be_0x28");
