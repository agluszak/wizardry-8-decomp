#pragma once

#include "srFilter.h"
#include "srMath.h"
#include "srPtr.h"
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
        SURFACE_RGB565 = 0x07,
        SURFACE_RGB555 = 0x08,
        SURFACE_ARGB1555 = 0x09,
        SURFACE_BGR24 = 0x0c,
        SURFACE_BGRA32 = 0x0e,
        SURFACE_COPY = 0x18
    };

    struct ConversionInfo;
    typedef void (__cdecl *ConversionFunc)(const ConversionInfo& info);

    struct PixelFormat {
        unsigned char red_bits;
        unsigned char red_shift;
        unsigned char green_bits;
        unsigned char green_shift;
        unsigned char blue_bits;
        unsigned char blue_shift;
        unsigned char alpha_bits;
        unsigned char alpha_shift;
        e_surfaceType surface_type;
        long bytes_per_pixel_minus_one;
        unsigned long flags;

        SR_DLL_IMPORT void getName(char* name);
        SR_DLL_IMPORT int isValid() const;
        SR_DLL_IMPORT unsigned long match(
            const PixelFormat* formats, unsigned long count) const;
    };

    static SR_DLL_IMPORT e_surfaceType mapPixelFormat(
        const PixelFormat& format);
    static SR_DLL_IMPORT void mapPixelFormat(e_surfaceType type, PixelFormat& format);
    static SR_DLL_IMPORT void selectFuncs(
        const PixelFormat& format,
        ConversionFunc& write,
        ConversionFunc& read);
};

static_assert(sizeof(srPixelConvert::PixelFormat) == 0x14,
              "srPixelConvert_PixelFormat_must_be_0x14");

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
        unsigned long clamp_modes;
        srFilter* filter;
        srPixelConvert::PixelFormat pixel_format;
    };

    SR_DLL_IMPORT srColorSurfaceIFace();
    SR_DLL_IMPORT srColorSurfaceIFace(const srColorSurfaceIFace& other);

    static SR_DLL_IMPORT const char* sGetClassName();

    static srRegistry::ClassNode* sGetClassNode()
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x3100);

        if (node == 0) {
            node = registry->registerClass(
                sGetClassName(), srClass::sGetClassNode(), 0x3100, 1);
        }
        return node;
    }

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT ~srColorSurfaceIFace() override;
    virtual srColorSurfaceIFace* clone() = 0;
    virtual SR_DLL_IMPORT unsigned long getPixel(long x, long y);
    virtual SR_DLL_IMPORT void setPixel(long x, long y, unsigned long pixel);
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
    virtual SR_DLL_IMPORT void setLine(
        long x0, long y0, long x1, long y1, unsigned long pixel);
    virtual SR_DLL_IMPORT void composite(
        long x, long y, srColorSurfaceIFace& source,
        long source_x, long source_y, long width, long height,
        double alpha);
    virtual SR_DLL_IMPORT void blit(
        const BlitInfo& info, srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void blit(
        long x, long y, srColorSurfaceIFace& source,
        long source_x, long source_y, long width, long height);
    virtual SR_DLL_IMPORT void copy(srColorSurfaceIFace& source);
    virtual SR_DLL_IMPORT void swapPixelRows(
        long x, long y0, long y1, long width, long rows);
    virtual SR_DLL_IMPORT void flipRectangle(const Rectangle& rectangle);
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

    SR_DLL_IMPORT void addNoise(double amplitude, int monochrome);
    SR_DLL_IMPORT void clampCoordinates(long& x, long& y);
    SR_DLL_IMPORT void flipHorizontal();
    SR_DLL_IMPORT void flipVertical();
    SR_DLL_IMPORT unsigned long getAlphaBits() const;
    SR_DLL_IMPORT double getAspectRatio() const;
    SR_DLL_IMPORT long getBitsPerPixel() const;
    SR_DLL_IMPORT long getBlueBits() const;
    SR_DLL_IMPORT long getBytesPerPixel() const;
    SR_DLL_IMPORT long getClampedX(long x) const;
    SR_DLL_IMPORT long getClampedY(long y) const;
    SR_DLL_IMPORT srFilter* getFilter() const;
    SR_DLL_IMPORT long getGreenBits() const;
    SR_DLL_IMPORT int getHClampMode() const;
    SR_DLL_IMPORT long getHeight() const;
    SR_DLL_IMPORT long getPitch() const;
    SR_DLL_IMPORT void getPixelFormat(
        srPixelConvert::PixelFormat& format) const;
    SR_DLL_IMPORT long getRedBits() const;
    SR_DLL_IMPORT void getSurfaceDesc(SurfaceDesc& description) const;
    SR_DLL_IMPORT int getVClampMode() const;
    SR_DLL_IMPORT long getWidth() const;
    SR_DLL_IMPORT int isAlpha() const;
    SR_DLL_IMPORT int isPaletted() const;
    SR_DLL_IMPORT void rotate180();
    SR_DLL_IMPORT void setFilter(srFilter* filter);
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

    SR_DLL_IMPORT void copySurfaceParameters(
        const srColorSurfaceIFace& source);
    SR_DLL_IMPORT const srPixelConvert::PixelFormat* getPixelFormat() const;
    SR_DLL_IMPORT int isPixelFormatCompatible(
        const srColorSurfaceIFace& source) const;
    SR_DLL_IMPORT void setSurfaceDesc(const SurfaceDesc& description);

    unsigned char unknown_18_[0x04];
    unsigned long width_1c;
    unsigned long height_20;
    long pitch_24;
    unsigned long clamp_modes_28;
    srFilter* filter_2c;
    srPixelConvert::PixelFormat pixel_format_30;
};

static_assert(sizeof(srColorSurfaceIFace) == 0x44,
              "srColorSurfaceIFace_must_be_0x44");

/* The interface owns the common 0x44-byte surface description. The concrete
   implementation adds conversion callbacks, palette ownership and pixel-data
   storage; srClassSupport contributes registry identity without storage. */
class srColorSurface
    : public srClassSupport<
          srColorSurface, srColorSurfaceIFace, 0, 0x3110> {
public:
    SR_DLL_IMPORT srColorSurface(
        const srPixelConvert::PixelFormat& format,
        unsigned long width,
        unsigned long height);
    SR_DLL_IMPORT srColorSurface(srPixelConvert::e_surfaceType type,
                                 unsigned long width,
                                 unsigned long height);
    SR_DLL_IMPORT srColorSurface(
        const srPixelConvert::PixelFormat& format,
        void* data,
        unsigned long width,
        unsigned long height,
        unsigned long pitch);
    SR_DLL_IMPORT srColorSurface(srPixelConvert::e_surfaceType type,
                                 void* data,
                                 unsigned long width,
                                 unsigned long height,
                                 unsigned long pitch);
    SR_DLL_IMPORT srColorSurface(const srColorSurface& other);
    SR_DLL_IMPORT srColorSurface& operator=(const srColorSurface& other);

    static SR_DLL_IMPORT const char* sGetClassName();

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT srClass* vInstance() override;

    virtual SR_DLL_IMPORT unsigned long getPixelRaw(long x, long y) override;
    virtual SR_DLL_IMPORT void setPixelRaw(long x, long y, unsigned long pixel) override;
    virtual SR_DLL_IMPORT void getPixels(
        unsigned long* pixels, const srVector2i* positions, long count) override;
    virtual SR_DLL_IMPORT void setPixels(
        const unsigned long* pixels, const srVector2i* positions, long count) override;
    virtual SR_DLL_IMPORT void getPixelsRaw(
        void* pixels, const srVector2i* positions, long count) override;
    virtual SR_DLL_IMPORT void setPixelsRaw(
        const void* pixels, const srVector2i* positions, long count) override;
    virtual SR_DLL_IMPORT void getPixelColumn(
        unsigned long* pixels, long x, long y, long count) override;
    virtual SR_DLL_IMPORT void setPixelColumn(
        const unsigned long* pixels, long x, long y, long count) override;
    virtual SR_DLL_IMPORT srPalette* getPalette() override;
    virtual SR_DLL_IMPORT void setPalette(srPalette* palette) override;
    virtual SR_DLL_IMPORT void* getDataPtr() override;
    virtual SR_DLL_IMPORT long getDataSize() override;
    virtual SR_DLL_IMPORT int resize(long width, long height) override;
    virtual SR_DLL_IMPORT int rescale(long width, long height) override;
    virtual SR_DLL_IMPORT int changePixelFormat(
        const srPixelConvert::PixelFormat& format, int preserve) override;
    virtual SR_DLL_IMPORT void fill(unsigned long pixel) override;
    virtual SR_DLL_IMPORT void setHLine(
        long x, long y, long length, unsigned long pixel) override;
    virtual SR_DLL_IMPORT void setVLine(
        long x, long y, long length, unsigned long pixel) override;
    virtual SR_DLL_IMPORT void blit(
        long x, long y, srColorSurfaceIFace& source,
        long source_x, long source_y, long width, long height) override;
    virtual SR_DLL_IMPORT void swapPixelRows(
        long x, long y0, long y1, long width, long rows) override;
    virtual SR_DLL_IMPORT void flipRectangle(const Rectangle& rectangle) override;
    virtual SR_DLL_IMPORT void getPixelRow(
        unsigned long* pixels, long x, long y, long count) override;
    virtual SR_DLL_IMPORT void setPixelRow(
        const unsigned long* pixels, long x, long y, long count) override;
    virtual SR_DLL_IMPORT void getPixelRowRaw(
        void* pixels, long x, long y, long count) override;
    virtual SR_DLL_IMPORT void setPixelRowRaw(
        const void* pixels, long x, long y, long count) override;

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
    SR_DLL_IMPORT void init(
        const srPixelConvert::PixelFormat& format,
        unsigned long width,
        unsigned long height,
        unsigned long pitch);

    srPixelConvert::ConversionFunc pixel_write_44;
    srPixelConvert::ConversionFunc pixel_read_48;
    srPtr<srPalette> palette_4c;
    unsigned long surface_flags_50;
    long data_size_54;
    void* data_58;
};

static_assert((sizeof(srColorSurfaceIFace::SurfaceDesc) == 0x28), "srSurfaceDesc_must_be_0x28");
static_assert((sizeof(srColorSurface) == 0x5c), "srColorSurface_must_be_0x5c");
