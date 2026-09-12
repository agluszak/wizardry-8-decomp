#pragma once

#include "srBinIStream.h"
#include "srBinOStream.h"
#include "srHeap.h"

/* Nested Huffman types named by SR.DLL's mangled exports. Wizardry's only
   consumers are BitArray::Load / Save in Engine Code\BitArray.cpp, which
   compress the octree alpha-bit and prop-sun-bit indices (magic 0xDEADD00D).
   Stack extents come from those Wiz8 frames and the SR constructors:
   BitIStream 0x8c, BitOStream 0x54, Sampler 0x1c, Compressor 0x28,
   Decompressor 0x51c. Interior fields stay unnamed; Load/Save only construct
   the objects and call the imported methods. */
class srHuffman {
public:
    class BitIStream {
    public:
        SR_DLL_IMPORT BitIStream(srBinIStream& stream);

    private:
        unsigned char unknown_00_[0x8c];
    };

    class BitOStream {
    public:
        SR_DLL_IMPORT BitOStream(srBinOStream& stream);
        SR_DLL_IMPORT ~BitOStream();
        SR_DLL_IMPORT void put(unsigned long value, unsigned long bits);

    private:
        unsigned char unknown_00_[0x54];
    };

    class Sampler {
    public:
        SR_DLL_IMPORT Sampler();
        SR_DLL_IMPORT ~Sampler();
        SR_DLL_IMPORT void insert(unsigned long symbol);

    private:
        unsigned char unknown_00_[0x1c];
    };

    class Compressor {
    public:
        SR_DLL_IMPORT Compressor(const Sampler& sampler);
        SR_DLL_IMPORT ~Compressor();
        SR_DLL_IMPORT void storeSymbolTable(BitOStream& stream);

    private:
        unsigned char unknown_00_[0x28];
    };

    class Decompressor {
    public:
        SR_DLL_IMPORT Decompressor(BitIStream& stream);
        SR_DLL_IMPORT ~Decompressor();
        SR_DLL_IMPORT unsigned long decompressSymbol();
        SR_DLL_IMPORT unsigned long getDataCount() const;

    private:
        unsigned char unknown_00_[0x51c];
    };
};

static_assert(sizeof(srHuffman::BitIStream) == 0x8c, "srHuffman_BitIStream_must_be_0x8c");
static_assert(sizeof(srHuffman::BitOStream) == 0x54, "srHuffman_BitOStream_must_be_0x54");
static_assert(sizeof(srHuffman::Sampler) == 0x1c, "srHuffman_Sampler_must_be_0x1c");
static_assert(sizeof(srHuffman::Compressor) == 0x28, "srHuffman_Compressor_must_be_0x28");
static_assert(sizeof(srHuffman::Decompressor) == 0x51c, "srHuffman_Decompressor_must_be_0x51c");
