#pragma once

#include "srColorSurfaceIFace.h"

/* The interface owns the common 0x44-byte surface description. The concrete
   implementation adds conversion callbacks, palette ownership and pixel-data
   storage; srClassSupport contributes registry identity without storage. */
class srColorSurface : public srClassSupport<srColorSurface, srColorSurfaceIFace, 0, 0x3110> {
public:
    SR_DLL_IMPORT srColorSurface(const srPixelConvert::PixelFormat& format, unsigned long width,
                                 unsigned long height);
    SR_DLL_IMPORT srColorSurface(srPixelConvert::e_surfaceType type, unsigned long width,
                                 unsigned long height);
    SR_DLL_IMPORT srColorSurface(const srPixelConvert::PixelFormat& format, void* data,
                                 unsigned long width, unsigned long height, unsigned long pitch);
    SR_DLL_IMPORT srColorSurface(srPixelConvert::e_surfaceType type, void* data,
                                 unsigned long width, unsigned long height, unsigned long pitch);
    SR_DLL_IMPORT srColorSurface(const srColorSurface& other);
    SR_DLL_IMPORT srColorSurface& operator=(const srColorSurface& other);

    static SR_DLL_IMPORT const char* sGetClassName();

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT srClass* vInstance() override;

    virtual SR_DLL_IMPORT unsigned long getPixelRaw(long x, long y) override;
    virtual SR_DLL_IMPORT void setPixelRaw(long x, long y, unsigned long pixel) override;
    virtual SR_DLL_IMPORT void getPixels(unsigned long* pixels, const srVector2i* positions,
                                         long count) override;
    virtual SR_DLL_IMPORT void setPixels(const unsigned long* pixels, const srVector2i* positions,
                                         long count) override;
    virtual SR_DLL_IMPORT void getPixelsRaw(void* pixels, const srVector2i* positions,
                                            long count) override;
    virtual SR_DLL_IMPORT void setPixelsRaw(const void* pixels, const srVector2i* positions,
                                            long count) override;
    virtual SR_DLL_IMPORT void getPixelColumn(unsigned long* pixels, long x, long y,
                                              long count) override;
    virtual SR_DLL_IMPORT void setPixelColumn(const unsigned long* pixels, long x, long y,
                                              long count) override;
    virtual SR_DLL_IMPORT srPalette* getPalette() override;
    virtual SR_DLL_IMPORT void setPalette(srPalette* palette) override;
    virtual SR_DLL_IMPORT void* getDataPtr() override;
    virtual SR_DLL_IMPORT long getDataSize() override;
    virtual SR_DLL_IMPORT int resize(long width, long height) override;
    virtual SR_DLL_IMPORT int rescale(long width, long height) override;
    virtual SR_DLL_IMPORT int changePixelFormat(const srPixelConvert::PixelFormat& format,
                                                int preserve) override;
    virtual SR_DLL_IMPORT void fill(unsigned long pixel) override;
    virtual SR_DLL_IMPORT void setHLine(long x, long y, long length, unsigned long pixel) override;
    virtual SR_DLL_IMPORT void setVLine(long x, long y, long length, unsigned long pixel) override;
    virtual SR_DLL_IMPORT void blit(long x, long y, srColorSurfaceIFace& source, long source_x,
                                    long source_y, long width, long height) override;
    virtual SR_DLL_IMPORT void swapPixelRows(long x, long y0, long y1, long width,
                                             long rows) override;
    virtual SR_DLL_IMPORT void flipRectangle(const Rectangle& rectangle) override;
    virtual SR_DLL_IMPORT void getPixelRow(unsigned long* pixels, long x, long y,
                                           long count) override;
    virtual SR_DLL_IMPORT void setPixelRow(const unsigned long* pixels, long x, long y,
                                           long count) override;
    virtual SR_DLL_IMPORT void getPixelRowRaw(void* pixels, long x, long y, long count) override;
    virtual SR_DLL_IMPORT void setPixelRowRaw(const void* pixels, long x, long y,
                                              long count) override;

    SR_DLL_IMPORT srPixelConvert::ConversionFunc getPixelReadFunc() const;
    SR_DLL_IMPORT srPixelConvert::ConversionFunc getPixelWriteFunc() const;
    SR_DLL_IMPORT void setPixelReadFunc(srPixelConvert::ConversionFunc function);
    SR_DLL_IMPORT void setPixelWriteFunc(srPixelConvert::ConversionFunc function);

protected:
    virtual SR_DLL_IMPORT ~srColorSurface() override;

private:
    virtual SR_DLL_IMPORT void copyNoScaling(srColorSurfaceIFace& source) override;
    virtual SR_DLL_IMPORT void scaleFast(srColorSurfaceIFace& source) override;

    SR_DLL_IMPORT void allocData();
    SR_DLL_IMPORT void freeData();
    SR_DLL_IMPORT void init(const srPixelConvert::PixelFormat& format, unsigned long width,
                            unsigned long height, unsigned long pitch);

    srPixelConvert::ConversionFunc pixel_write_44;
    srPixelConvert::ConversionFunc pixel_read_48;
    srPtr<srPalette> palette_4c;
    unsigned long surface_flags_50;
    long data_size_54;
    void* data_58;
};

static_assert((sizeof(srColorSurface) == 0x5c), "srColorSurface_must_be_0x5c");

/* Wizardry's client-side self-support instantiation of srColorSurface. The
   retail image constructs this form (the compiled constructor calls the
   imported srColorSurface constructor and then installs table 0x005EBD10 over
   the class's own), and the template supplies the registry lifecycle. */
typedef srClassSupport<srColorSurface, srColorSurface, false, 0x3110> W8ColorSurface;
