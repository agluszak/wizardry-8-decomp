#include "surrender/srBinIStream.h"
#include "surrender/srCore.h"
#include "surrender/srExtension.h"
#include "surrender/srIStreamOpener.h"
#include "surrender/srString.h"
#include "wiz8/virtual_file.h"
#include "wiz8/virtual_file_stream.h"
#include "FileMan.h"

/* Original translation-unit ownership is unknown; surrounding anchors do not resolve it. */

// FUNCTION: WIZ8 0x0047CBD0
W8VirtualFileBinIStream::W8VirtualFileBinIStream(const char* path) : m_hFile(0)
{
    srInlineString normalized(path);
    {
        srInlineString backslash("\\");
        srInlineString slash("/");

        w8_long index;
        while ((index = normalized.find(slash, 0)) != -1) {
            normalized.erase(index, index + slash.size() - 1);
            normalized.insert(backslash, index);
        }
    }

    m_hFile = FileOpen(normalized.data(), 0x41, 0);
    if (m_hFile != 0) {
        setState(SR_STREAM_OK);
    } else {
        setState(SR_STREAM_ERROR);
    }
}

srInlineString::srInlineString(const char* source)
{
    reset();
    if (source != 0) {
        operator=(source);
    }
}

srInlineString::srInlineString(const srInlineString& source)
{
    reset();
    if (source.data_ != 0) {
        operator=(source);
    }
}

srInlineString::srInlineString(const srInlineString& source, w8_long begin, w8_long end)
{
    reset();
    char* temporary = static_cast<char*>(srHeap.allocate(end - begin + 2));
    strncpy(temporary, source.data_ + begin, end - begin);
    temporary[end - begin] = '\0';
    operator=(temporary);
    srHeap.free(temporary);
}

// FUNCTION: WIZ8 0x0047CDD0
srInlineString::~srInlineString()
{
    release();
}

void srInlineString::release()
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    reset();
}

srInlineString& srInlineString::operator=(const srInlineString& source)
{
    release();
    if (source.data_ != 0 && *source.data_ != '\0') {
        size_ = strlen(source.data_) + 1;
        data_ = static_cast<char*>(srHeap.allocate(size_));
        strcpy(data_, source.data_);
    }
    return *this;
}

// FUNCTION: WIZ8 0x0047CE00
srInlineString& srInlineString::operator=(const char* source)
{
    release();
    if (source == 0 || *source == '\0') {
        return *this;
    }

    size_ = strlen(source) + 1;
    data_ = static_cast<char*>(srHeap.allocate(size_));
    strcpy(data_, source);
    return *this;
}

// FUNCTION: WIZ8 0x0047CE90
w8_long srInlineString::find(const srInlineString& needle, w8_ulong offset) const
{
    const char* found = strstr(data_ + offset, needle.data_);
    if (found != 0) {
        return static_cast<w8_long>(found - data_);
    }
    return -1;
}

// FUNCTION: WIZ8 0x0047CEC0
void srInlineString::erase(w8_ulong begin, w8_ulong end)
{
    if (begin != end) {
        strncpy(data_ + begin, data_ + end, size_ - end);
        size_ = strlen(data_) + 1;
    }
}

// FUNCTION: WIZ8 0x0047CF00
void srInlineString::insert(const srInlineString& text, w8_ulong position)
{
    srInlineString result;
    if (position == 0) {
        result = (text + *this).data();
    } else if (position == size_ - 1) {
        result = (*this + text).data();
    } else {
        result = srInlineString(*this, 0, static_cast<w8_long>(position)).data();
        result += text.data();
        result +=
            srInlineString(*this, static_cast<w8_long>(position), static_cast<w8_long>(size_ - 1))
                .data();
    }
    *this = result;
}

// FUNCTION: WIZ8 0x0047D1C0
srInlineString& srInlineString::operator+=(const char* suffix)
{
    if (suffix == 0 || *suffix == '\0') {
        return *this;
    }
    w8_ulong combined_size = size_ + strlen(suffix);
    char* combined = static_cast<char*>(srHeap.allocate(combined_size));
    strcpy(combined, data_);
    strcpy(combined + size_ - 1, suffix);
    release();
    size_ = combined_size;
    data_ = combined;
    return *this;
}

srInlineString::srInlineString()
{
    reset();
}

void srInlineString::reset()
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

// FUNCTION: WIZ8 0x0047D2A0
srInlineString operator+(const srInlineString& left, const srInlineString& right)
{
    srInlineString result(left);
    result += right.data();
    return result;
}

// FUNCTION: WIZ8 0x0047D490
W8VirtualFileBinIStream::~W8VirtualFileBinIStream()
{
    if (m_hFile) {
        FileClose(m_hFile);
    }
}

// FUNCTION: WIZ8 0x0047D4D0
srBinStream& W8VirtualFileBinIStream::seek(w8_ulong position, e_seekDir direction)
{
    int origin;
    switch (direction) {
    case SR_SEEK_BEGIN:
        origin = 1;
        break;
    case SR_SEEK_CURRENT:
        origin = 4;
        break;
    case SR_SEEK_END:
        origin = 2;
        break;
    default:
        return *this;
    }
    if (!FileSeek(m_hFile, position, origin)) {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

// FUNCTION: WIZ8 0x0047D560
srBinStream& W8VirtualFileBinIStream::seek(w8_ulong position)
{
    if (!FileSeek(m_hFile, position, 1)) {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

// FUNCTION: WIZ8 0x0047D5B0
w8_ulong W8VirtualFileBinIStream::tell()
{
    return FileGetPos(m_hFile);
}

// FUNCTION: WIZ8 0x0047d5c0
w8_ulong W8VirtualFileBinIStream::vread(void* buffer, w8_ulong size)
{
    unsigned int bytes_read;

    if (FileRead(m_hFile, buffer, size, &bytes_read)) {
        return bytes_read;
    }
    return 0;
}

// FUNCTION: WIZ8 0x0047CB30
srBinIStream* W8VirtualFileStreamOpener::open(const char* path)
{
    return new W8VirtualFileBinIStream(path);
}

// FUNCTION: WIZ8 0x0047CBA0
const char* W8VirtualFileStreamOpener::getDescription() const
{
    return "stBinIStream";
}

// GLOBAL: WIZ8 0x0065A124
W8VirtualFileStreamOpener g_virtual_file_stream_opener;

/* MSVC PDB spelling uses overload ordinals, not parameter types. Retail
   secondary-vtable order is seek(2)=ulong, seek(1)=ulong+dir, tell. */

// FUNCTION: WIZ8 0x0047d5f0
void InitializeVirtualFileImageImporters(void)
{
    srExtension::load("JPEGImporter", NULL);
    srExtension::load("TargaImporter", NULL);
    srCore.getIStreamOpener()->addStreamType(&g_virtual_file_stream_opener, "jpg");
    srCore.getIStreamOpener()->addStreamType(&g_virtual_file_stream_opener, "tga");
}
