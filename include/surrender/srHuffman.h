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
        SR_DLL_IMPORT w8_ulong get(w8_ulong bits);
        SR_DLL_IMPORT w8_ulong getBit();
        SR_DLL_IMPORT void rewind(w8_long bits);

    private:
        BitIStream(const BitIStream& stream);
        BitIStream& operator=(const BitIStream& stream);

        void fetchCache(w8_long position);
        w8_ulong getByte(w8_long position);
        w8_ulong getDWord(w8_long position);
        w8_ulong getWordOrLess(w8_ulong bits);

        srBinIStream* stream;
        unsigned char cache[0x80];
        w8_long cache_base;
        w8_long bit_pos;
    };

    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        BitOStream {
    public:
        SR_DLL_IMPORT BitOStream(srBinOStream& stream);
        SR_DLL_IMPORT ~BitOStream();
        SR_DLL_IMPORT void put(w8_ulong value, w8_ulong bits);

    private:
        BitOStream(const BitOStream& stream);
        BitOStream& operator=(const BitOStream& stream);

        void flush();
        void flushByte();
        void flushBuffer();
        void putBit(w8_ulong bit);

        srBinOStream* stream;
        w8_ulong bytes;
        w8_ulong bit_count;
        w8_ulong pending;
        unsigned char buffer[0x40];
        w8_ulong buffered;
    };

    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Sampler {
    public:
        struct Symbol {
            w8_ulong symbol;
            w8_ulong frequency;
        };

        SR_DLL_IMPORT Sampler();

#if !defined(SURRENDER_BUILD)
        SR_DLL_IMPORT ~Sampler();
#endif
        SR_DLL_IMPORT void insert(w8_ulong symbol);
        SR_DLL_IMPORT w8_ulong getNumSymbols() const;
        SR_DLL_IMPORT w8_ulong getSymbolValue(w8_ulong index) const;
        SR_DLL_IMPORT w8_ulong getSymbolFrequency(w8_ulong index) const;

    private:
        srHashTable<w8_ulong, int> table;
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
            w8_ulong symbol;
            w8_ulong frequency;
            w8_ulong code;
            w8_ulong bits;
            Node* next;
            Node* children[2];
        };

        SR_DLL_IMPORT Compressor(const Sampler& sampler);
        SR_DLL_IMPORT ~Compressor();

        SR_DLL_IMPORT void storeSymbolTable(BitOStream& stream);
        SR_DLL_IMPORT void buildSymbolTree();
        SR_DLL_IMPORT void collectSymbols(const Sampler& sampler);

        // FUNCTION: SURRENDER 0x10001430
        void compressSymbol(BitOStream& stream, w8_ulong symbol)
        {
            Node* node = table.Lookup(&symbol);
            if (node != 0) {
                stream.put(node->code, node->bits);
            }
        }

        srHashTable<w8_ulong, Node*> table;
        Node* nodes;
        Node* free_list;
        Node* root;
        w8_ulong num_symbols;
        w8_ulong code_width;
        w8_ulong total;

    private:
        Compressor(const Compressor& other);

        void dumpNode(BitOStream& stream, Node* node);

    public:
        void setupPath(Node* node, w8_ulong code, w8_ulong depth);
    };

    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Decompressor {
    public:
        SR_DLL_IMPORT Decompressor(BitIStream& stream);
        SR_DLL_IMPORT ~Decompressor();
        SR_DLL_IMPORT w8_ulong decompressSymbol();
        SR_DLL_IMPORT w8_ulong getDataCount() const;

    private:
        struct Symbol {
            w8_ulong symbol;
            Symbol* children[2];
        };

        Decompressor(const Decompressor& other);
        Decompressor& operator=(const Decompressor& other);

        void setupSymbolTable(Symbol* node);

        BitIStream* stream;
        Symbol* symbols;
        w8_ulong next_node;
        w8_ulong code_width;
        w8_ulong unknown_10;
        w8_ulong num_symbols;
        w8_ulong data_count;
        Symbol* lookup[0x100];
        unsigned char depth[0x100];
    };
};

W8_ABI_ASSERT(sizeof(srHuffman::Compressor::Node) == 0x1c,
              "srHuffman_Compressor_Node_must_be_0x1c");
W8_ABI_ASSERT(sizeof(srHuffman::BitIStream) == 0x8c, "srHuffman_BitIStream_must_be_0x8c");
W8_ABI_ASSERT(sizeof(srHuffman::BitOStream) == 0x54, "srHuffman_BitOStream_must_be_0x54");
W8_ABI_ASSERT(sizeof(srHuffman::Sampler) == 0x1c, "srHuffman_Sampler_must_be_0x1c");
W8_ABI_ASSERT(sizeof(srHuffman::Sampler::Symbol) == 0x08, "srHuffman_Sampler_Symbol_must_be_0x08");
W8_ABI_ASSERT(sizeof(srHuffman::Compressor) == 0x28, "srHuffman_Compressor_must_be_0x28");
W8_ABI_ASSERT(sizeof(srHuffman::Decompressor) == 0x51c, "srHuffman_Decompressor_must_be_0x51c");
