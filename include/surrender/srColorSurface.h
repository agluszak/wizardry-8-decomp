#pragma once

#include "srColorSurfaceIFace.h"
#include "srPalette.h"

// VTABLE: SURRENDER 0x100773A0
// class srClassSupport<srColorSurface, srColorSurfaceIFace, 0, 12560>

// VTABLE: SURRENDER 0x100772D0 srColorSurface
class SR_DLL_EXPORT srColorSurface
    : public srClassSupport<srColorSurface, srColorSurfaceIFace, 0, 0x3110> {
public:
    SR_DLL_IMPORT srColorSurface(const srPixelConvert::PixelFormat& format, w8_ulong width,
                                 w8_ulong height);
    SR_DLL_IMPORT srColorSurface(srPixelConvert::e_surfaceType type, w8_ulong width,
                                 w8_ulong height);
    SR_DLL_IMPORT srColorSurface(const srPixelConvert::PixelFormat& format, void* data,
                                 w8_ulong width, w8_ulong height, w8_ulong pitch);
    SR_DLL_IMPORT srColorSurface(srPixelConvert::e_surfaceType type, void* data, w8_ulong width,
                                 w8_ulong height, w8_ulong pitch);

    SR_DLL_IMPORT srColorSurface& operator=(const srColorSurface& other);

    static SR_DLL_IMPORT const char* sGetClassName();

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT srClass* vInstance() override;

    virtual SR_DLL_IMPORT w8_ulong getPixelRaw(w8_long x, w8_long y) override;
    virtual SR_DLL_IMPORT void setPixelRaw(w8_long x, w8_long y, w8_ulong pixel) override;
    virtual SR_DLL_IMPORT void getPixels(w8_ulong* pixels, const srVector2i* positions,
                                         w8_long count) override;
    virtual SR_DLL_IMPORT void setPixels(const w8_ulong* pixels, const srVector2i* positions,
                                         w8_long count) override;
    virtual SR_DLL_IMPORT void getPixelsRaw(void* pixels, const srVector2i* positions,
                                            w8_long count) override;
    virtual SR_DLL_IMPORT void setPixelsRaw(const void* pixels, const srVector2i* positions,
                                            w8_long count) override;
    virtual SR_DLL_IMPORT void getPixelColumn(w8_ulong* pixels, w8_long x, w8_long y_start,
                                              w8_long y_end) override;
    virtual SR_DLL_IMPORT void setPixelColumn(const w8_ulong* pixels, w8_long x, w8_long y_start,
                                              w8_long y_end) override;
    virtual SR_DLL_IMPORT srPalette* getPalette() override;
    virtual SR_DLL_IMPORT void setPalette(srPalette* palette) override;
    virtual SR_DLL_IMPORT void* getDataPtr() override;
    virtual SR_DLL_IMPORT w8_long getDataSize() override;
    virtual SR_DLL_IMPORT int resize(w8_long width, w8_long height) override;
    virtual SR_DLL_IMPORT int rescale(w8_long width, w8_long height) override;
    virtual SR_DLL_IMPORT int changePixelFormat(const srPixelConvert::PixelFormat& format,
                                                int preserve) override;
    virtual SR_DLL_IMPORT void fill(w8_ulong pixel) override;
    virtual SR_DLL_IMPORT void setHLine(w8_long y, w8_long x_start, w8_long x_end,
                                        w8_ulong pixel) override;
    virtual SR_DLL_IMPORT void setVLine(w8_long x, w8_long y_start, w8_long y_end,
                                        w8_ulong pixel) override;
    virtual SR_DLL_IMPORT void blit(w8_long x, w8_long y, srColorSurfaceIFace& source,
                                    w8_long source_x, w8_long source_y, w8_long width,
                                    w8_long height) override;
    virtual SR_DLL_IMPORT void swapPixelRows(w8_long x0, w8_long y0, w8_long x1, w8_long y1,
                                             w8_long count) override;
    virtual SR_DLL_IMPORT void flipRectangle(const Rectangle& rectangle) override;
    virtual SR_DLL_IMPORT void getPixelRow(w8_ulong* pixels, w8_long y, w8_long x_start,
                                           w8_long x_end) override;
    virtual SR_DLL_IMPORT void setPixelRow(const w8_ulong* pixels, w8_long y, w8_long x_start,
                                           w8_long x_end) override;
    virtual SR_DLL_IMPORT void getPixelRowRaw(void* pixels, w8_long y, w8_long x_start,
                                              w8_long x_end) override;
    virtual SR_DLL_IMPORT void setPixelRowRaw(const void* pixels, w8_long y, w8_long x_start,
                                              w8_long x_end) override;

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
    SR_DLL_IMPORT void init(const srPixelConvert::PixelFormat& format, w8_ulong width,
                            w8_ulong height, w8_ulong pitch);
    SR_DLL_IMPORT unsigned char* getAddress(w8_long x, w8_long y);
    SR_DLL_IMPORT void convertToARGB8888(w8_ulong* pixels, const void* source, w8_ulong count);
    SR_DLL_IMPORT void convertFromARGB8888(void* pixels, const w8_ulong* source, w8_ulong count);
    SR_DLL_IMPORT void reversePixels(void* pixels, w8_ulong count);
    SR_DLL_IMPORT int isCompatible(srColorSurfaceIFace& source);

    srPixelConvert::ConversionFunc pixel_write;
    srPixelConvert::ConversionFunc pixel_read;
    srPtr<srPalette> palette;
    enum { BORROWED_DATA = 0x01u };
    w8_ulong surface_flags;
    w8_long data_size;
    void* data;
};

W8_ABI_ASSERT((sizeof(srColorSurface) == 0x5c), "srColorSurface_must_be_0x5c");

/* Wizardry's client-side srColorSurface. */
typedef srClientSupport<srColorSurface, 0x3110> W8ColorSurface;
