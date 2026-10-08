/* Differential cases for the exported srHuffman bit streams, sampler,
   compressor and decompressor over the memory streams. The compressed bytes
   and every decoded symbol are printed. */

#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "surrender/srBinIStream.h"
#include "surrender/srBinOStream.h"
#include "surrender/srHuffman.h"

void runProbe(const char* name, void (*probe)());

static unsigned long g_huffman_rng = 1;

static unsigned long huffmanRandom()
{
    g_huffman_rng ^= g_huffman_rng << 13;
    g_huffman_rng ^= g_huffman_rng >> 17;
    g_huffman_rng ^= g_huffman_rng << 5;
    return g_huffman_rng;
}

static void dumpBytes(const char* label, const unsigned char* data, unsigned long size)
{
    unsigned long offset;
    printf("%s size %lu\n", label, size);
    for (offset = 0; offset < size; offset += 32) {
        unsigned long index;
        printf("%s %04lx ", label, offset);
        for (index = offset; index < offset + 32 && index < size; ++index) {
            printf("%02x", data[index]);
        }
        printf("\n");
    }
}

enum Distribution {
    DIST_SINGLE,
    DIST_TWO,
    DIST_UNIFORM16,
    DIST_SKEWED,
    DIST_BYTES,
    DIST_WIDE,
    DIST_FIBONACCI,
    DIST_COUNT
};

static const char* const g_distribution_names[] = {"single", "two",  "uniform16", "skewed",
                                                   "bytes",  "wide", "fibonacci"};
static int g_distribution;
static unsigned long g_symbol_count;
static char g_huffman_name[96];

static unsigned long drawSymbol(unsigned long index)
{
    unsigned long value = huffmanRandom();
    switch (g_distribution) {
    case DIST_SINGLE:
        return 42;
    case DIST_TWO:
        return (value & 3) == 0 ? 7 : 1000;
    case DIST_UNIFORM16:
        return value & 15;
    case DIST_SKEWED: {
        unsigned long symbol = 0;
        while ((value & 1) != 0 && symbol < 20) {
            value >>= 1;
            ++symbol;
        }
        return symbol;
    }
    case DIST_BYTES:
        return value & 0xff;
    case DIST_WIDE:
        return (value & 7) == 0 ? value : (value & 0x3f) * 0x01010101UL;
    case DIST_FIBONACCI: {
        /* Frequencies 1,1,2,3,5,8,...: maximally deep code tree. */
        static const unsigned long thresholds[] = {1, 2, 4, 7, 12, 20, 33, 54, 88, 143, 232, 376};
        unsigned long slot = index % 376;
        unsigned long symbol = 0;
        while (symbol < 11 && slot >= thresholds[symbol]) {
            ++symbol;
        }
        return 100 + symbol;
    }
    }
    return 0;
}

static void roundTripCase()
{
    srHuffman::Sampler sampler;
    unsigned long* symbols = new unsigned long[g_symbol_count + 1];
    unsigned long index;
    unsigned long mismatches = 0;
    for (index = 0; index < g_symbol_count; ++index) {
        symbols[index] = drawSymbol(index);
        sampler.insert(symbols[index]);
    }
    printf("sampler symbols %lu\n", sampler.getNumSymbols());
    for (index = 0; index < sampler.getNumSymbols(); ++index) {
        printf("sampler %lu value %08lx frequency %lu\n", index, sampler.getSymbolValue(index),
               sampler.getSymbolFrequency(index));
    }
    srHuffman::Compressor compressor(sampler);
    printf("compressor symbols %lu code-width %lu total %lu\n", compressor.num_symbols,
           compressor.code_width, compressor.total);
    srBinOMStream output;
    {
        srHuffman::BitOStream bits(output);
        /* Header layout used by the game's BitArray writer. */
        bits.put(compressor.num_symbols, 32);
        bits.put(compressor.code_width, 6);
        bits.put(g_symbol_count, 32);
        compressor.storeSymbolTable(bits);
        for (index = 0; index < g_symbol_count; ++index) {
            compressor.compressSymbol(bits, symbols[index]);
        }
    }
    unsigned long size = output.getSize();
    const unsigned char* data = static_cast<const unsigned char*>(output.getPtr());
    dumpBytes("packed", data, size);
    srBinIMStream input(data, size);
    srHuffman::BitIStream in_bits(input);
    srHuffman::Decompressor decompressor(in_bits);
    printf("decompressor data-count %lu\n", decompressor.getDataCount());
    for (index = 0; index < g_symbol_count; ++index) {
        unsigned long symbol = decompressor.decompressSymbol();
        if (index < 24 || symbol != symbols[index]) {
            printf("decoded %lu %08lx%s\n", index, symbol, symbol != symbols[index] ? " !" : "");
        }
        if (symbol != symbols[index]) {
            ++mismatches;
            if (mismatches > 8) {
                break;
            }
        }
    }
    printf("mismatches %lu\n", mismatches);
}

static void bitStreamCase()
{
    static const unsigned long widths[] = {1, 3, 8, 13, 16, 17, 24, 31, 32, 0, 5, 32, 7, 2, 9};
    unsigned long values[64];
    unsigned long index;
    srBinOMStream output;
    {
        srHuffman::BitOStream bits(output);
        for (index = 0; index < 64; ++index) {
            unsigned long width = widths[index % 15];
            values[index] = huffmanRandom();
            if (width < 32) {
                values[index] &= (1UL << width) - 1;
            }
            bits.put(values[index], width);
        }
    }
    unsigned long size = output.getSize();
    const unsigned char* data = static_cast<const unsigned char*>(output.getPtr());
    dumpBytes("bits", data, size);
    srBinIMStream input(data, size);
    srHuffman::BitIStream in_bits(input);
    for (index = 0; index < 64; ++index) {
        unsigned long width = widths[index % 15];
        unsigned long value = in_bits.get(width);
        printf("get %lu w%lu %08lx%s\n", index, width, value, value != values[index] ? " !" : "");
        if (index == 20) {
            in_bits.rewind(static_cast<long>(width));
            printf("reget %08lx\n", in_bits.get(width));
        }
    }
    srBinIMStream again(data, size);
    srHuffman::BitIStream single(again);
    printf("first-bits");
    for (index = 0; index < 40; ++index) {
        printf("%lu", single.getBit());
    }
    printf("\n");
}

void huffmanCases()
{
    static const unsigned long counts[] = {1, 2, 37, 1000, 5000};
    int distribution;
    int count;
    for (distribution = 0; distribution < DIST_COUNT; ++distribution) {
        for (count = 0; count < 5; ++count) {
            g_distribution = distribution;
            g_symbol_count = counts[count];
            sprintf(g_huffman_name, "huffman.roundtrip.%s.n%lu", g_distribution_names[distribution],
                    g_symbol_count);
            g_huffman_rng = 0x9e3779b9UL ^ (distribution * 131 + count * 7919 + 1);
            runProbe(g_huffman_name, roundTripCase);
        }
    }
    g_huffman_rng = 12345;
    runProbe("huffman.bitstream", bitStreamCase);
}
