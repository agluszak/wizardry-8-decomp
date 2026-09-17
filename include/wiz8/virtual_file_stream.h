#pragma once

#include "surrender/srBinIStream.h"
#include "surrender/srIStreamOpener.h"

/* Wizardry's SurRender stream adapter. The vbptr at +4 is installed at
   0x0047cc02 with table 0x005ec6a8: {-4, 12}. Its self displacement returns
   to offset zero, and its virtual-base displacement reaches +0x10.
   The virtual srBinStream base starts at
   +0x10; the remaining storage is the FileMan handle and one unknown word.
   The opener's getDescription returns the identifier-style string
   "stBinIStream" - a candidate original spelling for this class, recorded but
   not promoted: getDescription is a description API (the ZIP opener returns
   prose), so it does not prove the C++ class name on its own. */
// VTABLE: WIZ8 0x005ec6a0 W8VirtualFileBinIStream
// VTABLE: WIZ8 0x005ec68c srBinStream
// class W8VirtualFileBinIStream
#pragma vtordisp(off)
class W8VirtualFileBinIStream : public srBinIStream {
public:
    explicit W8VirtualFileBinIStream(const char* path);
    virtual ~W8VirtualFileBinIStream() override;

    unsigned long vread(void* buffer, unsigned long size) override;
    srBinStream& seek(unsigned long position, e_seekDir direction) override;
    srBinStream& seek(unsigned long position) override;
    unsigned long tell() override;

private:
    int m_hFile;                 /* 0x08 */
    unsigned char unknown_0c[4]; /* 0x0c */
};
#pragma vtordisp(on)

static_assert(sizeof(W8VirtualFileBinIStream) == 0x20, "W8VirtualFileBinIStream_size_must_be_0x20");

class W8VirtualFileStreamOpener : public srIStreamOpener::Opener {
public:
    srBinIStream* open(const char* path) override;
    const char* getDescription() const override;
};

extern W8VirtualFileStreamOpener g_virtual_file_stream_opener_65a124;

void InitializeVirtualFileImageImporters(void);
