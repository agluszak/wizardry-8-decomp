#pragma once

#include "srArray.h"
#include "srBinIStream.h"
#include "srBinOStream.h"
#include "srHash.h"
#include "srHeap.h"

/* Nested Huffman types named by SR.DLL's mangled exports. Wizardry's only
   consumers are BitArray::Load / Save in Engine Code\BitArray.cpp, which
   compress the octree alpha-bit and prop-sun-bit indices (magic 0xDEADD00D).
   Stack extents come from those Wiz8 frames and the SR constructors:
   BitIStream 0x8c, BitOStream 0x54, Sampler 0x1c, Compressor 0x28,
   Decompressor 0x51c.

   Retail exports every defined member of each nested class, including the
   private helpers (AAE access codes) and Sampler's user-defined copy
   operations, so the classes are dllexport-ed when building the provider
   (the same convention as srDebugDD). Members whose bodies sit here are the
   ones Wiz8 never imports: compressSymbol is inlined by BitArray::Save, and
   BitOStream's assignment is the private noncopyable-style no-op.

   Sampler's prefix is the srHashTable<unsigned long, int> open hash; the
   same shared code lives on the Wiz8 side as W8HashTable. Save does not call
   the imported ~Sampler (IAT 0x005eb768 is only an EH thunk); it destroys the
   symbol buffer with W8OwnedPtr (~ 0x004701b0) and then the hash table.
   Compressor keeps the same hash prefix, mapping symbols to Node*. */
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

        /* Retail inlines these into get/getBit and the Decompressor bodies at
           every site; the standalone emissions exist only because the class
           is dllexport-ed. */
        inline void fetchCache(long position);
        inline unsigned long getByte(long position);
        inline unsigned long getDWord(long position);
        inline unsigned long getWordOrLess(unsigned long bits);

        srBinIStream* stream_00;
        unsigned char cache_04[0x80];
        long cache_base_84;
        long bit_pos_88;
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

        /* ~BitOStream tail-calls flush(); the remaining helpers are never
           inlined at retail call sites. */
        void flush();
        void flushByte();
        void flushBuffer();
        void putBit(unsigned long bit);

        srBinOStream* stream_00;
        unsigned long bytes_04;
        unsigned long bit_count_08;
        unsigned long pending_0c;
        unsigned char buffer_10[0x40];
        unsigned long buffered_50;
    };

    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Sampler {
    public:
        struct Symbol {
            unsigned long symbol_00;
            unsigned long frequency_04;
        };

        SR_DLL_IMPORT Sampler();
        SR_DLL_IMPORT Sampler(const Sampler& other);
        SR_DLL_IMPORT ~Sampler();
        SR_DLL_IMPORT Sampler& operator=(const Sampler& other);
        SR_DLL_IMPORT void insert(unsigned long symbol);
        SR_DLL_IMPORT unsigned long getNumSymbols() const;
        SR_DLL_IMPORT unsigned long getSymbolValue(unsigned long index) const;
        SR_DLL_IMPORT unsigned long getSymbolFrequency(unsigned long index) const;

    private:
        srHashTable<unsigned long, int> table_00;
        srArray<Symbol> symbols_10;
        int count_18;
    };

    class
#if defined(SURRENDER_BUILD)
        __declspec(dllexport)
#endif
        Compressor {
    public:
        struct Node {
            unsigned long symbol_00;
            unsigned long frequency_04;
            unsigned long code_08;
            unsigned long bits_0c;
            Node* next_10;
            Node* children_14[2];
        };

        SR_DLL_IMPORT Compressor(const Sampler& sampler);
        SR_DLL_IMPORT ~Compressor();
        SR_DLL_IMPORT Compressor& operator=(const Compressor& other);
        SR_DLL_IMPORT void storeSymbolTable(BitOStream& stream);
        SR_DLL_IMPORT void buildSymbolTree();
        SR_DLL_IMPORT void collectSymbols(const Sampler& sampler);

        /* Wiz8 does not import compressSymbol; Save inlines the hash walk and
           the put here. */
        // FUNCTION: SURRENDER 0x10001430
        void compressSymbol(BitOStream& stream, unsigned long symbol)
        {
            Node* node = table_00.Lookup(&symbol);
            if (node != 0) {
                stream.put(node->code_08, node->bits_0c);
            }
        }

        srHashTable<unsigned long, Node*> table_00;
        Node* nodes_10;
        Node* free_list_14;
        Node* root_18;
        unsigned long num_symbols_1c;
        unsigned long code_width_20;
        unsigned long total_24;

    private:
        Compressor(const Compressor& other);

        void dumpNode(BitOStream& stream, Node* node);
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
            unsigned long value_00;
            Symbol* children_04[2];
        };

        Decompressor(const Decompressor& other);
        Decompressor& operator=(const Decompressor& other);

        void setupSymbolTable(Symbol* node);

        BitIStream* stream_00;
        Symbol* symbols_04;
        unsigned long next_node_08;
        unsigned long code_width_0c;
        unsigned long unknown_10;
        unsigned long num_symbols_14;
        unsigned long data_count_18;
        Symbol* lookup_1c[0x100];
        unsigned char depth_41c[0x100];
    };
};

static_assert(sizeof(srHuffman::Compressor::Node) == 0x1c,
              "srHuffman_Compressor_Node_must_be_0x1c");
static_assert(sizeof(srHuffman::BitIStream) == 0x8c, "srHuffman_BitIStream_must_be_0x8c");
static_assert(sizeof(srHuffman::BitOStream) == 0x54, "srHuffman_BitOStream_must_be_0x54");
static_assert(sizeof(srHuffman::Sampler) == 0x1c, "srHuffman_Sampler_must_be_0x1c");
static_assert(sizeof(srHuffman::Compressor) == 0x28, "srHuffman_Compressor_must_be_0x28");
static_assert(sizeof(srHuffman::Decompressor) == 0x51c, "srHuffman_Decompressor_must_be_0x51c");
