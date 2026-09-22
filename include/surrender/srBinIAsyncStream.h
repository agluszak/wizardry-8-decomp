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

    /* Class-level dllexport also emits a memberwise copy constructor and
       vbase destructor. Retail exports no assignment operator. */
    // SYNTHETIC: SURRENDER 0x1002EDC0
    // srBinIAsyncStream::srBinIAsyncStream
    // SYNTHETIC: SURRENDER 0x1002EE70
    // srBinIAsyncStream::`vbase destructor'

private:
    unsigned char* buffer_08;
    srScheduler::Job* job_0c;
    srBinIStream* stream_10;
    unsigned long position_14;
    unsigned long size_18;
    int finished_1c;
};

static_assert(sizeof(srBinIAsyncStream) == 0x34, "srBinIAsyncStream_must_be_0x34");
