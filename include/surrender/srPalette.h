#pragma once

#include "srARGB.h"
#include "srTypeRegistry.h"

class srColorSurfaceIFace;

/* SR.DLL owns palette storage and behavior. Its exported constructors and the
   allocation in Wizardry's TGA loader prove a 0x28-byte srClass-derived
   object. The executable emits the inline srClassSupport registry and clone
   slots, which is why its local vtable mixes imported srPalette methods with
   the 0x2900 class-support methods recovered in stTextureFile.cpp. */
class srPalette : public srClassSupport<srPalette, srClass, 1, 0x2900> {
public:
    /* Provider-side quantizer; its 0x21918-byte allocation and member surface
       are evidenced by updateQuantizer (0x10004240) and the exported
       Quantizer methods. Two-level lookup: lut_rg[(g<<8)|r] selects a palette
       row, lut_rgb[(row<<8)|b] yields the index. */
    class Quantizer {
    public:
        Quantizer();
        Quantizer(srARGB* colors, long color_count, unsigned char* duplicates,
                  unsigned char red_bits, unsigned char green_bits, unsigned char blue_bits);
        Quantizer(const Quantizer& other);
        ~Quantizer();
        Quantizer& operator=(const Quantizer& other);

        void setPalette(srARGB* colors, long color_count, unsigned char* duplicates,
                        unsigned char red_bits, unsigned char green_bits, unsigned char blue_bits);
        unsigned char quantize(const srARGB& color);
        unsigned char quantize(unsigned char red, unsigned char green, unsigned char blue);
        void quantize(unsigned char* indices, const srARGB* colors, long color_count);

    private:
        struct Entry {
            unsigned long red;
            unsigned long green;
            unsigned long blue;
            unsigned long index;
        };

        void checkLuts();
        void initRG(long red_lo, long red_hi, long green_lo, long green_hi);
        unsigned char findRG(long red, long green);
        void createRGTable();
        void initRGB(long red, long green, long blue_lo, long blue_hi);
        unsigned char findRGB(long blue);
        void partitionRGB(long blue_lo, long blue_hi);
        void createRGBTable();

        static int initialized;

        unsigned char lut_rg[0x10000];  /* 0x00000 */
        unsigned char lut_rgb[0x10000]; /* 0x10000 */
        srARGB palette[0x100];          /* 0x20000 */
        unsigned char duplicate[0x100]; /* 0x20400 */
        long color_count;               /* 0x20500 */
        unsigned long red_bits;         /* 0x20504 */
        unsigned long green_bits;       /* 0x20508 */
        unsigned long blue_bits;        /* 0x2050c */
        Entry entries[0x100];           /* 0x20510 */
        long entry_count;               /* 0x21510 */
        unsigned char* lut_row;         /* 0x21514 */
        long rg_dist[0x100];            /* 0x21518 */
    };

    /* Provider-side color sampler; its 0x2052c-byte layout is established by
       the constructor (0x100067d0) and the field offsets used throughout
       addColor (0x10006530). Sampled colors are quantized to sample_bits per
       channel and accumulated in a 0x8000-bucket open hash; entries hold the
       packed color and its total weight, linked through a parallel index
       array. */
    class Sampler {
    public:
        struct ColorEntry {
            srARGB color;
            long count;
        };

        Sampler(long sample_limit = 0);
        Sampler(const Sampler& other);
        ~Sampler();
        Sampler& operator=(const Sampler& other);

        long getColorCount();
        long getOutputPaletteSize();
        long getSampleCount();
        double getSampleFactor();
        long getSampleBits();
        void setOutputPaletteSize(long size);
        void setSampleBits(long bits);
        void setSampleFactor(double factor);
        void discard();
        void removeMaskColor(long index);
        void setMaskColor(long index, const srARGB& color);
        void addColor(const srARGB& color, long weight);
        void addColors(const srARGB* colors, long color_count, long weight);
        void addSurface(const char* name, long weight);
        void addSurface(srColorSurfaceIFace& surface, long weight);
        void addSurfaces(srColorSurfaceIFace** surfaces, long surface_count, long weight);
        srPalette* createOptimalPalette();
        void dump(std::ostream& stream);

    private:
        void shiftDown(srARGB& color);
        void shiftUp(srARGB& color);
        void reallocColors(long capacity);

        long sample_limit;               /* 0x000 */
        double sample_factor;            /* 0x008 */
        long sample_bits;                /* 0x010 */
        long color_count;                /* 0x014 */
        long sample_count;               /* 0x018 */
        long capacity;                   /* 0x01c */
        long output_palette_size;        /* 0x020 */
        unsigned char mask_flags[0x100]; /* 0x024 */
        srARGB mask_colors[0x100];       /* 0x124 */
        ColorEntry* colors;              /* 0x524 */
        long* links;                     /* 0x528 */
        long buckets[0x8000];            /* 0x52c */
    };

    /* Provider-side optimal-palette builder; an octree over the sampled
       5-5-5 color space. Level arrays hold 8^level nodes for levels 0-4;
       level-4 children index the sparse 15-bit leaf-bucket map. Leaf bucket
       nodes own 0x10-byte Leaf records {color, weight, error}. */
    class Optimizer {
    public:
        struct PaletteInfo {
            const Sampler::ColorEntry* colors; /* 0x00 */
            long color_count;                  /* 0x04 */
            long palette_size;                 /* 0x08 */
            const srARGB* mask_colors;         /* 0x0c */
            const unsigned char* mask_flags;   /* 0x10 */
            long mask_count;                   /* 0x14 */
        };

        Optimizer& operator=(const Optimizer& other);

        static srPalette* createOptimalPalette(const PaletteInfo& info);

    private:
        struct Leaf {
            unsigned long color;
            long weight;
            double error;
        };

        struct Node {
            double err_min;      /* 0x00 */
            double err_total;    /* 0x08 */
            unsigned long color; /* 0x10 */
            srARGB bound_lo;     /* 0x14 */
            srARGB bound_hi;     /* 0x18 */
            Leaf* leaves;        /* 0x1c */
            long leaf_count;     /* 0x20 */
            Node* parent;        /* 0x24 */
            Node* children[8];   /* 0x28 */
        };

        struct LUT {
            float dist_r[0x200]; /* 0x0000 (i-256)^2 * 0.299 */
            float dist_g[0x200]; /* 0x0800 (i-256)^2 * 0.587 */
            float dist_b[0x200]; /* 0x1000 (i-256)^2 * 0.114 */
            float* r;            /* 0x1800 dist_r biased by (0x100 - red) */
            float* g;            /* 0x1804 */
            float* b;            /* 0x1808 */
            srARGB color;        /* 0x180c */
        };

        static void findOptimalColor(Node* node, const LUT& lut);
        static void setupLUT(LUT& lut, const srARGB& color);

        static_assert(sizeof(Node) == 0x48, "OptimizerNode_must_be_0x48");
        static_assert(sizeof(Leaf) == 0x10, "OptimizerLeaf_must_be_0x10");
        static_assert(sizeof(LUT) == 0x1810, "OptimizerLUT_must_be_0x1810");
    };

    static SR_DLL_IMPORT const char* sGetClassName();
    static SR_DLL_IMPORT srPalette* findMatchingPalette(const srARGB* const colors,
                                                        long color_count);

    SR_DLL_IMPORT srPalette(srARGB* colors = 0, long color_count = 1);
    SR_DLL_IMPORT srPalette(const srPalette& other);
    SR_DLL_IMPORT srPalette& operator=(const srPalette& other);

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT srClass* vInstance() override;

    SR_DLL_IMPORT srARGB getColor(long index) const;
    SR_DLL_IMPORT const srARGB* getPaletteDataPtr();
    SR_DLL_IMPORT long getPaletteSize() const;
    SR_DLL_IMPORT int matchPalette(const srARGB* const colors, long color_count) const;
    SR_DLL_IMPORT unsigned char quantize(const srARGB& color);
    SR_DLL_IMPORT void quantize(unsigned char* const indices, const srARGB* const colors,
                                long color_count);
    SR_DLL_IMPORT void releaseQuantizer();
    SR_DLL_IMPORT void setColor(long index, const srARGB& color);
    SR_DLL_IMPORT void setColors(long destination_index, const srARGB* const colors,
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

static_assert(sizeof(srPalette::Quantizer) == 0x21918, "Quantizer_must_be_0x21918");
static_assert(sizeof(srPalette::Sampler) == 0x20530, "Sampler_must_be_0x20530");
static_assert(sizeof(srPalette) == 0x28, "srPalette_must_be_0x28");

typedef srClientSupport<srPalette, 0x2900> W8Palette;
