#pragma once

#include "srARGB.h"
#include "srTypeRegistry.h"

class srColorSurfaceIFace;

// VTABLE: SURRENDER 0x100753EC
// class srClassSupport<srPalette, srClass, 1, 10496>

// VTABLE: SURRENDER 0x100753CC srPalette
class SR_DLL_EXPORT srPalette : public srClassSupport<srPalette, srClass, 1, 0x2900> {
public:
    /* Two-level lookup: lut_rg[(g<<8)|r] selects a palette row, lut_rgb[(row<<8)|b] yields the
       index. */
    class SR_DLL_EXPORT Quantizer {
    public:
        Quantizer();
        // FUNCTION: SURRENDER 0x10004B40
        Quantizer(srARGB* colors, w8_long color_count, unsigned char* duplicates,
                  unsigned char red_bits, unsigned char green_bits, unsigned char blue_bits)
        {
            setPalette(colors, color_count, duplicates, red_bits, green_bits, blue_bits);
        }
        Quantizer(const Quantizer& other);
        ~Quantizer();

        void setPalette(srARGB* colors, w8_long color_count, unsigned char* duplicates,
                        unsigned char red_bits, unsigned char green_bits, unsigned char blue_bits);
        unsigned char quantize(const srARGB& color);
        unsigned char quantize(unsigned char red, unsigned char green, unsigned char blue);
        void quantize(unsigned char* indices, const srARGB* colors, w8_long color_count);

    private:
        struct Entry {
            w8_ulong red;
            w8_ulong green;
            w8_ulong blue;
            w8_ulong index;
        };

        void checkLuts();
        void initRG(w8_long red_lo, w8_long red_hi, w8_long green_lo, w8_long green_hi);
        unsigned char findRG(w8_long red, w8_long green);
        void createRGTable();
        void initRGB(w8_long red, w8_long green, w8_long blue_lo, w8_long blue_hi);
        unsigned char findRGB(w8_long blue);
        void partitionRGB(w8_long blue_lo, w8_long blue_hi);
        void createRGBTable();

        static int initialized;

        unsigned char lut_rg[0x10000];  /* 0x00000 */
        unsigned char lut_rgb[0x10000]; /* 0x10000 */
        srARGB palette[0x100];          /* 0x20000 */
        unsigned char duplicate[0x100]; /* 0x20400 */
        w8_long color_count;            /* 0x20500 */
        w8_ulong red_bits;              /* 0x20504 */
        w8_ulong green_bits;            /* 0x20508 */
        w8_ulong blue_bits;             /* 0x2050c */
        Entry entries[0x100];           /* 0x20510 */
        w8_long entry_count;            /* 0x21510 */
        unsigned char* lut_row;         /* 0x21514 */
        w8_long rg_dist[0x100];         /* 0x21518 */
    };

    /* Color sampler. Sampled colors are quantized to sample_bits per channel and accumulated in a
       0x8000-bucket open hash; entries hold the packed color and its total weight, linked through a
       parallel index array. */
    class SR_DLL_EXPORT Sampler {
    public:
        struct ColorEntry {
            srARGB color;
            w8_long count;
        };

        Sampler(w8_long sample_limit = 0);
        Sampler(const Sampler& other);
        ~Sampler();

        w8_long getColorCount();
        w8_long getOutputPaletteSize();
        w8_long getSampleCount();
        double getSampleFactor();
        w8_long getSampleBits();
        void setOutputPaletteSize(w8_long size);
        void setSampleBits(w8_long bits);
        void setSampleFactor(double factor);
        void discard();
        void removeMaskColor(w8_long index);
        void setMaskColor(w8_long index, const srARGB& color);
        void addColor(const srARGB& color, w8_long weight);
        void addColors(const srARGB* colors, w8_long color_count, w8_long weight);
        void addSurface(const char* name, w8_long weight);
        void addSurface(srColorSurfaceIFace& surface, w8_long weight);
        void addSurfaces(srColorSurfaceIFace** surfaces, w8_long surface_count, w8_long weight);
        srPalette* createOptimalPalette();
        void dump(std::ostream& stream);

    private:
        void shiftDown(srARGB& color);
        void shiftUp(srARGB& color);
        void reallocColors(w8_long capacity);

        w8_long sample_limit;            /* 0x000 */
        double sample_factor;            /* 0x008 */
        w8_long sample_bits;             /* 0x010 */
        w8_long color_count;             /* 0x014 */
        w8_long sample_count;            /* 0x018 */
        w8_long capacity;                /* 0x01c */
        w8_long output_palette_size;     /* 0x020 */
        unsigned char mask_flags[0x100]; /* 0x024 */
        srARGB mask_colors[0x100];       /* 0x124 */
        ColorEntry* colors;              /* 0x524 */
        w8_long* links;                  /* 0x528 */
        w8_long buckets[0x8000];         /* 0x52c */
    };

    /* Optimal-palette builder; an octree over the sampled 5-5-5 color space. Level arrays hold
       8^level nodes for levels 0-4; level-4 children index the sparse 15-bit leaf-bucket map. Leaf
       bucket nodes own Leaf records {color, weight, error}. */
    class SR_DLL_EXPORT Optimizer {
    public:
        struct PaletteInfo {
            const Sampler::ColorEntry* colors; /* 0x00 */
            w8_long color_count;               /* 0x04 */
            w8_long palette_size;              /* 0x08 */
            const srARGB* mask_colors;         /* 0x0c */
            const unsigned char* mask_flags;   /* 0x10 */
            w8_long mask_count;                /* 0x14 */
        };

        static srPalette* createOptimalPalette(const PaletteInfo& info);

    private:
        struct HashEntry {
            HashEntry() {}

            HashEntry* next;
            w8_ulong color;
            w8_long count;
        };

        struct Leaf {
            Leaf() {}

            w8_ulong color;
            w8_long weight;
            double error;
        };

        struct Node {
            double err_min;     /* 0x00 */
            double err_total;   /* 0x08 */
            w8_ulong color;     /* 0x10 */
            srARGB bound_lo;    /* 0x14 */
            srARGB bound_hi;    /* 0x18 */
            Leaf* leaves;       /* 0x1c */
            w8_long leaf_count; /* 0x20 */
            Node* parent;       /* 0x24 */
            Node* children[8];  /* 0x28 */
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

        W8_ABI_ASSERT(sizeof(Node) == 0x48, "OptimizerNode_must_be_0x48");
        W8_ABI_ASSERT(sizeof(Leaf) == 0x10, "OptimizerLeaf_must_be_0x10");
        W8_ABI_ASSERT(sizeof(LUT) == 0x1810, "OptimizerLUT_must_be_0x1810");
    };

    static SR_DLL_IMPORT const char* sGetClassName();
    static SR_DLL_IMPORT srPalette* findMatchingPalette(const srARGB* const colors,
                                                        w8_long color_count);

    SR_DLL_IMPORT srPalette(srARGB* colors = 0, w8_long color_count = 1);

    SR_DLL_IMPORT srPalette& operator=(const srPalette& other);

    virtual SR_DLL_IMPORT void dump(std::ostream& stream) override;
    virtual SR_DLL_IMPORT srClass* vInstance() override;

    SR_DLL_IMPORT srARGB getColor(w8_long index) const;
    SR_DLL_IMPORT const srARGB* getPaletteDataPtr();
    SR_DLL_IMPORT w8_long getPaletteSize() const;
    SR_DLL_IMPORT int matchPalette(const srARGB* const colors, w8_long color_count) const;
    SR_DLL_IMPORT unsigned char quantize(const srARGB& color);
    SR_DLL_IMPORT void quantize(unsigned char* const indices, const srARGB* const colors,
                                w8_long color_count);
    SR_DLL_IMPORT void releaseQuantizer();
    SR_DLL_IMPORT void setColor(w8_long index, const srARGB& color);
    SR_DLL_IMPORT void setColors(w8_long destination_index, const srARGB* const colors,
                                 w8_long color_count);
    SR_DLL_IMPORT void update();

protected:
    virtual SR_DLL_IMPORT ~srPalette() override;

private:
    SR_DLL_IMPORT void updateQuantizer();

    w8_ulong flags;
    srARGB* colors;
    w8_long color_count;
    Quantizer* quantizer;
};

W8_ABI_ASSERT(sizeof(srPalette::Quantizer) == 0x21918, "Quantizer_must_be_0x21918");
W8_ABI_ASSERT(sizeof(srPalette::Sampler) == 0x20530, "Sampler_must_be_0x20530");
W8_ABI_ASSERT(sizeof(srPalette) == 0x28, "srPalette_must_be_0x28");

typedef srClientSupport<srPalette, 0x2900> W8Palette;
