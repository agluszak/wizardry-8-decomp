#include "surrender/srHuffman.h"

#include <string.h>

static void sortSymbolPairs(srHuffman::Sampler::Symbol* pairs, unsigned long count);
static void* copyMemory(void* destination, const void* source, long size);

// FUNCTION: SURRENDER 0x100010A0
srHuffman::BitIStream::BitIStream(srBinIStream& stream)
{
    stream_00 = &stream;
    cache_base_84 = -0x100;
    bit_pos_88 = stream_00->tell() << 3;
}

// FUNCTION: SURRENDER 0x100010E0
inline unsigned long srHuffman::BitIStream::get(unsigned long bits)
{
    if (bits <= 0x10) {
        return getWordOrLess(bits);
    }
    unsigned long low = getWordOrLess(0x10);
    unsigned long high = getWordOrLess(bits - 0x10);
    return (high << 0x10) | low;
}

// FUNCTION: SURRENDER 0x10001290
inline unsigned long srHuffman::BitIStream::getBit()
{
    unsigned long bit = (getByte(bit_pos_88 / 8) >> (bit_pos_88 & 7)) & 1;
    ++bit_pos_88;
    return bit;
}

// FUNCTION: SURRENDER 0x10001320
inline void srHuffman::BitIStream::rewind(long bits)
{
    bit_pos_88 -= bits;
}

// FUNCTION: SURRENDER 0x10001340
inline unsigned long srHuffman::BitIStream::getWordOrLess(unsigned long bits)
{
    long position = bit_pos_88 / 8;
    unsigned long shift = bit_pos_88 & 7;
    unsigned long data = getDWord(position);
    bit_pos_88 += bits;
    return (data & ((1 << (shift + bits)) - 1)) >> shift;
}

// FUNCTION: SURRENDER 0x100013D0
inline void srHuffman::BitIStream::fetchCache(long position)
{
    cache_base_84 = position;
    stream_00->seek(position);
    stream_00->read(cache_04, 0x80);
    stream_00->setState(srBinStream::SR_STREAM_OK);
}

// FUNCTION: SURRENDER 0x10001420
srHuffman::BitOStream& srHuffman::BitOStream::operator=(const BitOStream& stream)
{
    return *this;
}

// FUNCTION: SURRENDER 0x10001490
inline unsigned long srHuffman::BitIStream::getDWord(long position)
{
    unsigned long offset = position - cache_base_84;
    if (offset >= 0x7d) {
        fetchCache(position);
        offset = 0;
    }
    /* reinterpret-ok: the byte cache is read as an unaligned dword. */
    return *reinterpret_cast<unsigned long*>(cache_04 + offset);
}

// FUNCTION: SURRENDER 0x100014F0
srHuffman::Sampler::Sampler(const Sampler& other) : table_00(other.table_00)
{
    symbols_10 = other.symbols_10;
    count_18 = other.count_18;
}

// FUNCTION: SURRENDER 0x100015C0
inline unsigned long srHuffman::BitIStream::getByte(long position)
{
    unsigned long offset = position - cache_base_84;
    if (offset >= 0x80) {
        fetchCache(position);
        offset = 0;
    }
    return cache_04[offset];
}

// FUNCTION: SURRENDER 0x10001630
srHuffman::Sampler& srHuffman::Sampler::operator=(const Sampler& other)
{
    table_00 = other.table_00;
    symbols_10 = other.symbols_10;
    count_18 = other.count_18;
    return *this;
}

// FUNCTION: SURRENDER 0x100016D0
srHuffman::BitOStream::BitOStream(srBinOStream& stream)
{
    stream_00 = &stream;
    pending_0c = 0;
    bit_count_08 = 0;
    bytes_04 = 0;
    buffered_50 = 0;
}

// FUNCTION: SURRENDER 0x100016F0
srHuffman::Sampler::~Sampler() {}

// FUNCTION: SURRENDER 0x10001730
srHuffman::BitOStream::~BitOStream()
{
    flush();
}

// FUNCTION: SURRENDER 0x10001740
srHuffman::Compressor& srHuffman::Compressor::operator=(const Compressor& other)
{
    memcpy(this, &other, sizeof(Compressor));
    return *this;
}

// FUNCTION: SURRENDER 0x10001760
void srHuffman::BitOStream::put(unsigned long value, unsigned long bits)
{
    for (; bits > 0; --bits) {
        putBit(value & 1);
        value >>= 1;
    }
}

// FUNCTION: SURRENDER 0x10001790
void srHuffman::BitOStream::flush()
{
    flushByte();
    flushBuffer();
}

// FUNCTION: SURRENDER 0x100017B0
void srHuffman::BitOStream::flushByte()
{
    if (bit_count_08 != 0) {
        buffer_10[buffered_50] = (unsigned char)pending_0c;
        ++bytes_04;
        ++buffered_50;
        pending_0c = 0;
        bit_count_08 = 0;
        if (buffered_50 == 0x40) {
            flushBuffer();
        }
    }
}

// FUNCTION: SURRENDER 0x100017F0
void srHuffman::BitOStream::flushBuffer()
{
    if (buffered_50 != 0) {
        stream_00->write(buffer_10, buffered_50);
    }
    buffered_50 = 0;
}

// FUNCTION: SURRENDER 0x10001810
void srHuffman::BitOStream::putBit(unsigned long bit)
{
    pending_0c |= (bit & 1) << bit_count_08;
    ++bit_count_08;
    if (bit_count_08 == 8) {
        flushByte();
    }
}

// FUNCTION: SURRENDER 0x10001840
srHuffman::Sampler::Sampler() : count_18(0) {}

// FUNCTION: SURRENDER 0x10001990
void srHuffman::Sampler::insert(unsigned long symbol)
{
    if (table_00.FindNextEntry(&symbol, -1) == -1) {
        symbols_10[count_18].symbol_00 = symbol;
        symbols_10[count_18].frequency_04 = 1;
        table_00.Insert(&symbol, &count_18);
        ++count_18;
    } else {
        ++symbols_10[table_00.Lookup(&symbol)].frequency_04;
    }
}

// FUNCTION: SURRENDER 0x10001BA0
unsigned long srHuffman::Sampler::getNumSymbols() const
{
    return count_18;
}

// FUNCTION: SURRENDER 0x10001BB0
unsigned long srHuffman::Sampler::getSymbolValue(unsigned long index) const
{
    return symbols_10.data[index].symbol_00;
}

// FUNCTION: SURRENDER 0x10001BC0
unsigned long srHuffman::Sampler::getSymbolFrequency(unsigned long index) const
{
    return symbols_10.data[index].frequency_04;
}

// FUNCTION: SURRENDER 0x10001BD0
srHuffman::Compressor::Compressor(const Sampler& sampler)
{
    num_symbols_1c = sampler.getNumSymbols();
    code_width_20 = 0;
    total_24 = 0;
    nodes_10 = 0;
    root_18 = 0;
    free_list_14 = 0;
    if (num_symbols_1c != 0) {
        nodes_10 = static_cast<Node*>(::operator new(num_symbols_1c * 2 * sizeof(Node)));
        free_list_14 = nodes_10;
        for (unsigned long index = 0; index < num_symbols_1c * 2; ++index) {
            nodes_10[index].symbol_00 = 0;
            nodes_10[index].frequency_04 = 0;
            nodes_10[index].next_10 = nodes_10 + index + 1;
            nodes_10[index].children_14[0] = 0;
            nodes_10[index].children_14[1] = 0;
            nodes_10[index].code_08 = 0;
            nodes_10[index].bits_0c = 0;
        }
        nodes_10[num_symbols_1c - 1].next_10 = 0;
        free_list_14 = nodes_10 + num_symbols_1c;
        nodes_10[num_symbols_1c * 2 - 1].next_10 = 0;
        collectSymbols(sampler);
        buildSymbolTree();
        total_24 = 0;
        setupPath(root_18, 0, 0);
    }
}

// FUNCTION: SURRENDER 0x10001E10
srHuffman::Compressor::~Compressor()
{
    ::operator delete(nodes_10);
}

// FUNCTION: SURRENDER 0x10001E40
void srHuffman::Compressor::storeSymbolTable(BitOStream& stream)
{
    dumpNode(stream, root_18);
}

// FUNCTION: SURRENDER 0x10001E60
void srHuffman::Compressor::dumpNode(BitOStream& stream, Node* node)
{
    while (node != 0 && node->children_14[0] != 0) {
        stream.put(0, 1);
        dumpNode(stream, node->children_14[0]);
        node = node->children_14[1];
    }
    if (node != 0) {
        stream.put(1, 1);
        stream.put(node->symbol_00, code_width_20);
    }
}

// FUNCTION: SURRENDER 0x10001EC0
void srHuffman::Compressor::setupPath(Node* node, unsigned long code, unsigned long depth)
{
    if (node != 0) {
        while (node->children_14[0] != 0) {
            setupPath(node->children_14[0], code, depth + 1);
            node = node->children_14[1];
            code |= 1 << depth;
            ++depth;
            if (node == 0) {
                return;
            }
        }
        node->code_08 = code;
        node->bits_0c = depth;
        table_00.Insert(&node->symbol_00, &node);
        total_24 += node->frequency_04 * node->bits_0c;
    }
}

// FUNCTION: SURRENDER 0x10001F90
void srHuffman::Compressor::buildSymbolTree()
{
    if (num_symbols_1c != 1) {
        Node* heads[2];
        Node* tails[2];
        Node* root = 0;
        heads[0] = nodes_10;
        heads[1] = 0;
        tails[1] = 0;
        for (unsigned long merged = 0; merged < num_symbols_1c - 1; ++merged) {
            Node* node = free_list_14;
            free_list_14 = node->next_10;
            node->next_10 = 0;
            Node** slot = node->children_14;
            for (int remaining = 2; remaining != 0; --remaining) {
                int which = 0;
                if (heads[1] != 0 &&
                    (heads[0] == 0 || heads[1]->frequency_04 <= heads[0]->frequency_04)) {
                    which = 1;
                }
                Node* picked = heads[which];
                *slot = picked;
                heads[which] = picked->next_10;
                if (heads[which] == 0) {
                    tails[which] = 0;
                }
                picked = *slot;
                ++slot;
                picked->next_10 = 0;
            }
            node->frequency_04 =
                node->children_14[1]->frequency_04 + node->children_14[0]->frequency_04;
            if (tails[1] == 0) {
                heads[1] = node;
            } else {
                tails[1]->next_10 = node;
            }
            root = heads[1];
            tails[1] = node;
        }
        root_18 = root;
    } else {
        root_18 = nodes_10;
    }
}

// FUNCTION: SURRENDER 0x10002080
void srHuffman::Compressor::collectSymbols(const Sampler& sampler)
{
    unsigned long* symbols = static_cast<unsigned long*>(srHeap.allocate(num_symbols_1c * 4));
    unsigned long* frequencies = static_cast<unsigned long*>(srHeap.allocate(num_symbols_1c * 4));
    unsigned long max_symbol = 0;
    unsigned long index;
    for (index = 0; index < num_symbols_1c; ++index) {
        symbols[index] = sampler.getSymbolValue(index);
        frequencies[index] = sampler.getSymbolFrequency(index);
        if (max_symbol < symbols[index]) {
            max_symbol = symbols[index];
        }
    }
    int width;
    if (max_symbol == 0) {
        width = -1;
    } else {
        width = 0;
        if ((max_symbol & 0xffff0000) != 0) {
            width = 0x10;
            max_symbol >>= 0x10;
        }
        if ((max_symbol & 0xff00) != 0) {
            width += 8;
            max_symbol >>= 8;
        }
        if ((max_symbol & 0xf0) != 0) {
            width += 4;
            max_symbol >>= 4;
        }
        if ((max_symbol & 0x0c) != 0) {
            width += 2;
            max_symbol >>= 2;
        }
        if ((max_symbol & 0x02) != 0) {
            width += 1;
        }
    }
    code_width_20 = width + 1;
    if (code_width_20 < 1) {
        code_width_20 = 1;
    }
    if (num_symbols_1c > 1) {
        Sampler::Symbol* pairs = static_cast<Sampler::Symbol*>(srHeap.allocate(num_symbols_1c * 8));
        unsigned long bulk = num_symbols_1c & ~3;
        for (index = 0; index < bulk; index += 4) {
            pairs[index].symbol_00 = symbols[index];
            pairs[index].frequency_04 = frequencies[index];
            pairs[index + 1].symbol_00 = symbols[index + 1];
            pairs[index + 1].frequency_04 = frequencies[index + 1];
            pairs[index + 2].symbol_00 = symbols[index + 2];
            pairs[index + 2].frequency_04 = frequencies[index + 2];
            pairs[index + 3].symbol_00 = symbols[index + 3];
            pairs[index + 3].frequency_04 = frequencies[index + 3];
        }
        for (; index < num_symbols_1c; ++index) {
            pairs[index].symbol_00 = symbols[index];
            pairs[index].frequency_04 = frequencies[index];
        }
        sortSymbolPairs(pairs, num_symbols_1c);
        for (index = 0; index < bulk; index += 4) {
            symbols[index] = pairs[index].symbol_00;
            frequencies[index] = pairs[index].frequency_04;
            symbols[index + 1] = pairs[index + 1].symbol_00;
            frequencies[index + 1] = pairs[index + 1].frequency_04;
            symbols[index + 2] = pairs[index + 2].symbol_00;
            frequencies[index + 2] = pairs[index + 2].frequency_04;
            symbols[index + 3] = pairs[index + 3].symbol_00;
            frequencies[index + 3] = pairs[index + 3].frequency_04;
        }
        for (; index < num_symbols_1c; ++index) {
            symbols[index] = pairs[index].symbol_00;
            frequencies[index] = pairs[index].frequency_04;
        }
        srHeap.free(pairs);
    }
    for (index = 0; index < num_symbols_1c; ++index) {
        nodes_10[index].symbol_00 = symbols[index];
        nodes_10[index].frequency_04 = frequencies[index];
    }
    srHeap.free(symbols);
    srHeap.free(frequencies);
}

// FUNCTION: SURRENDER 0x10002340
srHuffman::Decompressor::Decompressor(BitIStream& stream)
{
    symbols_04 = 0;
    next_node_08 = 0;
    stream_00 = &stream;
    num_symbols_14 = stream_00->get(0x20);
    code_width_0c = stream_00->get(6);
    data_count_18 = stream_00->get(0x20);
    if (num_symbols_14 != 0) {
        symbols_04 = static_cast<Symbol*>(::operator new(num_symbols_14 * 0x18));
        setupSymbolTable(symbols_04);
        for (unsigned long index = 0; index < 0x100; ++index) {
            Symbol* node = symbols_04;
            unsigned long depth = 0;
            do {
                Symbol* next = node->children_04[0];
                if (next == 0) {
                    break;
                }
                if ((index & (1 << depth)) != 0) {
                    next = node->children_04[1];
                }
                ++depth;
                node = next;
            } while (depth < 8);
            lookup_1c[index] = node;
            depth_41c[index] = (unsigned char)depth;
        }
    }
}

// FUNCTION: SURRENDER 0x100024F0
srHuffman::Decompressor::~Decompressor()
{
    ::operator delete(symbols_04);
}

// FUNCTION: SURRENDER 0x10002500
unsigned long srHuffman::Decompressor::getDataCount() const
{
    return data_count_18;
}

// FUNCTION: SURRENDER 0x10002510
unsigned long srHuffman::Decompressor::decompressSymbol()
{
    BitIStream* stream = stream_00;
    unsigned long key = stream->get(8);
    stream->rewind(8 - depth_41c[key]);
    Symbol* node = lookup_1c[key];
    while (node->children_04[0] != 0) {
        node = node->children_04[stream->getBit()];
    }
    return node->value_00;
}

// FUNCTION: SURRENDER 0x10002630
void srHuffman::Decompressor::setupSymbolTable(Symbol* node)
{
    while (true) {
        ++next_node_08;
        if (stream_00->get(1) != 0) {
            break;
        }
        node->value_00 = 0xffffffff;
        node->children_04[0] = &symbols_04[next_node_08];
        setupSymbolTable(node->children_04[0]);
        node->children_04[1] = &symbols_04[next_node_08];
        node = node->children_04[1];
    }
    node->value_00 = stream_00->get(code_width_0c);
    node->children_04[0] = 0;
    node->children_04[1] = 0;
}

// FUNCTION: SURRENDER 0x100027F0
static void sortSymbolPairs(srHuffman::Sampler::Symbol* pairs, unsigned long count)
{
    if (count <= 1) {
        return;
    }
    srHuffman::Sampler::Symbol* scratch =
        static_cast<srHuffman::Sampler::Symbol*>(srHeap.allocate(count * 8));
    unsigned long counts[0x100];
    srHuffman::Sampler::Symbol* src = pairs;
    srHuffman::Sampler::Symbol* dst = scratch;
    unsigned long bulk = count & ~3;
    for (unsigned long pass = 0; pass < 4; ++pass) {
        srZeroMemory(counts, sizeof(counts));
        /* reinterpret-ok: radix pass extracts byte `pass` of each key. */
        unsigned char* keys = reinterpret_cast<unsigned char*>(&src[0].frequency_04) + pass;
        unsigned long index = 0;
        for (; index < bulk; index += 4) {
            ++counts[keys[index * 8]];
            ++counts[keys[index * 8 + 8]];
            ++counts[keys[index * 8 + 0x10]];
            ++counts[keys[index * 8 + 0x18]];
        }
        for (; index < count; ++index) {
            ++counts[keys[index * 8]];
        }
        unsigned long position = 0;
        for (unsigned long bucket = 0; bucket < 0x100; bucket += 4) {
            unsigned long saved = counts[bucket];
            counts[bucket] = position;
            position += saved;
            saved = counts[bucket + 1];
            counts[bucket + 1] = position;
            position += saved;
            saved = counts[bucket + 2];
            counts[bucket + 2] = position;
            position += saved;
            saved = counts[bucket + 3];
            counts[bucket + 3] = position;
            position += saved;
        }
        for (index = 0; index < bulk; index += 4) {
            dst[counts[keys[index * 8]]++] = src[index];
            dst[counts[keys[index * 8 + 8]]++] = src[index + 1];
            dst[counts[keys[index * 8 + 0x10]]++] = src[index + 2];
            dst[counts[keys[index * 8 + 0x18]]++] = src[index + 3];
        }
        for (; index < count; ++index) {
            dst[counts[keys[index * 8]]++] = src[index];
        }
        srHuffman::Sampler::Symbol* swap = src;
        src = dst;
        dst = swap;
    }
    if (src != pairs) {
        copyMemory(pairs, src, count * 8);
    }
    srHeap.free(dst);
}

// FUNCTION: SURRENDER 0x10002A90
static void* copyMemory(void* destination, const void* source, long size)
{
    if (size <= 0) {
        return 0;
    }
    memcpy(destination, source, size);
    return destination;
}

/* srArray<Sampler::Symbol>::release — the shared teardown sr.dll emits out of
   line; reached from inlined setCapacity/operator[] expansions. */
// TEMPLATE: SURRENDER 0x100027D0
// srArray<srHuffman::Sampler::Symbol>::release

/* srArray<Sampler::Symbol>::setCapacity — called out of line by insert's
   existing-symbol path (operator[] grow check). */
// TEMPLATE: SURRENDER 0x10002AC0
// srArray<srHuffman::Sampler::Symbol>::setCapacity

/* srHashTable<unsigned long,int>::Grow — called out of line from AllocateEntry
   inside insert's inlined Insert. */
// TEMPLATE: SURRENDER 0x10002BA0
// srHashTable<unsigned long,int>::Grow

/* srHashTable<unsigned long,Compressor::Node*>::Grow — called out of line from
   AllocateEntry inside setupPath's inlined Insert. */
// TEMPLATE: SURRENDER 0x10002CF0
// srHashTable<unsigned long,srHuffman::Compressor::Node*>::Grow
