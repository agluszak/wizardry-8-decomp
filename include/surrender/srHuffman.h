#pragma once

#include "srBinIStream.h"
#include "srBinOStream.h"
#include "srHeap.h"
#include "wiz8/engine_code/stHash.hpp"

/* Nested Huffman types named by SR.DLL's mangled exports. Wizardry's only
   consumers are BitArray::Load / Save in Engine Code\BitArray.cpp, which
   compress the octree alpha-bit and prop-sun-bit indices (magic 0xDEADD00D).
   Stack extents come from those Wiz8 frames and the SR constructors:
   BitIStream 0x8c, BitOStream 0x54, Sampler 0x1c, Compressor 0x28,
   Decompressor 0x51c.

   Sampler's prefix is the same open hash Wiz8 instantiates as
   W8HashTable<unsigned int, int>. Save does not call the imported ~Sampler
   (IAT 0x005eb768 is only an EH thunk); it destroys the symbol buffer with
   W8OwnedPtr (~ 0x004701b0) and then the hash table. Compressor keeps the
   same hash prefix, mapping symbols to Node*. */
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
        SR_DLL_IMPORT void insert(unsigned long symbol);

        W8HashTable<unsigned int, int> table;
        W8OwnedPtr symbols;
        unsigned long count_18;
    };

    class Compressor {
    public:
        struct Node {
            unsigned char unknown_00_[8];
            unsigned long code_08;
            unsigned long bits_0c;
            unsigned char unknown_10_[0xc];
        };

        SR_DLL_IMPORT Compressor(const Sampler& sampler);
        SR_DLL_IMPORT ~Compressor();
        SR_DLL_IMPORT void storeSymbolTable(BitOStream& stream);

        /* Wiz8 does not import compressSymbol; Save inlines Lookup+put. */
        void compressSymbol(BitOStream& stream, unsigned int symbol) const
        {
            Node* node = table.Lookup(&symbol);
            if (node != 0) {
                stream.put(node->code_08, node->bits_0c);
            }
        }

        W8HashTable<unsigned int, Node*> table;
        unsigned char unknown_10_[0xc];
        unsigned long num_symbols_1c;
        unsigned long code_width_20;
        unsigned char unknown_24_[4];
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

static_assert(sizeof(srHuffman::Compressor::Node) == 0x1c,
              "srHuffman_Compressor_Node_must_be_0x1c");
static_assert(sizeof(srHuffman::BitIStream) == 0x8c, "srHuffman_BitIStream_must_be_0x8c");
static_assert(sizeof(srHuffman::BitOStream) == 0x54, "srHuffman_BitOStream_must_be_0x54");
static_assert(sizeof(srHuffman::Sampler) == 0x1c, "srHuffman_Sampler_must_be_0x1c");
static_assert(sizeof(srHuffman::Compressor) == 0x28, "srHuffman_Compressor_must_be_0x28");
static_assert(sizeof(srHuffman::Decompressor) == 0x51c, "srHuffman_Decompressor_must_be_0x51c");
