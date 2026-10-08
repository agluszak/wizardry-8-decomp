/* Differential cases for the exported binary streams. Writers dump every
   byte they produce as "blob <id> <hex>" lines; readers decode the same
   layout with the getters and every operator>>. With --blobs FILE the
   stream.xread.* cases decode another variant's blobs instead, so the driver
   can check retail-written bytes read by the rebuilt DLL and vice versa. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "surrender/srBinFStream.h"
#include "surrender/srBinIStream.h"
#include "surrender/srBinOStream.h"

void runProbe(const char* name, void (*probe)());

const char* g_blob_file = 0;

struct Blob {
    char id[48];
    unsigned char* data;
    unsigned long size;
};

static Blob g_peer_blobs[64];
static int g_peer_blob_count = -1;

static void loadPeerBlobs()
{
    FILE* file;
    static char line[0x20000];
    if (g_peer_blob_count >= 0) {
        return;
    }
    g_peer_blob_count = 0;
    file = g_blob_file ? fopen(g_blob_file, "r") : 0;
    if (file == 0) {
        return;
    }
    while (fgets(line, sizeof(line), file) != 0 && g_peer_blob_count < 64) {
        char id[48];
        char* hex;
        unsigned long length;
        unsigned long index;
        Blob& blob = g_peer_blobs[g_peer_blob_count];
        if (strncmp(line, "blob ", 5) != 0 || sscanf(line + 5, "%47s", id) != 1) {
            continue;
        }
        hex = strchr(line + 5, ' ');
        hex = hex ? hex + 1 : line + strlen(line);
        length = static_cast<unsigned long>(strspn(hex, "0123456789abcdef")) / 2;
        strcpy(blob.id, id);
        blob.data = static_cast<unsigned char*>(malloc(length + 1));
        for (index = 0; index < length; ++index) {
            unsigned int value;
            sscanf(hex + index * 2, "%2x", &value);
            blob.data[index] = static_cast<unsigned char>(value);
        }
        blob.size = length;
        ++g_peer_blob_count;
    }
    fclose(file);
}

static const Blob* findPeerBlob(const char* id)
{
    int index;
    loadPeerBlobs();
    for (index = 0; index < g_peer_blob_count; ++index) {
        if (strcmp(g_peer_blobs[index].id, id) == 0) {
            return &g_peer_blobs[index];
        }
    }
    return 0;
}

static void printBlob(const char* id, const unsigned char* data, unsigned long size)
{
    unsigned long index;
    printf("blob %s ", id);
    for (index = 0; index < size; ++index) {
        printf("%02x", data[index]);
    }
    printf("\n");
}

static unsigned long g_stream_rng = 1;

static unsigned long streamRandom()
{
    g_stream_rng ^= g_stream_rng << 13;
    g_stream_rng ^= g_stream_rng >> 17;
    g_stream_rng ^= g_stream_rng << 5;
    return g_stream_rng;
}

/* The scalar layout written by stream.write.scalars and decoded by the
   readers. Floats and doubles include zeros, denormals, infinities and
   quiet and signalling NaNs. */
struct ScalarOp {
    char kind;
    unsigned long lo;
    unsigned long hi;
};

static const ScalarOp g_scalar_ops[] = {
    {'c', 0x00, 0},
    {'c', 0x7f, 0},
    {'c', 0x80, 0},
    {'c', 0xff, 0},
    {'w', 0x0000, 0},
    {'w', 0x1234, 0},
    {'w', 0xfffe, 0},
    {'d', 0x00000000, 0},
    {'d', 0x12345678, 0},
    {'d', 0xffffffff, 0},
    {'d', 0x80000001, 0},
    {'f', 0x3f800000, 0},
    {'f', 0x80000000, 0},
    {'f', 0x00000001, 0},
    {'f', 0x7f800000, 0},
    {'f', 0xff800000, 0},
    {'f', 0x7fc00001, 0},
    {'f', 0x7f800001, 0}, /* signalling NaN */
    {'f', 0xffbfffff, 0}, /* signalling NaN, negative */
    {'D', 0x00000000, 0x3ff00000},
    {'D', 0x00000001, 0x00000000},
    {'D', 0x00000000, 0x7ff00000},
    {'D', 0x00000001, 0x7ff00000}, /* signalling NaN */
    {'D', 0xffffffff, 0xfff7ffff}, /* signalling NaN, negative */
    {'D', 0x54442d18, 0x400921fb},
    {'q', 0x89abcdef, 0x01234567},
    {'q', 0xffffffff, 0x00000000},
    {'b', 0, 0},
    {'b', 1, 0},
    {'b', 7, 0},
    {'c', 0x41, 0},
};

static const int g_scalar_op_count = sizeof(g_scalar_ops) / sizeof(g_scalar_ops[0]);

static unsigned long bitsOfFloat(float value)
{
    unsigned long bits;
    memcpy(&bits, &value, 4);
    return bits;
}

static float floatOfBits(unsigned long bits)
{
    float value;
    memcpy(&value, &bits, 4);
    return value;
}

static void writeScalars(srBinOStream& stream)
{
    int index;
    for (index = 0; index < g_scalar_op_count; ++index) {
        const ScalarOp& op = g_scalar_ops[index];
        switch (op.kind) {
        case 'c':
            stream.putChar(static_cast<char>(op.lo));
            break;
        case 'w':
            stream.putWord(static_cast<unsigned short>(op.lo));
            break;
        case 'd':
            stream.putDWord(op.lo);
            break;
        case 'f': {
            /* putFloat takes the value on the x87 stack; signalling NaNs are
               passed by memory so the runner does not quiet them first. */
            volatile unsigned long bits = op.lo;
            stream.putFloat(*reinterpret_cast<volatile float*>(&bits));
            break;
        }
        case 'D': {
            volatile unsigned long bits[2];
            bits[0] = op.lo;
            bits[1] = op.hi;
            stream.putDouble(*reinterpret_cast<volatile double*>(bits));
            break;
        }
        case 'q': {
            srQuadWord value;
            value.lo = op.lo;
            value.hi = op.hi;
            stream.putQWord(value);
            break;
        }
        case 'b': {
            static const unsigned char pattern[] = {0xde, 0xad, 0xbe, 0xef, 0x01, 0x02, 0x03, 0x04};
            stream.write(pattern, op.lo);
            break;
        }
        }
    }
}

static void printStreamState(const char* label, srBinStream& stream)
{
    printf("%s good %d not %d void %d order %d\n", label, stream.good() ? 1 : 0, !stream ? 1 : 0,
           static_cast<void*>(stream) != 0 ? 1 : 0, static_cast<int>(stream.getByteOrder()));
}

static void decodeScalars(const unsigned char* data, unsigned long size, int order)
{
    srBinIMStream stream(data, size);
    int index;
    stream.setByteOrder(static_cast<srBinStream::e_byteOrder>(order));
    printStreamState("open", stream);
    printf("size %lu tell %lu\n", stream.getSize(), stream.tell());
    for (index = 0; index < g_scalar_op_count; ++index) {
        const ScalarOp& op = g_scalar_ops[index];
        switch (op.kind) {
        case 'c':
            printf("%d char %04x\n", index, stream.getChar());
            break;
        case 'w':
            printf("%d word %04x\n", index, stream.getWord());
            break;
        case 'd':
            printf("%d dword %08lx\n", index, stream.getDWord());
            break;
        case 'f': {
            volatile float value = stream.getFloat();
            printf("%d float %08lx\n", index, bitsOfFloat(value));
            break;
        }
        case 'D': {
            volatile double value = stream.getDouble();
            unsigned long bits[2];
            memcpy(bits, const_cast<double*>(&value), 8);
            printf("%d double %08lx%08lx\n", index, bits[1], bits[0]);
            break;
        }
        case 'q': {
            srQuadWord value = stream.getQuadWord();
            printf("%d quad %08x%08x\n", index, value.hi, value.lo);
            break;
        }
        case 'b': {
            unsigned char bytes[8];
            unsigned long byte;
            memset(bytes, 0xcc, sizeof(bytes));
            stream.read(bytes, op.lo);
            printf("%d bytes", index);
            for (byte = 0; byte < op.lo; ++byte) {
                printf(" %02x", bytes[byte]);
            }
            printf("\n");
            break;
        }
        }
        printf("  tell %lu good %d\n", stream.tell(), stream.good() ? 1 : 0);
    }
    /* Past the end. */
    printf("eof-char %04x\n", stream.getChar());
    printStreamState("after-eof", stream);
    printf("eof-dword %08lx\n", stream.getDWord());
    stream.clear();
    printStreamState("cleared", stream);
    stream.seek(2, srBinStream::SR_SEEK_END);
    printf("seek-end tell %lu word %04x\n", stream.tell(), stream.getWord());
    printf("short-word %04x\n", stream.getWord());
    printStreamState("short", stream);
    stream.clear();
    stream.seek(size + 1);
    printStreamState("seek-past", stream);
    stream.clear();
    printf("tell-after-seek-past %lu\n", stream.tell());
    stream.seek(1, srBinStream::SR_SEEK_BEGIN);
    stream.seek(3, srBinStream::SR_SEEK_CURRENT);
    printf("seek-current tell %lu dword %08lx\n", stream.tell(), stream.getDWord());
    bool old = stream.exceptions(true);
    printf("exceptions-old %d\n", old ? 1 : 0);
    try {
        stream.seek(0, srBinStream::SR_SEEK_END);
        unsigned char byte;
        stream.read(&byte, 1);
        printf("no-throw\n");
    } catch (srBinStream::Failure& failure) {
        printf("caught state %d\n", failure.state);
    }
    stream.exceptions(false);
    stream.clear();
    stream.seek(0);
}

template <class T> static void readAndPrint(srBinIStream& stream, const char* label)
{
    T value;
    unsigned long words[32];
    unsigned long index;
    memset(&value, 0xa5, sizeof(T));
    stream >> value;
    memcpy(words, &value, sizeof(T) <= sizeof(words) ? sizeof(T) : sizeof(words));
    printf("%s good %d", label, stream.good() ? 1 : 0);
    if (sizeof(T) < 4) {
        unsigned long small = 0;
        memcpy(&small, &value, sizeof(T));
        printf(" %0*lx", static_cast<int>(sizeof(T) * 2), small);
    } else {
        for (index = 0; index < sizeof(T) / 4; ++index) {
            printf(" %08lx", words[index]);
        }
    }
    printf("\n");
}

static void decodeOperators(const unsigned char* data, unsigned long size, int order)
{
    srBinIMStream stream(data, size);
    stream.setByteOrder(static_cast<srBinStream::e_byteOrder>(order));
    readAndPrint<int>(stream, "int");
    readAndPrint<char>(stream, "char");
    readAndPrint<unsigned char>(stream, "uchar");
    readAndPrint<short>(stream, "short");
    readAndPrint<unsigned short>(stream, "ushort");
    readAndPrint<long>(stream, "long");
    readAndPrint<unsigned long>(stream, "ulong");
    readAndPrint<srQuadWord>(stream, "quad");
    readAndPrint<float>(stream, "float");
    readAndPrint<double>(stream, "double");
    readAndPrint<srVector2T<float> >(stream, "vec2f");
    readAndPrint<srVector2T<double> >(stream, "vec2d");
    readAndPrint<srVector3T<float> >(stream, "vec3f");
    readAndPrint<srVector3T<double> >(stream, "vec3d");
    readAndPrint<srVector4T<float> >(stream, "vec4f");
    readAndPrint<srVector4T<double> >(stream, "vec4d");
    readAndPrint<srVector2i>(stream, "vec2i");
    readAndPrint<srVector3i>(stream, "vec3i");
    readAndPrint<srVector4i>(stream, "vec4i");
    readAndPrint<srQuaternion>(stream, "quat");
    readAndPrint<srMatrix2T<float> >(stream, "mat2f");
    readAndPrint<srMatrix3T<float> >(stream, "mat3f");
    readAndPrint<srMatrix4T<float> >(stream, "mat4f");
    readAndPrint<srMatrix2T<double> >(stream, "mat2d");
    readAndPrint<srMatrix3T<double> >(stream, "mat3d");
    readAndPrint<srMatrix4T<double> >(stream, "mat4d");
    /* Runs out of data part way through a matrix. */
    readAndPrint<srMatrix4T<double> >(stream, "mat4d-tail");
    printf("tell %lu\n", stream.tell());
}

static int g_stream_order;

static void writeScalarsCase()
{
    srBinOMStream stream;
    char id[48];
    stream.setByteOrder(static_cast<srBinStream::e_byteOrder>(g_stream_order));
    printStreamState("open", stream);
    printf("empty size %lu tell %lu\n", stream.getSize(), stream.tell());
    writeScalars(stream);
    printf("size %lu tell %lu\n", stream.getSize(), stream.tell());
    sprintf(id, "scalars.o%d", g_stream_order);
    printBlob(id, static_cast<unsigned char*>(stream.getPtr()), stream.getSize());
    /* Overwrite inside, then append after seeking from the end. */
    stream.seek(3);
    stream.putDWord(0xa1b2c3d4);
    printf("overwrite size %lu tell %lu\n", stream.getSize(), stream.tell());
    stream.seek(2, srBinStream::SR_SEEK_CURRENT);
    stream.putWord(0x5566);
    stream.seek(1, srBinStream::SR_SEEK_END);
    stream.putDWord(0x01020304);
    printf("append size %lu tell %lu\n", stream.getSize(), stream.tell());
    sprintf(id, "edited.o%d", g_stream_order);
    printBlob(id, static_cast<unsigned char*>(stream.getPtr()), stream.getSize());
}

static void readScalarsCase()
{
    srBinOMStream stream;
    stream.setByteOrder(static_cast<srBinStream::e_byteOrder>(g_stream_order));
    writeScalars(stream);
    decodeScalars(static_cast<unsigned char*>(stream.getPtr()), stream.getSize(), g_stream_order);
    printf("cross-order\n");
    decodeScalars(static_cast<unsigned char*>(stream.getPtr()), stream.getSize(),
                  1 - g_stream_order);
}

static unsigned char g_random_bytes[700];

static void fillRandomBytes()
{
    unsigned long index;
    g_stream_rng = 0x2545f491;
    for (index = 0; index < sizeof(g_random_bytes); ++index) {
        g_random_bytes[index] = static_cast<unsigned char>(streamRandom() >> 11);
    }
    /* Seed a few signalling-NaN floats and doubles at the float/double slots. */
    g_random_bytes[27] = 0x7f;
    g_random_bytes[26] = 0x80;
    g_random_bytes[25] = 0x00;
    g_random_bytes[24] = 0x01;
}

static void operatorsCase()
{
    fillRandomBytes();
    decodeOperators(g_random_bytes, sizeof(g_random_bytes), g_stream_order);
    /* Starting one byte in shifts every field. */
    decodeOperators(g_random_bytes + 1, 401, g_stream_order);
}

static void writeOperatorsBlobCase()
{
    char id[48];
    fillRandomBytes();
    sprintf(id, "random.o%d", g_stream_order);
    printBlob(id, g_random_bytes, sizeof(g_random_bytes));
}

static void emptyStreamCase()
{
    srBinIMStream empty(0, 0);
    printStreamState("null", empty);
    printf("null size %lu tell %lu char %04x\n", empty.getSize(), empty.tell(), empty.getChar());
    unsigned char one = 0x5a;
    srBinIMStream zero(&one, 0);
    printStreamState("zero-size", zero);
    srBinIMStream single(&one, 1);
    printf("single char %04x char %04x\n", single.getChar(), single.getChar());
    printStreamState("single", single);
    srBinOMStream output;
    output.write(&one, 0);
    printf("om empty-write size %lu tell %lu\n", output.getSize(), output.tell());
    output.seek(0, srBinStream::SR_SEEK_END);
    output.putChar(1);
    printf("om size %lu tell %lu\n", output.getSize(), output.tell());
    srBinOMStream large;
    unsigned long index;
    for (index = 0; index < 5000; ++index) {
        large.putDWord(index * 2654435761UL);
    }
    unsigned long sum = 0;
    const unsigned char* bytes = static_cast<const unsigned char*>(large.getPtr());
    for (index = 0; index < large.getSize(); ++index) {
        sum = sum * 31 + bytes[index];
    }
    printf("large size %lu tell %lu sum %08lx\n", large.getSize(), large.tell(), sum);
}

static void fileStreamCase()
{
    char directory[MAX_PATH];
    char path[MAX_PATH];
    GetTempPathA(sizeof(directory), directory);
    sprintf(path, "%ssr_difftest_stream.bin", directory);
    DeleteFileA(path);
    srBinOFStream* output = new srBinOFStream(path);
    printf("ofs open %d good %d\n", output->isOpen(), output->good() ? 1 : 0);
    output->setByteOrder(static_cast<srBinStream::e_byteOrder>(g_stream_order));
    writeScalars(*output);
    printf("ofs tell %lu size %lu\n", output->tell(), output->getSize());
    output->seek(3);
    output->putDWord(0xa1b2c3d4);
    output->seek(0, srBinStream::SR_SEEK_END);
    output->putWord(0x7788);
    printf("ofs tell %lu\n", output->tell());
    output->close();
    printf("ofs closed open %d\n", output->isOpen());
    FILE* file = fopen(path, "rb");
    unsigned char bytes[512];
    unsigned long size =
        file ? static_cast<unsigned long>(fread(bytes, 1, sizeof(bytes), file)) : 0;
    if (file) {
        fclose(file);
    }
    char id[48];
    sprintf(id, "file.o%d", g_stream_order);
    printBlob(id, bytes, size);
    srBinIFStream* input = new srBinIFStream(path);
    printf("ifs open %d good %d size %lu\n", input->isOpen(), input->good() ? 1 : 0,
           input->getSize());
    input->setByteOrder(static_cast<srBinStream::e_byteOrder>(g_stream_order));
    printf("ifs char %04x word %04x dword %08lx\n", input->getChar(), input->getWord(),
           input->getDWord());
    input->seek(4, srBinStream::SR_SEEK_END);
    printf("ifs tell %lu dword %08lx\n", input->tell(), input->getDWord());
    printf("ifs eof %04x good %d\n", input->getChar(), input->good() ? 1 : 0);
    input->close();
    srBinIFStream* missing = new srBinIFStream("Z:\\no\\such\\sr_difftest_file.bin");
    printf("missing open %d good %d\n", missing->isOpen(), missing->good() ? 1 : 0);
    DeleteFileA(path);
}

static const char* g_xread_id;

static void xreadCase()
{
    const Blob* blob = findPeerBlob(g_xread_id);
    int order = g_xread_id[strlen(g_xread_id) - 1] - '0';
    if (blob == 0) {
        printf("missing blob %s\n", g_xread_id);
        return;
    }
    printf("peer size %lu\n", blob->size);
    if (strncmp(g_xread_id, "random", 6) == 0) {
        decodeOperators(blob->data, blob->size, order);
    } else {
        decodeScalars(blob->data, blob->size, order);
        decodeOperators(blob->data, blob->size, order);
    }
}

void streamCases()
{
    static char names[16][64];
    static const char* const xread_ids[] = {"scalars.o0", "scalars.o1", "edited.o0", "edited.o1",
                                            "random.o0",  "random.o1",  "file.o0",   "file.o1"};
    int order;
    int index;
    if (g_blob_file != 0) {
        for (index = 0; index < 8; ++index) {
            g_xread_id = xread_ids[index];
            sprintf(names[index], "stream.xread.%s", xread_ids[index]);
            runProbe(names[index], xreadCase);
        }
        return;
    }
    for (order = 0; order < 2; ++order) {
        char name[64];
        g_stream_order = order;
        sprintf(name, "stream.write.scalars.o%d", order);
        runProbe(name, writeScalarsCase);
        sprintf(name, "stream.write.random.o%d", order);
        runProbe(name, writeOperatorsBlobCase);
        sprintf(name, "stream.read.scalars.o%d", order);
        runProbe(name, readScalarsCase);
        sprintf(name, "stream.read.operators.o%d", order);
        runProbe(name, operatorsCase);
        sprintf(name, "stream.file.o%d", order);
        runProbe(name, fileStreamCase);
    }
    runProbe("stream.edge.empty-large", emptyStreamCase);
}
