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

    virtual srBinStream& seek(unsigned long position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(unsigned long position) override;
    virtual unsigned long tell() override;
    virtual unsigned long vread(void* destination, unsigned long size) override;

private:
    unsigned char* buffer;
    srScheduler::Job* job;
    srBinIStream* stream;
    unsigned long position;
    unsigned long size;
    int finished;
};

static_assert(sizeof(srBinIAsyncStream) == 0x34, "srBinIAsyncStream_must_be_0x34");
