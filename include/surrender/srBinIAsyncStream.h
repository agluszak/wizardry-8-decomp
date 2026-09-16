#pragma once

#include "srBinIStream.h"
#include "srScheduler.h"

/* SR-owned asynchronous reader. The provider vtable is recovered, but no known
   Wizardry/JPEG/ZIP consumer imports srBinIAsyncStream symbols. */
class srBinIAsyncStream : public srBinIStream {
public:
    srBinIAsyncStream(const char* path);
    srBinIAsyncStream(const srBinIAsyncStream& stream);
    virtual ~srBinIAsyncStream() override;

    int isFinished();

    virtual srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(unsigned long position) override;
    virtual unsigned long tell() override;
    virtual unsigned long vread(
        void* destination, unsigned long size) override;

private:
    unsigned char* buffer_08;
    srScheduler::Job* job_0c;
    srBinIStream* stream_10;
    unsigned long position_14;
    unsigned long size_18;
    int finished_1c;
};

static_assert(sizeof(srBinIAsyncStream) == 0x34,
              "srBinIAsyncStream_must_be_0x34");
