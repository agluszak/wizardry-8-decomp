#pragma once

#include "srTypeRegistry.h"

class srPalette;
class srStat;
class srVector2i;

class srARGB {
public:
    enum e_index {
        INDEX_ALPHA = 0,
        INDEX_RED = 1,
        INDEX_GREEN = 2,
        INDEX_BLUE = 3
    };
};

template <class T>
class srVector4T;

class srPixelConvert {
public:
    enum e_surfaceType {
        SURFACE_L8 = 0x02,
        SURFACE_BGR24 = 0x0c,
        SURFACE_BGRA32 = 0x0e,
        SURFACE_COPY = 0x18
    };

    struct PixelFormat {
        unsigned long words[5];
    };

    static SR_DLL_IMPORT void mapPixelFormat(e_surfaceType type, PixelFormat& format);
};

class srColorSurfaceIFace : public srClass {
public:
    struct Rectangle {
        long left;
        long top;
        long right;
        long bottom;
    };

    struct BlitInfo {
        unsigned long words[8];
    };

    struct SurfaceDesc {
        unsigned long width;
        unsigned long height;
        unsigned long pitch;
        unsigned long unknown_0c;
        unsigned long unknown_10;
        srPixelConvert::PixelFormat pixel_format;
    };

    virtual srColorSurfaceIFace* clone() = 0;
    virtual unsigned long getPixel(long x, long y) = 0;
    virtual void setPixel(long x, long y, unsigned long pixel) = 0;
    virtual unsigned long getPixelRaw(long x, long y) = 0;
    virtual void setPixelRaw(long x, long y, unsigned long pixel) = 0;
    virtual void getPixels(unsigned long* pixels, const srVector2i* positions, long count) = 0;
    virtual void setPixels(const unsigned long* pixels, const srVector2i* positions, long count) = 0;
    virtual void getPixelsRaw(void* pixels, const srVector2i* positions, long count) = 0;
    virtual void setPixelsRaw(const void* pixels, const srVector2i* positions, long count) = 0;
    virtual void getPixelColumn(unsigned long* pixels, long x, long y, long count) = 0;
    virtual void setPixelColumn(const unsigned long* pixels, long x, long y, long count) = 0;
    virtual srPalette* getPalette() = 0;
    virtual void setPalette(srPalette* palette) = 0;
    virtual void* getDataPtr() = 0;
    virtual long getDataSize() = 0;
    virtual int resize(long width, long height) = 0;
    virtual int rescale(long width, long height) = 0;
    virtual int changePixelFormat(const srPixelConvert::PixelFormat& format, int preserve) = 0;
    virtual void fill(unsigned long pixel) = 0;
    virtual void setHLine(long x, long y, long length, unsigned long pixel) = 0;
    virtual void setVLine(long x, long y, long length, unsigned long pixel) = 0;
    virtual void setLine(long x0, long y0, long x1, long y1, unsigned long pixel) = 0;
    virtual void composite(long x, long y, srColorSurfaceIFace& source,
                           long source_x, long source_y, long width, long height,
                           double alpha) = 0;
    virtual void blit(const BlitInfo& info, srColorSurfaceIFace& source) = 0;
    virtual void blit(long x, long y, srColorSurfaceIFace& source,
                      long source_x, long source_y, long width, long height) = 0;
    virtual void copy(srColorSurfaceIFace& source) = 0;
    virtual void swapPixelRows(long x, long y0, long y1, long width, long rows) = 0;
    virtual void flipRectangle(const Rectangle& rectangle) = 0;
    virtual void adjust(const srVector4T<float>& scale,
                        const srVector4T<float>& offset,
                        const srVector4T<float>& gamma) = 0;
    virtual void adjustSaturation(double saturation) = 0;
    virtual void getChannelStatistics(srStat& statistics, srARGB::e_index channel) = 0;
    virtual void remapPixels(const srARGB& from, const srARGB& to) = 0;
    virtual void copyColorChannel(srARGB::e_index destination, srARGB::e_index source) = 0;
    virtual void flipColorChannels(srARGB::e_index first, srARGB::e_index second) = 0;
    virtual void getPixelRow(unsigned long* pixels, long x, long y, long count) = 0;
    virtual void setPixelRow(const unsigned long* pixels, long x, long y, long count) = 0;
    virtual void getPixelRowRaw(void* pixels, long x, long y, long count) = 0;
    virtual void setPixelRowRaw(const void* pixels, long x, long y, long count) = 0;

protected:
    virtual void copyNoScaling(srColorSurfaceIFace& source) = 0;
    virtual void scaleHorizontal(srColorSurfaceIFace& source) = 0;
    virtual void scaleVertical(srColorSurfaceIFace& source) = 0;
    virtual void scaleFast(srColorSurfaceIFace& source) = 0;
    virtual void scale(srColorSurfaceIFace& source) = 0;
    virtual void magnify(srColorSurfaceIFace& source) = 0;
    virtual void minify(srColorSurfaceIFace& source) = 0;
};

// The concrete implementation lives in SR.DLL.  Its primary interface is at
// offset zero, but declaring it as a C++ subclass here would require inventing
// definitions for every SDK-owned virtual override merely to call its imported
// constructor.  Keep the concrete storage opaque and cross the proven
// zero-offset interface boundary explicitly at call sites.
class srColorSurface {
public:
    SR_DLL_IMPORT srColorSurface(srPixelConvert::e_surfaceType type,
                                 unsigned long width,
                                 unsigned long height);
    virtual SR_DLL_IMPORT ~srColorSurface();
    SR_DLL_IMPORT srColorSurface& operator=(const srColorSurface& other);

    static SR_DLL_IMPORT const char* sGetClassName();

    long rowPitch() const { return row_pitch_; }
    srColorSurfaceIFace* asInterface() {
        return reinterpret_cast<srColorSurfaceIFace*>(this);
    }

private:
    unsigned char unknown_04_[0x20];
    long row_pitch_; // +0x24
    unsigned char unknown_28_[0x34];
};

typedef char srSurfaceDesc_must_be_0x28[
    (sizeof(srColorSurfaceIFace::SurfaceDesc) == 0x28) ? 1 : -1];
typedef char srColorSurface_must_be_0x5c[(sizeof(srColorSurface) == 0x5c) ? 1 : -1];
