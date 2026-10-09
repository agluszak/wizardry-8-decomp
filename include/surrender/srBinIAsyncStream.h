#pragma once

#include "srBinIStream.h"
#include "srScheduler.h"

// VTABLE: SURRENDER 0x100769E0 srBinStream
// VTABLE: SURRENDER 0x100769F4 srBinIStream
// class srBinIAsyncStream
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srBinIAsyncStream : public srBinIStream {
public:
    srBinIAsyncStream(const char* path);
    virtual ~srBinIAsyncStream() override;

    int isFinished();

    virtual srBinStream& seek(w8_ulong position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(w8_ulong position) override;
    virtual w8_ulong tell() override;
    virtual w8_ulong vread(void* destination, w8_ulong size) override;

private:
    unsigned char* buffer;
    srScheduler::Job* job;
    srBinIStream* stream;
    w8_ulong position;
    w8_ulong size;
    int finished;
};

W8_ABI_ASSERT(sizeof(srBinIAsyncStream) == 0x34, "srBinIAsyncStream_must_be_0x34");
