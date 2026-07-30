#pragma once

#include "srBinIStream.h"
#include "srScheduler.h"

class srBinIAsyncStream : public srBinIStream {
public:
    SR_DLL_IMPORT srBinIAsyncStream(const char* path);
    SR_DLL_IMPORT srBinIAsyncStream(const srBinIAsyncStream& stream);
    virtual SR_DLL_IMPORT ~srBinIAsyncStream() override;

    SR_DLL_IMPORT int isFinished();

    virtual SR_DLL_IMPORT srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction) override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position) override;
    virtual SR_DLL_IMPORT unsigned long tell() override;
    virtual SR_DLL_IMPORT unsigned long vread(
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
