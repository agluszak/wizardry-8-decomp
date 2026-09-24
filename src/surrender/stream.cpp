#include "surrender/srBinFStream.h"
#include "surrender/srBinOStream.h"
#include "surrender/srHeap.h"
#include "surrender/srIStreamOpener.h"

#include <string.h>

/* TU-local srInlineString methods. Retail expands the constructor, copy
   assignment, destructor, find and erase inline in this unit while init,
   reset and operator+ stay callable emissions owned by sibling units (init
   at 0x10004150, reset at 0x10012C80, operator+ at 0x10012CB0).
   replace/insert/operator+= are this unit's own emissions. */

inline srInlineString::srInlineString()
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

inline srInlineString::srInlineString(const char* source)
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
    if (source != 0) {
        operator=(source);
    }
}

inline srInlineString::srInlineString(const srInlineString& source)
{
    init();
    if (source.data_ != 0) {
        operator=(source);
    }
}

inline srInlineString::~srInlineString()
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    init();
}

inline srInlineString& srInlineString::operator=(const srInlineString& source)
{
    init();
    if (source.data_ != 0 && *source.data_ != '\0') {
        size_ = strlen(source.data_) + 1;
        data_ = static_cast<char*>(srHeap.allocate(size_));
        strcpy(data_, source.data_);
    }
    return *this;
}

inline srInlineString& srInlineString::operator=(const char* source)
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
    if (source == 0 || *source == '\0') {
        return *this;
    }
    size_ = strlen(source) + 1;
    data_ = static_cast<char*>(srHeap.allocate(size_));
    strcpy(data_, source);
    return *this;
}

inline long srInlineString::find(const srInlineString& needle, unsigned long offset) const
{
    const char* found = strstr(data_ + offset, needle.data_);
    if (found != 0) {
        return static_cast<long>(found - data_);
    }
    return -1;
}

inline void srInlineString::erase(unsigned long begin, unsigned long end)
{
    if (begin != end) {
        strncpy(data_ + begin, data_ + end, size_ - end);
        size_ = strlen(data_) + 1;
    }
}

// FUNCTION: SURRENDER 0x100309C0
srBinIMStream::srBinIMStream(const void* data, unsigned long size)
{
    if (data == 0 || size == 0) {
        data_08 = 0;
        size_0c = 0;
        setState(SR_STREAM_ERROR);
    } else {
        data_08 = static_cast<const unsigned char*>(data);
        size_0c = size;
        setState(SR_STREAM_OK);
    }
    position_10 = 0;
}

// FUNCTION: SURRENDER 0x10030AA0
srBinStream& srBinIMStream::seek(unsigned long position, e_seekDir direction)
{
    unsigned long new_position = 0;
    switch (direction) {
    case SR_SEEK_BEGIN:
        new_position = position;
        break;
    case SR_SEEK_CURRENT:
        new_position = position_10 + position;
        break;
    case SR_SEEK_END:
        new_position = size_0c - position;
        break;
    }
    seek(new_position);
    return *this;
}

// FUNCTION: SURRENDER 0x10030B00
srBinStream& srBinIMStream::seek(unsigned long position)
{
    if (position <= size_0c) {
        position_10 = position;
    } else {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10030B40
unsigned long srBinIMStream::tell()
{
    return position_10;
}

// FUNCTION: SURRENDER 0x10030B50
unsigned long srBinIMStream::vread(void* destination, unsigned long size)
{
    if (position_10 + size >= size_0c) {
        size = size_0c - position_10;
    }
    if (size != 0) {
        if (static_cast<int>(size) > 0) {
            memcpy(destination, data_08 + position_10, size);
        }
        position_10 += size;
    }
    return size;
}

// FUNCTION: SURRENDER 0x10030BA0
unsigned long srBinIMStream::getSize()
{
    return size_0c;
}

// FUNCTION: SURRENDER 0x10030E00
srBinOMStream::srBinOMStream()
{
    size_14 = 0;
    position_10 = 0;
    setState(SR_STREAM_OK);
}

// FUNCTION: SURRENDER 0x10030EC0
void* srBinOMStream::getPtr()
{
    return &buffer_08[0];
}

// FUNCTION: SURRENDER 0x10030F30
unsigned long srBinOMStream::getSize()
{
    return size_14;
}

// FUNCTION: SURRENDER 0x10030F40
srBinStream& srBinOMStream::seek(unsigned long position, e_seekDir direction)
{
    unsigned long new_position = 0;
    switch (direction) {
    case SR_SEEK_BEGIN:
        new_position = position;
        break;
    case SR_SEEK_CURRENT:
        new_position = position_10 + position;
        break;
    case SR_SEEK_END:
        new_position = size_14 - position;
        break;
    }
    seek(new_position);
    return *this;
}

// FUNCTION: SURRENDER 0x10030FA0
srBinStream& srBinOMStream::seek(unsigned long position)
{
    position_10 = position;
    return *this;
}

// FUNCTION: SURRENDER 0x10030FC0
unsigned long srBinOMStream::tell()
{
    return position_10;
}

// FUNCTION: SURRENDER 0x10030FD0
unsigned long srBinOMStream::vwrite(const void* source, unsigned long size)
{
    if (size != 0) {
        buffer_08[position_10 + size];
        unsigned char* destination = &buffer_08[position_10];
        if (static_cast<int>(size) > 0) {
            memcpy(destination, source, size);
        }
        position_10 += size;
        if (size_14 < position_10) {
            size_14 = position_10;
        }
        return size;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x10031120
srBinOMStream::srBinOMStream(const srBinOMStream& other) : srBinOStream(other)
{
    buffer_08 = other.buffer_08;
    position_10 = other.position_10;
    size_14 = other.size_14;
}

// FUNCTION: SURRENDER 0x10031330
srBinOMStream::~srBinOMStream() {}

// FUNCTION: SURRENDER 0x10031C80
srBinIStream::srBinIStream(const srBinIStream& stream) : srBinStream(stream) {}

// FUNCTION: SURRENDER 0x10031CE0
srBinIStream& srBinIStream::operator=(const srBinIStream& stream)
{
    srBinStream::operator=(stream);
    return *this;
}

// FUNCTION: SURRENDER 0x10032020
srBinOStream::srBinOStream() {}

// FUNCTION: SURRENDER 0x10032060
srBinOStream::srBinOStream(const srBinOStream& stream) : srBinStream(stream) {}

// FUNCTION: SURRENDER 0x100320C0
srBinOStream& srBinOStream::operator=(const srBinOStream& stream)
{
    srBinStream::operator=(stream);
    return *this;
}

// FUNCTION: SURRENDER 0x10031490
srBinIStream& srBinIStream::read(void* destination, unsigned long size)
{
    if (good()) {
        if (vread(destination, size) != size) {
            setState(SR_STREAM_ERROR);
        }
    }
    return *this;
}

// FUNCTION: SURRENDER 0x100314E0
unsigned short srBinIStream::vget()
{
    unsigned char character = 0;
    if (vread(&character, 1) != 1) {
        return 0xffff;
    }
    return character;
}

// FUNCTION: SURRENDER 0x10031510
unsigned short srBinIStream::getChar()
{
    if (good()) {
        unsigned short character = vget();
        if (character == 0xffff) {
            setState(SR_STREAM_ERROR);
        }
        return character;
    }
    return 0xffff;
}

// FUNCTION: SURRENDER 0x10031560
unsigned short srBinIStream::getWord()
{
    unsigned short value = 0;
    unsigned short buffer;
    if (good()) {
        read(&buffer, 2);
        if (good()) {
            if (!byteOrderMatch()) {
                // reinterpret-ok: in-place swap of the word's raw bytes.
                unsigned char* bytes = reinterpret_cast<unsigned char*>(&buffer);
                unsigned char byte = bytes[0];
                bytes[0] = bytes[1];
                bytes[1] = byte;
            }
            return buffer;
        }
    }
    return value;
}

// FUNCTION: SURRENDER 0x100315D0
unsigned long srBinIStream::getDWord()
{
    unsigned long value = 0;
    unsigned long buffer;
    if (good()) {
        read(&buffer, 4);
        if (good()) {
            if (!byteOrderMatch()) {
                // reinterpret-ok: byteSwap rewrites the object's raw bytes.
                byteSwap(reinterpret_cast<unsigned char*>(&buffer), 4);
            }
            return buffer;
        }
    }
    return value;
}

// FUNCTION: SURRENDER 0x10031640
float srBinIStream::getFloat()
{
    float value = 0.0f;
    float buffer;
    if (good()) {
        read(&buffer, 4);
        if (good()) {
            if (!byteOrderMatch()) {
                // reinterpret-ok: byteSwap rewrites the object's raw bytes.
                byteSwap(reinterpret_cast<unsigned char*>(&buffer), 4);
            }
            return buffer;
        }
    }
    return value;
}

// FUNCTION: SURRENDER 0x100316C0
double srBinIStream::getDouble()
{
    double value = 0.0;
    double buffer;
    if (good()) {
        read(&buffer, 8);
        if (good()) {
            if (!byteOrderMatch()) {
                // reinterpret-ok: byteSwap rewrites the object's raw bytes.
                byteSwap(reinterpret_cast<unsigned char*>(&buffer), 8);
            }
            return buffer;
        }
    }
    return value;
}

// FUNCTION: SURRENDER 0x10031740
srQuadWord srBinIStream::getQuadWord()
{
    srQuadWord value = {0, 0};
    srQuadWord buffer;
    if (good()) {
        read(&buffer, 8);
        if (good()) {
            if (!byteOrderMatch()) {
                // reinterpret-ok: byteSwap rewrites the object's raw bytes.
                byteSwap(reinterpret_cast<unsigned char*>(&buffer), 8);
            }
            return buffer;
        }
    }
    return value;
}

// FUNCTION: SURRENDER 0x100317C0
srBinIStream& operator>>(srBinIStream& stream, int& value)
{
    value = stream.getDWord();
    return stream;
}

// FUNCTION: SURRENDER 0x100317E0
srBinIStream& operator>>(srBinIStream& stream, char& value)
{
    value = stream.getChar();
    return stream;
}

// FUNCTION: SURRENDER 0x10031800
srBinIStream& operator>>(srBinIStream& stream, unsigned char& value)
{
    value = stream.getChar();
    return stream;
}

// FUNCTION: SURRENDER 0x10031820
srBinIStream& operator>>(srBinIStream& stream, short& value)
{
    value = stream.getWord();
    return stream;
}

// FUNCTION: SURRENDER 0x10031840
srBinIStream& operator>>(srBinIStream& stream, unsigned short& value)
{
    value = stream.getWord();
    return stream;
}

// FUNCTION: SURRENDER 0x10031860
srBinIStream& operator>>(srBinIStream& stream, long& value)
{
    value = stream.getDWord();
    return stream;
}

// FUNCTION: SURRENDER 0x10031880
srBinIStream& operator>>(srBinIStream& stream, unsigned long& value)
{
    value = stream.getDWord();
    return stream;
}

// FUNCTION: SURRENDER 0x100318A0
srBinIStream& operator>>(srBinIStream& stream, srQuadWord& value)
{
    value = stream.getQuadWord();
    return stream;
}

// FUNCTION: SURRENDER 0x100318D0
srBinIStream& operator>>(srBinIStream& stream, float& value)
{
    value = stream.getFloat();
    return stream;
}

// FUNCTION: SURRENDER 0x100318F0
srBinIStream& operator>>(srBinIStream& stream, double& value)
{
    value = stream.getDouble();
    return stream;
}

// FUNCTION: SURRENDER 0x10031910
srBinIStream& operator>>(srBinIStream& stream, srVector2T<float>& value)
{
    value.x = stream.getFloat();
    value.y = stream.getFloat();
    return stream;
}

// FUNCTION: SURRENDER 0x10031940
srBinIStream& operator>>(srBinIStream& stream, srVector2T<double>& value)
{
    value.x = stream.getDouble();
    value.y = stream.getDouble();
    return stream;
}

// FUNCTION: SURRENDER 0x10031970
srBinIStream& operator>>(srBinIStream& stream, srVector3T<float>& value)
{
    value.x = stream.getFloat();
    value.y = stream.getFloat();
    value.z = stream.getFloat();
    return stream;
}

// FUNCTION: SURRENDER 0x100319A0
srBinIStream& operator>>(srBinIStream& stream, srVector3T<double>& value)
{
    value.x = stream.getDouble();
    value.y = stream.getDouble();
    value.z = stream.getDouble();
    return stream;
}

// FUNCTION: SURRENDER 0x100319D0
srBinIStream& operator>>(srBinIStream& stream, srVector4T<float>& value)
{
    value.x = stream.getFloat();
    value.y = stream.getFloat();
    value.z = stream.getFloat();
    value.w = stream.getFloat();
    return stream;
}

// FUNCTION: SURRENDER 0x10031A10
srBinIStream& operator>>(srBinIStream& stream, srVector4T<double>& value)
{
    value.x = stream.getDouble();
    value.y = stream.getDouble();
    value.z = stream.getDouble();
    value.w = stream.getDouble();
    return stream;
}

// FUNCTION: SURRENDER 0x10031A50
srBinIStream& operator>>(srBinIStream& stream, srVector2i& value)
{
    value.x = stream.getDWord();
    value.y = stream.getDWord();
    return stream;
}

// FUNCTION: SURRENDER 0x10031A80
srBinIStream& operator>>(srBinIStream& stream, srVector3i& value)
{
    value.x = stream.getDWord();
    value.y = stream.getDWord();
    value.z = stream.getDWord();
    return stream;
}

// FUNCTION: SURRENDER 0x10031AB0
srBinIStream& operator>>(srBinIStream& stream, srVector4i& value)
{
    value.x = stream.getDWord();
    value.y = stream.getDWord();
    value.z = stream.getDWord();
    value.w = stream.getDWord();
    return stream;
}

// FUNCTION: SURRENDER 0x10031AF0
srBinIStream& operator>>(srBinIStream& stream, srQuaternion& value)
{
    stream >> value.w;
    stream >> value.v;
    return stream;
}

// FUNCTION: SURRENDER 0x10031B20
srBinIStream& operator>>(srBinIStream& stream, srMatrix2T<float>& value)
{
    srVector2T<float>* vector = value.vectors;
    long index = 2;
    do {
        stream >> *vector;
        ++vector;
        --index;
    } while (index != 0);
    return stream;
}

// FUNCTION: SURRENDER 0x10031B50
srBinIStream& operator>>(srBinIStream& stream, srMatrix3T<float>& value)
{
    srVector3T<float>* vector = value.vectors;
    long index = 3;
    do {
        stream >> *vector;
        ++vector;
        --index;
    } while (index != 0);
    return stream;
}

// FUNCTION: SURRENDER 0x10031B80
srBinIStream& operator>>(srBinIStream& stream, srMatrix4T<float>& value)
{
    srVector4T<float>* vector = value.vectors;
    long index = 4;
    do {
        stream >> *vector;
        ++vector;
        --index;
    } while (index != 0);
    return stream;
}

// FUNCTION: SURRENDER 0x10031BB0
srBinIStream& operator>>(srBinIStream& stream, srMatrix2T<double>& value)
{
    srVector2T<double>* vector = value.vectors;
    long index = 2;
    do {
        stream >> *vector;
        ++vector;
        --index;
    } while (index != 0);
    return stream;
}

// FUNCTION: SURRENDER 0x10031BE0
srBinIStream& operator>>(srBinIStream& stream, srMatrix3T<double>& value)
{
    srVector3T<double>* vector = value.vectors;
    long index = 3;
    do {
        stream >> *vector;
        ++vector;
        --index;
    } while (index != 0);
    return stream;
}

// FUNCTION: SURRENDER 0x10031C10
srBinIStream& operator>>(srBinIStream& stream, srMatrix4T<double>& value)
{
    srVector4T<double>* vector = value.vectors;
    long index = 4;
    do {
        stream >> *vector;
        ++vector;
        --index;
    } while (index != 0);
    return stream;
}

// FUNCTION: SURRENDER 0x10031DA0
unsigned short srBinOStream::vput(char character)
{
    return vwrite(&character, 1) != 1 ? 0xffff : 0;
}

// FUNCTION: SURRENDER 0x10031DC0
srBinOStream& srBinOStream::putChar(char character)
{
    if (good()) {
        if (vput(character) == 0xffff) {
            setState(SR_STREAM_ERROR);
        }
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10031E00
srBinOStream& srBinOStream::write(const void* source, unsigned long size)
{
    if (good()) {
        if (vwrite(source, size) != size) {
            setState(SR_STREAM_ERROR);
        }
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10031E50
srBinOStream& srBinOStream::putWord(unsigned short value)
{
    if (good()) {
        if (!byteOrderMatch()) {
            // reinterpret-ok: byteSwap rewrites the object's raw bytes.
            byteSwap(reinterpret_cast<unsigned char*>(&value), 2);
        }
        write(&value, 2);
        return *this;
    }
    setState(SR_STREAM_ERROR);
    return *this;
}

// FUNCTION: SURRENDER 0x10031EC0
srBinOStream& srBinOStream::putDWord(unsigned long value)
{
    if (good()) {
        if (!byteOrderMatch()) {
            // reinterpret-ok: byteSwap rewrites the object's raw bytes.
            byteSwap(reinterpret_cast<unsigned char*>(&value), 4);
        }
        write(&value, 4);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10031F10
srBinOStream& srBinOStream::putQWord(srQuadWord value)
{
    if (good()) {
        if (!byteOrderMatch()) {
            // reinterpret-ok: byteSwap rewrites the object's raw bytes.
            byteSwap(reinterpret_cast<unsigned char*>(&value), 8);
        }
        write(&value, 8);
        return *this;
    }
    setState(SR_STREAM_ERROR);
    return *this;
}

// FUNCTION: SURRENDER 0x10031F80
srBinOStream& srBinOStream::putFloat(float value)
{
    if (good()) {
        if (!byteOrderMatch()) {
            // reinterpret-ok: byteSwap rewrites the object's raw bytes.
            byteSwap(reinterpret_cast<unsigned char*>(&value), 4);
        }
        write(&value, 4);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10031FD0
srBinOStream& srBinOStream::putDouble(double value)
{
    if (good()) {
        if (!byteOrderMatch()) {
            // reinterpret-ok: byteSwap rewrites the object's raw bytes.
            byteSwap(reinterpret_cast<unsigned char*>(&value), 8);
        }
        write(&value, 8);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x10032180
srBinStream::srBinStream()
{
    byte_order_0c = SR_BYTE_ORDER_0;
    state_04 = SR_STREAM_STATE_2;
    exceptions_08 = false;
}

// FUNCTION: SURRENDER 0x100321D0
bool srBinStream::byteOrderMatch() const
{
    return getByteOrder() != SR_BYTE_ORDER_1 ? true : false;
}

// FUNCTION: SURRENDER 0x100321A0
srBinStream::e_byteOrder srBinStream::getByteOrder() const
{
    return byte_order_0c;
}

// FUNCTION: SURRENDER 0x100321B0
void srBinStream::setByteOrder(e_byteOrder order)
{
    byte_order_0c = order;
}

// FUNCTION: SURRENDER 0x100321C0
bool srBinStream::good() const
{
    return state_04 == SR_STREAM_OK;
}

// FUNCTION: SURRENDER 0x100321E0
void srBinStream::byteSwap(unsigned char* data, int size)
{
    for (int index = 0; index < size / 2; ++index) {
        unsigned char byte = data[index];
        data[index] = data[size - 1 - index];
        data[size - 1 - index] = byte;
    }
}

// FUNCTION: SURRENDER 0x10032220
srBinStream::operator void*() const
{
    return (void*)good();
}

// FUNCTION: SURRENDER 0x10032230
bool srBinStream::operator!() const
{
    return !good();
}

// FUNCTION: SURRENDER 0x10032270
void srBinStream::clear()
{
    state_04 = SR_STREAM_OK;
}

// FUNCTION: SURRENDER 0x100322A0
bool srBinStream::exceptions(bool flag)
{
    bool old = exceptions_08;
    exceptions_08 = flag;
    return old;
}

// FUNCTION: SURRENDER 0x100322B0
void srBinStream::setState(e_state state)
{
    state_04 = state;
    if (exceptions_08 && state == SR_STREAM_ERROR) {
        throw Failure(state);
    }
}

// FUNCTION: SURRENDER 0x10032340
unsigned long srBinStream::getSize()
{
    if (good()) {
        unsigned long position = tell();
        seek(0, SR_SEEK_END);
        unsigned long size = tell();
        seek(position, SR_SEEK_BEGIN);
        return size;
    }
    return 0;
}

// FUNCTION: SURRENDER 0x1002E950
srBinStream::~srBinStream() {}

// FUNCTION: SURRENDER 0x100324C0
void srIStreamOpener::parsePrefix(char** prefix, char** path, const char* source)
{
    for (int index = 0; source[index] != '\0'; ++index) {
        if (source[index] == ':' && source[index + 1] == '/' && source[index + 2] == '/') {
            *prefix = new char[index + 2];
            *path = new char[strlen(source) - index];
            strncpy(*prefix, source, index);
            (*prefix)[index] = '\0';
            strcpy(*path, source + index + 3);
            return;
        }
    }
    *prefix = 0;
    *path = new char[strlen(source) + 1];
    strcpy(*path, source);
}

// FUNCTION: SURRENDER 0x100325B0
void srIStreamOpener::addStreamType(Opener* opener, const char* stream_type)
{
    char* type_copy = new char[strlen(stream_type) + 1];
    strcpy(type_copy, stream_type);
    StreamType* node = new StreamType;
    node->opener_00 = opener;
    node->extension_04 = type_copy;
    StreamType* first = first_04;
    StreamType* previous = first->previous_0c;
    node->next_08 = first;
    node->previous_0c = previous;
    if (previous != 0) {
        previous->next_08 = node;
    } else {
        first_04 = node;
    }
    if (node->next_08 != 0) {
        node->next_08->previous_0c = node;
    }
    ++count_00;
}

// FUNCTION: SURRENDER 0x10032630
srIStreamOpener::Opener* srIStreamOpener::findOpener(const char* stream_type)
{
    for (StreamType* node = first_04; node != end_08; node = node->next_08) {
        if (_stricmp(node->extension_04, stream_type) == 0) {
            return node->opener_00;
        }
    }
    return 0;
}

// FUNCTION: SURRENDER 0x100326A0
srIStreamOpener::Opener& srIStreamOpener::Opener::operator=(const Opener& other)
{
    return *this;
}

// FUNCTION: SURRENDER 0x100326E0
srBinIStream* srIStreamOpener::open(const char* path)
{
    char* prefix = 0;
    char* local_path = 0;
    parsePrefix(&prefix, &local_path, path);
    srBinIStream* stream = open(prefix, local_path);
    delete[] local_path;
    delete[] prefix;
    return stream;
}

// FUNCTION: SURRENDER 0x10032740
srIStreamOpener& srIStreamOpener::operator=(const srIStreamOpener& other)
{
    count_00 = other.count_00;
    first_04 = other.first_04;
    end_08 = other.end_08;
    return *this;
}

// FUNCTION: SURRENDER 0x10032780
srBinIStream* srIStreamOpener::open(const char* prefix, const char* path)
{
    srInlineString local_path;
    if (path != 0) {
        local_path = path;
    }
    srInlineString slash("/");
    srInlineString backslash("\\");
    while (local_path.replace(backslash, slash) != 0) {
    }
    if (prefix == 0 || *prefix == '\0') {
        srBinIStream* stream = new srBinIFStream(local_path.data());
        if (!stream->good()) {
            delete stream;
            for (StreamType* node = first_04; node != end_08; node = node->next_08) {
                stream = node->opener_00->open(local_path.data());
                if (stream != 0) {
                    return stream;
                }
            }
            return 0;
        }
        return stream;
    }
    Opener* opener = findOpener(prefix);
    if (opener == 0) {
        return 0;
    }
    srBinIStream* stream = opener->open(local_path.data());
    if (stream != 0 && !stream->good()) {
        delete stream;
        return 0;
    }
    return stream;
}

// FUNCTION: SURRENDER 0x10032A80
srIStreamOpener::~srIStreamOpener()
{
    for (StreamType* node = first_04; node != end_08; node = node->next_08) {
        delete[] node->extension_04;
        node->extension_04 = 0;
    }
    StreamType* entry = first_04;
    while (entry != end_08) {
        first_04 = entry->next_08;
        if (entry->previous_0c != 0) {
            entry->previous_0c->next_08 = entry->next_08;
        }
        if (entry->next_08 != 0) {
            entry->next_08->previous_0c = entry->previous_0c;
        }
        delete entry;
        entry = first_04;
        --count_00;
    }
    delete first_04;
}

// FUNCTION: SURRENDER 0x10032B00
int srInlineString::replace(const srInlineString& needle, const srInlineString& replacement)
{
    long position = find(needle, 0);
    if (position == -1) {
        return 0;
    }
    erase(position, position + needle.size_ - 1);
    insert(replacement, position);
    return 1;
}

// FUNCTION: SURRENDER 0x10032B80
void srInlineString::insert(const srInlineString& text, unsigned long position)
{
    srInlineString result;
    if (position == 0) {
        result = (text + *this).data();
    } else if (position == size_ - 1) {
        result = (*this + text).data();
    } else {
        srInlineString left;
        char* buffer = static_cast<char*>(srHeap.allocate(position + 2));
        strncpy(buffer, data_, position);
        buffer[position] = '\0';
        left = buffer;
        srHeap.free(buffer);
        result = left.data();
        result += text.data();
        long count = size_ - 1 - position;
        srInlineString right;
        buffer = static_cast<char*>(srHeap.allocate(count + 2));
        strncpy(buffer, data_ + position, count);
        buffer[count] = '\0';
        right = buffer;
        srHeap.free(buffer);
        result += right.data();
    }
    *this = result;
}

// FUNCTION: SURRENDER 0x10032E30
srInlineString& srInlineString::operator+=(const char* suffix)
{
    if (suffix != 0 && *suffix != '\0') {
        unsigned long needed = strlen(suffix) + size_;
        char* buffer = static_cast<char*>(srHeap.allocate(needed));
        strcpy(buffer, data_);
        strcpy(buffer + size_ - 1, suffix);
        if (data_ != inline_) {
            srHeap.free(data_);
        }
        size_ = needed;
        inline_[0] = '\0';
        data_ = buffer;
    }
    return *this;
}

/* The retail emission is a bare ret; ours still stores the vftable before
   returning, the same destructor-vptr gap ~srBinIStream records. */
// FUNCTION: SURRENDER 0x10016850
srFStreamOpener::~srFStreamOpener() {}

// FUNCTION: SURRENDER 0x10032450
srFStreamOpener& srFStreamOpener::operator=(const srFStreamOpener& other)
{
    return *this;
}

// FUNCTION: SURRENDER 0x10032380
const char* srFStreamOpener::getDescription() const
{
    return "Standard file stream opener";
}

// FUNCTION: SURRENDER 0x10032390
srBinIStream* srFStreamOpener::open(const char* path)
{
    srBinIStream* stream = new srBinIFStream(path);
    if (stream->good()) {
        return stream;
    }
    delete stream;
    return 0;
}
