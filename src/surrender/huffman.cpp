#include "surrender/srHuffman.h"

#include <string.h>

static void sortSymbolPairs(srHuffman::Sampler::Symbol* pairs, unsigned long count);
static void* copyMemory(void* destination, const void* source, long size);

// FUNCTION: SURRENDER 0x100013D0
void srHuffman::BitIStream::fetchCache(long position)
{
    cache_base = position;
    stream->seek(position);
    stream->read(cache, 0x80);
    stream->setState(srBinStream::SR_STREAM_OK);
}

// FUNCTION: SURRENDER 0x100015C0
unsigned long srHuffman::BitIStream::getByte(long position)
{
    unsigned long offset = position - cache_base;
    if (offset >= 0x80) {
        fetchCache(position);
        offset = 0;
    }
    return cache[offset];
}

// FUNCTION: SURRENDER 0x10001490
unsigned long srHuffman::BitIStream::getDWord(long position)
{
    unsigned long offset = position - cache_base;
    if (offset >= 0x7d) {
        fetchCache(position);
        offset = 0;
    }
    /* reinterpret-ok: the byte cache is read as an unaligned dword. */
    return *reinterpret_cast<unsigned long*>(cache + offset);
}

// FUNCTION: SURRENDER 0x10001340
unsigned long srHuffman::BitIStream::getWordOrLess(unsigned long bits)
{
    long position = bit_pos / 8;
    unsigned long shift = bit_pos & 7;
    unsigned long data = getDWord(position);
    bit_pos += bits;
    return (data & ((1 << (shift + bits)) - 1)) >> shift;
}

// FUNCTION: SURRENDER 0x100010A0
srHuffman::BitIStream::BitIStream(srBinIStream& stream)
{
    this->stream = &stream;
    cache_base = -0x100;
    bit_pos = this->stream->tell() << 3;
}

// FUNCTION: SURRENDER 0x100010E0
unsigned long srHuffman::BitIStream::get(unsigned long bits)
{
    if (bits <= 0x10) {
        return getWordOrLess(bits);
    }
    unsigned long low = getWordOrLess(0x10);
    unsigned long high = getWordOrLess(bits - 0x10);
    return (high << 0x10) | low;
}

// FUNCTION: SURRENDER 0x10001290
unsigned long srHuffman::BitIStream::getBit()
{
    unsigned long data = getByte(bit_pos / 8);
    unsigned long bit = (data >> (bit_pos & 7)) & 1;
    ++bit_pos;
    return bit;
}

// FUNCTION: SURRENDER 0x10001320
void srHuffman::BitIStream::rewind(long bits)
{
    bit_pos -= bits;
}

// FUNCTION: SURRENDER 0x10001420
srHuffman::BitOStream& srHuffman::BitOStream::operator=(const BitOStream& stream)
{
    return *this;
}

// FUNCTION: SURRENDER 0x100016D0
srHuffman::BitOStream::BitOStream(srBinOStream& stream)
{
    this->stream = &stream;
    pending = 0;
    bit_count = 0;
    bytes = 0;
    buffered = 0;
}

// FUNCTION: SURRENDER 0x10001730
srHuffman::BitOStream::~BitOStream()
{
    flush();
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
    if (bit_count != 0) {
        buffer[buffered] = (unsigned char)pending;
        ++bytes;
        ++buffered;
        pending = 0;
        bit_count = 0;
        if (buffered == 0x40) {
            flushBuffer();
        }
    }
}

// FUNCTION: SURRENDER 0x100017F0
void srHuffman::BitOStream::flushBuffer()
{
    if (buffered != 0) {
        stream->write(buffer, buffered);
    }
    buffered = 0;
}

// FUNCTION: SURRENDER 0x10001810
void srHuffman::BitOStream::putBit(unsigned long bit)
{
    pending |= (bit & 1) << bit_count;
    ++bit_count;
    if (bit_count == 8) {
        flushByte();
    }
}

// FUNCTION: SURRENDER 0x10001840
srHuffman::Sampler::Sampler() : count(0) {}

// FUNCTION: SURRENDER 0x10001990
void srHuffman::Sampler::insert(unsigned long symbol)
{
    if (table.FindNextEntry(&symbol, -1) == -1) {
        symbols[count].symbol = symbol;
        symbols[count].frequency = 1;
        table.Insert(&symbol, &count);
        ++count;
    } else {
        ++symbols[table.Lookup(&symbol)].frequency;
    }
}

// FUNCTION: SURRENDER 0x10001BA0
unsigned long srHuffman::Sampler::getNumSymbols() const
{
    return count;
}

// FUNCTION: SURRENDER 0x10001BB0
unsigned long srHuffman::Sampler::getSymbolValue(unsigned long index) const
{
    return symbols.data[index].symbol;
}

// FUNCTION: SURRENDER 0x10001BC0
unsigned long srHuffman::Sampler::getSymbolFrequency(unsigned long index) const
{
    return symbols.data[index].frequency;
}

// FUNCTION: SURRENDER 0x10001BD0
srHuffman::Compressor::Compressor(const Sampler& sampler)
{
    num_symbols = sampler.getNumSymbols();
    code_width = 0;
    total = 0;
    nodes = 0;
    root = 0;
    free_list = 0;
    if (num_symbols != 0) {
        nodes = static_cast<Node*>(::operator new(num_symbols * 2 * sizeof(Node)));
        free_list = nodes;
        for (unsigned long index = 0; index < num_symbols * 2; ++index) {
            nodes[index].symbol = 0;
            nodes[index].frequency = 0;
            nodes[index].next = nodes + index + 1;
            nodes[index].children[0] = 0;
            nodes[index].children[1] = 0;
            nodes[index].code = 0;
            nodes[index].bits = 0;
        }
        nodes[num_symbols - 1].next = 0;
        free_list = nodes + num_symbols;
        nodes[num_symbols * 2 - 1].next = 0;
        collectSymbols(sampler);
        buildSymbolTree();
        total = 0;
        setupPath(root, 0, 0);
    }
}

// FUNCTION: SURRENDER 0x10001E10
srHuffman::Compressor::~Compressor()
{
    ::operator delete(nodes);
}

// FUNCTION: SURRENDER 0x10001E40
void srHuffman::Compressor::storeSymbolTable(BitOStream& stream)
{
    dumpNode(stream, root);
}

// FUNCTION: SURRENDER 0x10001E60
void srHuffman::Compressor::dumpNode(BitOStream& stream, Node* node)
{
    while (node != 0 && node->children[0] != 0) {
        stream.put(0, 1);
        dumpNode(stream, node->children[0]);
        node = node->children[1];
    }
    if (node != 0) {
        stream.put(1, 1);
        stream.put(node->symbol, code_width);
    }
}

// FUNCTION: SURRENDER 0x10001EC0
void srHuffman::Compressor::setupPath(Node* node, unsigned long code, unsigned long depth)
{
    if (node != 0) {
        while (node->children[0] != 0) {
            setupPath(node->children[0], code, depth + 1);
            node = node->children[1];
            code |= 1 << depth;
            ++depth;
            if (node == 0) {
                return;
            }
        }
        node->code = code;
        node->bits = depth;
        table.Insert(&node->symbol, &node);
        total += node->frequency * node->bits;
    }
}

// FUNCTION: SURRENDER 0x10001F90
void srHuffman::Compressor::buildSymbolTree()
{
    if (num_symbols != 1) {
        Node* heads[2];
        Node* tails[2];
        Node* root = 0;
        heads[0] = nodes;
        heads[1] = 0;
        tails[1] = 0;
        for (unsigned long merged = 0; merged < num_symbols - 1; ++merged) {
            Node* node = free_list;
            free_list = node->next;
            node->next = 0;
            Node** slot = node->children;
            for (int remaining = 2; remaining != 0; --remaining) {
                int which = 0;
                if (heads[1] != 0 &&
                    (heads[0] == 0 || heads[1]->frequency <= heads[0]->frequency)) {
                    which = 1;
                }
                Node* picked = heads[which];
                *slot = picked;
                heads[which] = picked->next;
                if (heads[which] == 0) {
                    tails[which] = 0;
                }
                picked = *slot;
                ++slot;
                picked->next = 0;
            }
            node->frequency = node->children[1]->frequency + node->children[0]->frequency;
            if (tails[1] == 0) {
                heads[1] = node;
            } else {
                tails[1]->next = node;
            }
            root = heads[1];
            tails[1] = node;
        }
        this->root = root;
    } else {
        this->root = nodes;
    }
}

// FUNCTION: SURRENDER 0x10002080
void srHuffman::Compressor::collectSymbols(const Sampler& sampler)
{
    unsigned long* symbols = static_cast<unsigned long*>(srHeap.allocate(num_symbols * 4));
    unsigned long* frequencies = static_cast<unsigned long*>(srHeap.allocate(num_symbols * 4));
    unsigned long max_symbol = 0;
    unsigned long index;
    for (index = 0; index < num_symbols; ++index) {
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
    code_width = width + 1;
    if (code_width < 1) {
        code_width = 1;
    }
    if (num_symbols > 1) {
        Sampler::Symbol* pairs = static_cast<Sampler::Symbol*>(srHeap.allocate(num_symbols * 8));
        unsigned long bulk = num_symbols & ~3;
        for (index = 0; index < bulk; index += 4) {
            pairs[index].symbol = symbols[index];
            pairs[index].frequency = frequencies[index];
            pairs[index + 1].symbol = symbols[index + 1];
            pairs[index + 1].frequency = frequencies[index + 1];
            pairs[index + 2].symbol = symbols[index + 2];
            pairs[index + 2].frequency = frequencies[index + 2];
            pairs[index + 3].symbol = symbols[index + 3];
            pairs[index + 3].frequency = frequencies[index + 3];
        }
        for (; index < num_symbols; ++index) {
            pairs[index].symbol = symbols[index];
            pairs[index].frequency = frequencies[index];
        }
        sortSymbolPairs(pairs, num_symbols);
        for (index = 0; index < bulk; index += 4) {
            symbols[index] = pairs[index].symbol;
            frequencies[index] = pairs[index].frequency;
            symbols[index + 1] = pairs[index + 1].symbol;
            frequencies[index + 1] = pairs[index + 1].frequency;
            symbols[index + 2] = pairs[index + 2].symbol;
            frequencies[index + 2] = pairs[index + 2].frequency;
            symbols[index + 3] = pairs[index + 3].symbol;
            frequencies[index + 3] = pairs[index + 3].frequency;
        }
        for (; index < num_symbols; ++index) {
            symbols[index] = pairs[index].symbol;
            frequencies[index] = pairs[index].frequency;
        }
        srHeap.free(pairs);
    }
    for (index = 0; index < num_symbols; ++index) {
        nodes[index].symbol = symbols[index];
        nodes[index].frequency = frequencies[index];
    }
    srHeap.free(symbols);
    srHeap.free(frequencies);
}

// FUNCTION: SURRENDER 0x10002340
srHuffman::Decompressor::Decompressor(BitIStream& stream)
{
    symbols = 0;
    next_node = 0;
    this->stream = &stream;
    num_symbols = this->stream->get(0x20);
    code_width = this->stream->get(6);
    data_count = this->stream->get(0x20);
    if (num_symbols != 0) {
        symbols = static_cast<Symbol*>(::operator new(num_symbols * 0x18));
        setupSymbolTable(symbols);
        for (unsigned long index = 0; index < 0x100; ++index) {
            Symbol* node = symbols;
            unsigned long depth;
            for (depth = 0; depth < 8; ++depth) {
                Symbol* next = node->children[0];
                if (next == 0) {
                    break;
                }
                if ((index & (1 << depth)) != 0) {
                    next = node->children[1];
                }
                node = next;
            }
            lookup[index] = node;
            this->depth[index] = (unsigned char)depth;
        }
    }
}

// FUNCTION: SURRENDER 0x100024F0
srHuffman::Decompressor::~Decompressor()
{
    ::operator delete(symbols);
}

// FUNCTION: SURRENDER 0x10002500
unsigned long srHuffman::Decompressor::getDataCount() const
{
    return data_count;
}

// FUNCTION: SURRENDER 0x10002510
unsigned long srHuffman::Decompressor::decompressSymbol()
{
    BitIStream* stream = this->stream;
    unsigned long key = stream->get(8);
    stream->rewind(8 - depth[key]);
    Symbol* node = lookup[key];
    while (node->children[0] != 0) {
        node = node->children[stream->getBit()];
    }
    return node->symbol;
}

// FUNCTION: SURRENDER 0x10002630
void srHuffman::Decompressor::setupSymbolTable(Symbol* node)
{
    while (true) {
        ++next_node;
        if (stream->get(1) != 0) {
            break;
        }
        node->symbol = 0xffffffff;
        node->children[0] = &symbols[next_node];
        setupSymbolTable(node->children[0]);
        node->children[1] = &symbols[next_node];
        node = node->children[1];
    }
    node->symbol = stream->get(code_width);
    node->children[0] = 0;
    node->children[1] = 0;
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
        unsigned char* keys = reinterpret_cast<unsigned char*>(&src[0].frequency) + pass;
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
