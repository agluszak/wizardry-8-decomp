#pragma once

#include "srBinIStream.h"
#include "srScheduler.h"

/* SR-owned asynchronous reader. The provider vtable is recovered, but no known
   Wizardry/JPEG/ZIP consumer imports srBinIAsyncStream symbols. Retail exports
   the full member surface, so the provider build dllexport-s the class. */
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
