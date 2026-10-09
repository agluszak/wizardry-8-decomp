#pragma once

#include "srARGB.h"
#include "srFilter.h"
#include "srMath.h"
#include "srPixelConvert.h"
#include "srPtr.h"
#include "srStat.h"
#include "srTypeRegistry.h"

class srColorSurface;
class srPalette;
struct W8TgaHeader;

// VTABLE: SURRENDER 0x10076708
// class srClassSupport<srColorSurfaceIFace, srClass, 1, 12544>

class __declspec(novtable) srColorSurfaceIFace
    : public srClassSupport<srColorSurfaceIFace, srClass, true, 0x3100> {
public:
    struct Rectangle {
        w8_long left;
        w8_long top;
        w8_long right;
        w8_long bottom;
    };

    struct BlitInfo {
        Rectangle destination;
        Rectangle source;
    };

    enum { CLAMP_HORIZONTAL = 0x01u, CLAMP_VERTICAL = 0x02u };

    struct SurfaceDesc {
        w8_ulong width;
        w8_ulong height;
        w8_ulong pitch;
        w8_ulong clamp_modes;
        srFilter* filter;
        srPixelConvert::PixelFormat pixel_format;
    };

    SR_DLL_IMPORT srColorSurfaceIFace();
    SR_DLL_IMPORT srColorSurfaceIFace(const srColorSurfaceIFace& other);
    SR_DLL_IMPORT srColorSurfaceIFace& operator=(const srColorSurfaceIFace& other);

    static SR_DLL_IMPORT const char* sGetClassName();

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT w8_ulong getPixel(w8_long x, w8_long y);
    virtual SR_DLL_IMPORT void setPixel(w8_long x, w8_long y, w8_ulong pixel);
    virtual SR_DLL_IMPORT w8_ulong getPixelRaw(w8_long x, w8_long y);
    virtual SR_DLL_IMPORT void setPixelRaw(w8_long x, w8_long y, w8_ulong pixel);
    virtual SR_DLL_IMPORT void getPixels(w8_ulong* pixels, const srVector2i* positions,
                                         w8_long count);
    virtual SR_DLL_IMPORT void setPixels(const w8_ulong* pixels, const srVector2i* positions,
                                         w8_long count);
    virtual SR_DLL_IMPORT void getPixelsRaw(void* pixels, const srVector2i* positions,
                                            w8_long count);
    virtual SR_DLL_IMPORT void setPixelsRaw(const void* pixels, const srVector2i* positions,
                                            w8_long count);
    virtual SR_DLL_IMPORT void getPixelColumn(w8_ulong* pixels, w8_long x, w8_long y_start,
                                              w8_long y_end);
    virtual SR_DLL_IMPORT void setPixelColumn(const w8_ulong* pixels, w8_long x, w8_long y_start,
                                              w8_long y_end);
    virtual SR_DLL_IMPORT srPalette* getPalette();
    virtual SR_DLL_IMPORT void setPalette(srPalette* palette);
    virtual SR_DLL_IMPORT void* getDataPtr();
    virtual SR_DLL_IMPORT w8_long getDataSize();
    virtual SR_DLL_IMPORT int resize(w8_long width, w8_long height);
    virtual SR_DLL_IMPORT int rescale(w8_long width, w8_long height);
    virtual SR_DLL_IMPORT int changePixelFormat(const srPixelConvert::PixelFormat& format,
                                                int preserve);
    virtual SR_DLL_IMPORT void fill(w8_ulong pixel);
    virtual SR_DLL_IMPORT void setHLine(w8_long y, w8_long x_start, w8_long x_end, w8_ulong pixel);
    virtual SR_DLL_IMPORT void setVLine(w8_long x, w8_long y_start, w8_long y_end, w8_ulong pixel);
    virtual SR_DLL_IMPORT void setLine(w8_long x0, w8_long y0, w8_long x1, w8_long y1,
                                       w8_ulong pixel);
    virtual SR_DLL_IMPORT void composite(w8_long x, w8_long y, srColorSurfaceIFace& source,
                                         w8_long source_x, w8_long source_y, w8_long width,
                                         w8_long height, double alpha);
    virtual SR_DLL_IMPORT void blit(w8_long x, w8_long y, srColorSurfaceIFace& source,
                                    w8_long source_x, w8_long source_y, w8_long width,
                                    w8_long height);
    virtual SR_DLL_IMPORT void blit(const BlitInfo& info, srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void copy(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void swapPixelRows(w8_long x0, w8_long y0, w8_long x1, w8_long y1,
                                             w8_long count);
    virtual SR_DLL_IMPORT void flipRectangle(const Rectangle& rectangle);
    virtual SR_DLL_IMPORT void adjust(const srVector4T<float>& scale,
                                      const srVector4T<float>& offset,
                                      const srVector4T<float>& gamma);
    virtual SR_DLL_IMPORT void adjustSaturation(double saturation);
    virtual SR_DLL_IMPORT void getChannelStatistics(srStat& statistics, srARGB::e_index channel);
    virtual SR_DLL_IMPORT void remapPixels(const srARGB& from, const srARGB& to);
    virtual SR_DLL_IMPORT void copyColorChannel(srARGB::e_index destination,
                                                srARGB::e_index source);
    virtual SR_DLL_IMPORT void flipColorChannels(srARGB::e_index first, srARGB::e_index second);
    virtual void getPixelRow(w8_ulong* pixels, w8_long y, w8_long x_start, w8_long x_end) = 0;
    virtual void setPixelRow(const w8_ulong* pixels, w8_long y, w8_long x_start, w8_long x_end) = 0;
    virtual void getPixelRowRaw(void* pixels, w8_long y, w8_long x_start, w8_long x_end) = 0;
    virtual void setPixelRowRaw(const void* pixels, w8_long y, w8_long x_start, w8_long x_end) = 0;

    SR_DLL_IMPORT void addNoise(double amplitude, int monochrome);
    SR_DLL_IMPORT void clampCoordinates(w8_long& x, w8_long& y);
    SR_DLL_IMPORT void flipHorizontal();
    SR_DLL_IMPORT void flipVertical();
    SR_DLL_IMPORT w8_ulong getAlphaBits() const;
    SR_DLL_IMPORT double getAspectRatio() const;
    SR_DLL_IMPORT w8_long getBitsPerPixel() const;
    SR_DLL_IMPORT w8_long getBlueBits() const;
    SR_DLL_IMPORT w8_long getBytesPerPixel() const;
    SR_DLL_IMPORT w8_long getClampedX(w8_long x) const;
    SR_DLL_IMPORT w8_long getClampedY(w8_long y) const;
    SR_DLL_IMPORT srFilter* getFilter() const;
    SR_DLL_IMPORT w8_long getGreenBits() const;
    SR_DLL_IMPORT int getHClampMode() const;
// FUNCTION: SURRENDER 0x100598F0 SYMBOL
// RECOMP: ?getHeight@srColorSurfaceIFace@@QBEJXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    w8_long getHeight() const
    {
        return height;
    }
// FUNCTION: SURRENDER 0x100599E0 SYMBOL
// RECOMP: ?getPitch@srColorSurfaceIFace@@QBEJXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    w8_long getPitch() const
    {
        return pitch;
    }
// FUNCTION: SURRENDER 0x100599F0 SYMBOL
// RECOMP: ?getPixelFormat@srColorSurfaceIFace@@QBEXAAUPixelFormat@srPixelConvert@@@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void getPixelFormat(srPixelConvert::PixelFormat& format) const
    {
        format = pixel_format;
    }
    SR_DLL_IMPORT w8_long getRedBits() const;
// FUNCTION: SURRENDER 0x10059A10 SYMBOL
// RECOMP: ?getSurfaceDesc@srColorSurfaceIFace@@QBEXAAUSurfaceDesc@1@@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void getSurfaceDesc(SurfaceDesc& description) const
    {
        description.width = width;
        description.height = height;
        description.pitch = pitch;
        description.clamp_modes = clamp_modes;
        description.filter = filter;
        description.pixel_format = pixel_format;
    }
    SR_DLL_IMPORT int getVClampMode() const;
// FUNCTION: SURRENDER 0x10059A60 SYMBOL
// RECOMP: ?getWidth@srColorSurfaceIFace@@QBEJXZ
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    w8_long getWidth() const
    {
        return width;
    }
    SR_DLL_IMPORT int isAlpha() const;
    SR_DLL_IMPORT int isPaletted() const;
    SR_DLL_IMPORT void rotate180();
// FUNCTION: SURRENDER 0x10059A90 SYMBOL
// RECOMP: ?setFilter@srColorSurfaceIFace@@QAEXPAVsrFilter@@@Z
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    void setFilter(srFilter* filter)
    {
        this->filter = filter;
    }
    SR_DLL_IMPORT void setHClampMode(int enabled);
    SR_DLL_IMPORT void setVClampMode(int enabled);

protected:
    virtual SR_DLL_IMPORT void copyNoScaling(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void scaleHorizontal(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void scaleVertical(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void scaleFast(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void scale(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void magnify(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void minify(srColorSurfaceIFace& source);

    SR_DLL_IMPORT void copySurfaceParameters(const srColorSurfaceIFace& source);
    SR_DLL_IMPORT const srPixelConvert::PixelFormat* getPixelFormat() const;
    SR_DLL_IMPORT int isPixelFormatCompatible(const srColorSurfaceIFace& source) const;
    SR_DLL_IMPORT void setSurfaceDesc(const SurfaceDesc& description);

    friend class stTextureFile;
    friend void __stdcall LoadSurfacePixels(int handle, srColorSurface* surface,
                                            const W8TgaHeader* header);
    friend class srColorSurface;

    unsigned char unknown_18_[0x04];
    w8_long width;
    w8_long height;
    w8_long pitch;
    w8_ulong clamp_modes;
    srFilter* filter;
    srPixelConvert::PixelFormat pixel_format;
};

W8_ABI_ASSERT(sizeof(srColorSurfaceIFace) == 0x44, "srColorSurfaceIFace_must_be_0x44");

W8_ABI_ASSERT((sizeof(srColorSurfaceIFace::SurfaceDesc) == 0x28), "srSurfaceDesc_must_be_0x28");
