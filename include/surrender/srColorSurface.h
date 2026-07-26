#pragma once

#include "srMath.h"
#include "srTypeRegistry.h"

class srPalette;
class srStat;

class srARGB {
public:
    enum e_index {
        INDEX_ALPHA = 0,
        INDEX_RED = 1,
        INDEX_GREEN = 2,
        INDEX_BLUE = 3
    };
};

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

    static SR_DLL_IMPORT const char* sGetClassName();

    virtual srColorSurfaceIFace* clone() = 0;
    virtual SR_DLL_IMPORT unsigned long getPixel(long x, long y);
    virtual SR_DLL_IMPORT void setPixel(long x, long y, unsigned long pixel);
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
    virtual SR_DLL_IMPORT void setLine(
        long x0, long y0, long x1, long y1, unsigned long pixel);
    virtual SR_DLL_IMPORT void composite(
        long x, long y, srColorSurfaceIFace& source,
        long source_x, long source_y, long width, long height,
        double alpha);
    virtual SR_DLL_IMPORT void blit(
        const BlitInfo& info, srColorSurfaceIFace& source);
    virtual void blit(long x, long y, srColorSurfaceIFace& source,
                      long source_x, long source_y, long width, long height) = 0;
    virtual SR_DLL_IMPORT void copy(srColorSurfaceIFace& source);
    virtual void swapPixelRows(long x, long y0, long y1, long width, long rows) = 0;
    virtual void flipRectangle(const Rectangle& rectangle) = 0;
    virtual SR_DLL_IMPORT void adjust(
        const srVector4T<float>& scale,
        const srVector4T<float>& offset,
        const srVector4T<float>& gamma);
    virtual SR_DLL_IMPORT void adjustSaturation(double saturation);
    virtual SR_DLL_IMPORT void getChannelStatistics(
        srStat& statistics, srARGB::e_index channel);
    virtual SR_DLL_IMPORT void remapPixels(const srARGB& from, const srARGB& to);
    virtual SR_DLL_IMPORT void copyColorChannel(
        srARGB::e_index destination, srARGB::e_index source);
    virtual SR_DLL_IMPORT void flipColorChannels(
        srARGB::e_index first, srARGB::e_index second);
    virtual void getPixelRow(unsigned long* pixels, long x, long y, long count) = 0;
    virtual void setPixelRow(const unsigned long* pixels, long x, long y, long count) = 0;
    virtual void getPixelRowRaw(void* pixels, long x, long y, long count) = 0;
    virtual void setPixelRowRaw(const void* pixels, long x, long y, long count) = 0;

protected:
    virtual void copyNoScaling(srColorSurfaceIFace& source) = 0;
    virtual SR_DLL_IMPORT void scaleHorizontal(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void scaleVertical(srColorSurfaceIFace& source);
    virtual void scaleFast(srColorSurfaceIFace& source) = 0;
    virtual SR_DLL_IMPORT void scale(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void magnify(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void minify(srColorSurfaceIFace& source);
};

// SR.DLL owns the storage implementation and most operations. SDK clients use
// a zero-data derived wrapper to supply the four runtime-class methods, so the
// wrapper has this same proven 0x5c-byte layout.
class srColorSurface : public srColorSurfaceIFace {
public:
    SR_DLL_IMPORT srColorSurface(srPixelConvert::e_surfaceType type,
                                 unsigned long width,
                                 unsigned long height);
    SR_DLL_IMPORT srColorSurface& operator=(const srColorSurface& other);

    static SR_DLL_IMPORT const char* sGetClassName();

    virtual SR_DLL_IMPORT void dump(std::ostream& stream);
    virtual SR_DLL_IMPORT srClass* vInstance();

    virtual SR_DLL_IMPORT unsigned long getPixelRaw(long x, long y);
    virtual SR_DLL_IMPORT void setPixelRaw(long x, long y, unsigned long pixel);
    virtual SR_DLL_IMPORT void getPixels(
        unsigned long* pixels, const srVector2i* positions, long count);
    virtual SR_DLL_IMPORT void setPixels(
        const unsigned long* pixels, const srVector2i* positions, long count);
    virtual SR_DLL_IMPORT void getPixelsRaw(
        void* pixels, const srVector2i* positions, long count);
    virtual SR_DLL_IMPORT void setPixelsRaw(
        const void* pixels, const srVector2i* positions, long count);
    virtual SR_DLL_IMPORT void getPixelColumn(
        unsigned long* pixels, long x, long y, long count);
    virtual SR_DLL_IMPORT void setPixelColumn(
        const unsigned long* pixels, long x, long y, long count);
    virtual SR_DLL_IMPORT srPalette* getPalette();
    virtual SR_DLL_IMPORT void setPalette(srPalette* palette);
    virtual SR_DLL_IMPORT void* getDataPtr();
    virtual SR_DLL_IMPORT long getDataSize();
    virtual SR_DLL_IMPORT int resize(long width, long height);
    virtual SR_DLL_IMPORT int rescale(long width, long height);
    virtual SR_DLL_IMPORT int changePixelFormat(
        const srPixelConvert::PixelFormat& format, int preserve);
    virtual SR_DLL_IMPORT void fill(unsigned long pixel);
    virtual SR_DLL_IMPORT void setHLine(
        long x, long y, long length, unsigned long pixel);
    virtual SR_DLL_IMPORT void setVLine(
        long x, long y, long length, unsigned long pixel);
    virtual SR_DLL_IMPORT void blit(
        long x, long y, srColorSurfaceIFace& source,
        long source_x, long source_y, long width, long height);
    virtual SR_DLL_IMPORT void swapPixelRows(
        long x, long y0, long y1, long width, long rows);
    virtual SR_DLL_IMPORT void flipRectangle(const Rectangle& rectangle);
    virtual SR_DLL_IMPORT void getPixelRow(
        unsigned long* pixels, long x, long y, long count);
    virtual SR_DLL_IMPORT void setPixelRow(
        const unsigned long* pixels, long x, long y, long count);
    virtual SR_DLL_IMPORT void getPixelRowRaw(
        void* pixels, long x, long y, long count);
    virtual SR_DLL_IMPORT void setPixelRowRaw(
        const void* pixels, long x, long y, long count);

    long rowPitch() const { return row_pitch_; }
    srColorSurfaceIFace* asInterface() { return this; }
    static srColorSurface* fromInterface(srColorSurfaceIFace& surface) {
        return reinterpret_cast<srColorSurface*>(&surface);
    }

    unsigned long width() const { return width_; }
    unsigned long height() const { return height_; }

protected:
    virtual SR_DLL_IMPORT ~srColorSurface();

private:
    virtual SR_DLL_IMPORT void copyNoScaling(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void scaleFast(srColorSurfaceIFace& source);

    unsigned char unknown_04_[0x18];
    unsigned long width_; // +0x1c
    unsigned long height_; // +0x20
    long row_pitch_; // +0x24
    unsigned char unknown_28_[0x34];
};

typedef char srSurfaceDesc_must_be_0x28[
    (sizeof(srColorSurfaceIFace::SurfaceDesc) == 0x28) ? 1 : -1];
typedef char srColorSurface_must_be_0x5c[(sizeof(srColorSurface) == 0x5c) ? 1 : -1];
