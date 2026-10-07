#pragma once

#include "srArray.h"
#include "srBinIStream.h"
#include "srBinOStream.h"
#include "srHash.h"
#include "srHeap.h"

class srHuffman {
public:
    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        BitIStream {
    public:
        SR_DLL_IMPORT BitIStream(srBinIStream& stream);
        SR_DLL_IMPORT unsigned long get(unsigned long bits);
        SR_DLL_IMPORT unsigned long getBit();
        SR_DLL_IMPORT void rewind(long bits);

    private:
        BitIStream(const BitIStream& stream);
        BitIStream& operator=(const BitIStream& stream);

        void fetchCache(long position);
        unsigned long getByte(long position);
        unsigned long getDWord(long position);
        unsigned long getWordOrLess(unsigned long bits);

        srBinIStream* stream;
        unsigned char cache[0x80];
        long cache_base;
        long bit_pos;
    };

    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        BitOStream {
    public:
        SR_DLL_IMPORT BitOStream(srBinOStream& stream);
        SR_DLL_IMPORT ~BitOStream();
        SR_DLL_IMPORT void put(unsigned long value, unsigned long bits);

    private:
        BitOStream(const BitOStream& stream);
        BitOStream& operator=(const BitOStream& stream);

        void flush();
        void flushByte();
        void flushBuffer();
        void putBit(unsigned long bit);

        srBinOStream* stream;
        unsigned long bytes;
        unsigned long bit_count;
        unsigned long pending;
        unsigned char buffer[0x40];
        unsigned long buffered;
    };

    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Sampler {
    public:
        struct Symbol {
            unsigned long symbol;
            unsigned long frequency;
        };

        SR_DLL_IMPORT Sampler();
        /* Provider teardown is consistent with hash/array destruction. Wiz8
           imports the standalone destructor, so only the consumer declares it. */

#if !defined(SURRENDER_BUILD)
        SR_DLL_IMPORT ~Sampler();
#endif
        SR_DLL_IMPORT void insert(unsigned long symbol);
        SR_DLL_IMPORT unsigned long getNumSymbols() const;
        SR_DLL_IMPORT unsigned long getSymbolValue(unsigned long index) const;
        SR_DLL_IMPORT unsigned long getSymbolFrequency(unsigned long index) const;

    private:
        srHashTable<unsigned long, int> table;
        srArray<Symbol> symbols;
        int count;
    };

    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Compressor {
    public:
        struct Node {
            unsigned long symbol;
            unsigned long frequency;
            unsigned long code;
            unsigned long bits;
            Node* next;
            Node* children[2];
        };

        SR_DLL_IMPORT Compressor(const Sampler& sampler);
        SR_DLL_IMPORT ~Compressor();

        SR_DLL_IMPORT void storeSymbolTable(BitOStream& stream);
        SR_DLL_IMPORT void buildSymbolTree();
        SR_DLL_IMPORT void collectSymbols(const Sampler& sampler);

        // FUNCTION: SURRENDER 0x10001430
        void compressSymbol(BitOStream& stream, unsigned long symbol)
        {
            Node* node = table.Lookup(&symbol);
            if (node != 0) {
                stream.put(node->code, node->bits);
            }
        }

        srHashTable<unsigned long, Node*> table;
        Node* nodes;
        Node* free_list;
        Node* root;
        unsigned long num_symbols;
        unsigned long code_width;
        unsigned long total;

    private:
        Compressor(const Compressor& other);

        void dumpNode(BitOStream& stream, Node* node);

    public:
        void setupPath(Node* node, unsigned long code, unsigned long depth);
    };

    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Decompressor {
    public:
        SR_DLL_IMPORT Decompressor(BitIStream& stream);
        SR_DLL_IMPORT ~Decompressor();
        SR_DLL_IMPORT unsigned long decompressSymbol();
        SR_DLL_IMPORT unsigned long getDataCount() const;

    private:
        struct Symbol {
            unsigned long symbol;
            Symbol* children[2];
        };

        Decompressor(const Decompressor& other);
        Decompressor& operator=(const Decompressor& other);

        void setupSymbolTable(Symbol* node);

        BitIStream* stream;
        Symbol* symbols;
        unsigned long next_node;
        unsigned long code_width;
        unsigned long unknown_10;
        unsigned long num_symbols;
        unsigned long data_count;
        Symbol* lookup[0x100];
        unsigned char depth[0x100];
    };
};

static_assert(sizeof(srHuffman::Compressor::Node) == 0x1c,
              "srHuffman_Compressor_Node_must_be_0x1c");
static_assert(sizeof(srHuffman::BitIStream) == 0x8c, "srHuffman_BitIStream_must_be_0x8c");
static_assert(sizeof(srHuffman::BitOStream) == 0x54, "srHuffman_BitOStream_must_be_0x54");
static_assert(sizeof(srHuffman::Sampler) == 0x1c, "srHuffman_Sampler_must_be_0x1c");
static_assert(sizeof(srHuffman::Sampler::Symbol) == 0x08, "srHuffman_Sampler_Symbol_must_be_0x08");
static_assert(sizeof(srHuffman::Compressor) == 0x28, "srHuffman_Compressor_must_be_0x28");
static_assert(sizeof(srHuffman::Decompressor) == 0x51c, "srHuffman_Decompressor_must_be_0x51c");
