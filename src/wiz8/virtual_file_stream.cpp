#include "surrender/srBinIStream.h"
#include "surrender/srCore.h"
#include "surrender/srExtension.h"
#include "surrender/srIStreamOpener.h"
#include "surrender/srString.h"
#include "wiz8/virtual_file.h"
#include "wiz8/virtual_file_stream.h"
#include "FileMan.h"

/* Unresolved fragment: all nine functions lie in the single anchored gap
   between Quality.cpp (0x0047B500) and stModelInstance.cpp (0x00480920).
   One contiguous interval, but no anchor proves a single original TU. */

/* The SurRender-facing stream adapter that carries the SLF virtual file system
   into the SR stream hierarchy. It is declared as what it is rather than as an
   opaque prefix: the two vtables the constructor at 0x0047CBD0 installs are the
   two this declaration produces; their reviewed symbols and slots live in
   Ghidra rather than a parallel generated inventory.

   Primary 0x005EC6A0, at offset 0, has vget imported from SR.DLL in slot 0 and
   Read in slot 1 - the slot srBinIStream leaves pure. Secondary 0x005EC68C, the
   virtual srBinStream base at +0x10, inherits getSize from SR.DLL in slot 1 and
   overrides the destructor and the three seek/tell slots locally.

   The 0x20-byte size the sole caller of the constructor allocates is what the
   assertion below checks, and it holds only if the srBinIStream base really is
   vptr, vbptr and a virtual srBinStream subobject placed last.

   The path normalization is the one Wiz8.exe reach of SurRender's srInlineString:
   the retail body constructs three objects here and calls this TU's out-of-line
   find/erase/insert to rewrite each '/' as '\\'. In this product only the
   constructors and destructor expand inline; the remaining members are defined
   out-of-line below. */
// FUNCTION: WIZ8 0x0047CBD0
W8VirtualFileBinIStream::W8VirtualFileBinIStream(const char* path) : m_hFile(0)
{
    srInlineString normalized(path);
    {
        srInlineString backslash("\\");
        srInlineString slash("/");

        long index;
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
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
    if (source.data_ != 0) {
        operator=(source);
    }
}

inline srInlineString::srInlineString(const srInlineString& source, long begin, long end)
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
    char* temporary = static_cast<char*>(srHeap.allocate(end - begin + 2));
    strncpy(temporary, source.data_ + begin, end - begin);
    temporary[end - begin] = '\0';
    operator=(temporary);
    srHeap.free(temporary);
}

/* Retail expands this destructor at shallow sites and calls the emission from
   deeper ones (insert's `*this =` tail). Our build splits the same way. */
// FUNCTION: WIZ8 0x0047CDD0
inline srInlineString::~srInlineString()
{
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    reset();
}

/* Releases this object's contents, then copies the source text. Where
   srEXT_Unzip delegates to the (const char*) overload, this product's copy
   destroys first and copies inline - insert's tail calls the destructor and
   performs the copy without a second call. */
inline srInlineString& srInlineString::operator=(const srInlineString& source)
{
    this->~srInlineString();
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

// FUNCTION: WIZ8 0x0047CE90
long srInlineString::find(const srInlineString& needle, unsigned long offset) const
{
    const char* found = strstr(data_ + offset, needle.data_);
    if (found != 0) {
        return static_cast<long>(found - data_);
    }
    return -1;
}

// FUNCTION: WIZ8 0x0047CEC0
void srInlineString::erase(unsigned long begin, unsigned long end)
{
    if (begin != end) {
        strncpy(data_ + begin, data_ + end, size_ - end);
        size_ = strlen(data_) + 1;
    }
}

/* Splices text into the content at position: prepend when 0, append at the
   terminator, otherwise rebuild from the [0, position) and [position, size)
   pieces through the substring constructor. */
// FUNCTION: WIZ8 0x0047CF00
void srInlineString::insert(const srInlineString& text, unsigned long position)
{
    srInlineString result;
    if (position == 0) {
        result = (text + *this).data();
    } else if (position == size_ - 1) {
        result = (*this + text).data();
    } else {
        result = srInlineString(*this, 0, static_cast<long>(position)).data();
        result += text.data();
        result +=
            srInlineString(*this, static_cast<long>(position), static_cast<long>(size_ - 1)).data();
    }
    *this = result;
}

// FUNCTION: WIZ8 0x0047D1C0
srInlineString& srInlineString::operator+=(const char* suffix)
{
    if (suffix == 0 || *suffix == '\0') {
        return *this;
    }
    unsigned long combined_size = size_ + strlen(suffix);
    char* combined = static_cast<char*>(srHeap.allocate(combined_size));
    strcpy(combined, data_);
    strcpy(combined + size_ - 1, suffix);
    if (data_ != inline_) {
        srHeap.free(data_);
    }
    inline_[0] = '\0';
    size_ = combined_size;
    data_ = combined;
    return *this;
}

inline srInlineString::srInlineString()
{
    reset();
}

/* Retail has a callable emission here - deep expansion sites (insert's
   destroyed temporaries, the operator+ copy-out) keep calls while shallow
   sites expand the three stores. Our build inlines it at every site, so the
   emission does not materialize; the divergence is VC6's per-site inline
   budget, not the declaration. Address 0x0047D290 is the retail callable
   form and is not claimed here. */
inline void srInlineString::reset()
{
    inline_[0] = '\0';
    data_ = inline_;
    size_ = 1;
}

// FUNCTION: WIZ8 0x0047D2A0
srInlineString operator+(const srInlineString& left, const srInlineString& right)
{
    srInlineString result(left);
    if (right.data() == 0 || *right.data() == '\0') {
        return result;
    }

    const unsigned long combined_size = result.size() + strlen(right.data());
    char* combined = static_cast<char*>(srHeap.allocate(combined_size));
    strcpy(combined, result.data());
    strcpy(combined + result.size() - 1, right.data());
    if (result.data_ != result.inline_) {
        srHeap.free(result.data_);
    }
    result.inline_[0] = '\0';
    result.size_ = combined_size;
    result.data_ = combined;
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
srBinStream& W8VirtualFileBinIStream::seek(unsigned long position, e_seekDir direction)
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
srBinStream& W8VirtualFileBinIStream::seek(unsigned long position)
{
    if (!FileSeek(m_hFile, position, 1)) {
        setState(SR_STREAM_ERROR);
    }
    return *this;
}

// FUNCTION: WIZ8 0x0047D5B0
unsigned long W8VirtualFileBinIStream::tell()
{
    return FileGetPos(m_hFile);
}

// Note the original reuses the `size` parameter slot as the completed-count
// out-parameter, and returns it branchlessly.
// FUNCTION: WIZ8 0x0047d5c0
unsigned long W8VirtualFileBinIStream::vread(void* buffer, unsigned long size)
{
    /* SurRender spells its 32-bit count unsigned long; SGP spells the same
       ABI word UINT32 (unsigned int). The canonical body reuses this parameter
       slot, so keep that ownership explicit at the header boundary.
       reinterpret-ok: unsigned long and unsigned int are the same ABI word. */
    if (FileRead(m_hFile, buffer, size, reinterpret_cast<unsigned int*>(&size))) {
        return size;
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

W8VirtualFileStreamOpener g_virtual_file_stream_opener_65a124;

// SYNTHETIC: WIZ8 0x0047CBB0
// W8VirtualFileStreamOpener::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0047DA10
// W8VirtualFileBinIStream::`scalar deleting destructor'`vtordisp{-4, 0}'

// SYNTHETIC: WIZ8 0x0047DA20
// W8VirtualFileBinIStream::`scalar deleting destructor'

/* MSVC PDB spelling uses overload ordinals, not parameter types. Retail
   secondary-vtable order is seek(2)=ulong, seek(1)=ulong+dir, tell. */
// SYNTHETIC: WIZ8 0x0047DA50
// W8VirtualFileBinIStream::seek(2)`vtordisp{-4, 0}'

// SYNTHETIC: WIZ8 0x0047DA60
// W8VirtualFileBinIStream::seek(1)`vtordisp{-4, 0}'

// SYNTHETIC: WIZ8 0x0047DA70
// W8VirtualFileBinIStream::tell`vtordisp{-4, 0}'

/* Loads the image importers and routes their JPG/TGA reads through Wizardry's
   SLF-aware virtual file stream, which is the bridge the real menu assets use. */
// FUNCTION: WIZ8 0x0047d5f0
void InitializeVirtualFileImageImporters(void)
{
    srExtension::load("JPEGImporter", NULL);
    srExtension::load("TargaImporter", NULL);
    srCore.getIStreamOpener()->addStreamType(&g_virtual_file_stream_opener_65a124, "jpg");
    srCore.getIStreamOpener()->addStreamType(&g_virtual_file_stream_opener_65a124, "tga");
}
