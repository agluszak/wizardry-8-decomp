#pragma once

#include "surrender/srBinIStream.h"
#include "surrender/srIStreamOpener.h"

/* Wizardry's SurRender stream adapter. The virtual srBinStream base starts at
   +0x10; the remaining storage is the FileMan handle and one unknown word. */
#pragma vtordisp(off)
class W8VirtualFileBinIStream : public srBinIStream {
public:
    explicit W8VirtualFileBinIStream(char* path);
    virtual ~W8VirtualFileBinIStream() override;

    unsigned long vread(void* buffer, unsigned long size) override;
    srBinStream& seek(
        unsigned long position, e_seekDir direction) override;
    srBinStream& seek(unsigned long position) override;
    unsigned long tell() override;

private:
    int m_hFile;                         /* 0x08 */
    unsigned char unknown_0c[4];         /* 0x0c */
};
#pragma vtordisp(on)

static_assert(sizeof(W8VirtualFileBinIStream) == 0x20,
              "W8VirtualFileBinIStream_size_must_be_0x20");

class W8VirtualFileStreamOpener : public srIStreamOpener::Opener {
public:
    srBinIStream* open(char* path) override;
    const char* getDescription() const override;
};

extern W8VirtualFileStreamOpener g_virtual_file_stream_opener_65a124;
