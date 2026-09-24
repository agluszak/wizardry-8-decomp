#pragma once

#include "srArray.h"
#include "srBinStream.h"
#include "srQuadWord.h"

// Like srBinIStream, this is an abstract directional interface. Its own
// virtual slot provides the primary vptr at +0, and the original JPEG exporter
// consequently reaches the virtual srBinStream base through the vbptr at +4.
// novtable prevents this SDK boundary from emitting an abstract local table.
// The provider exports the full member surface and both vftables, so the class
// is dllexport under SURRENDER_BUILD.
// VTABLE: SURRENDER 0x10076AE8 srBinStream
// VTABLE: SURRENDER 0x10076AFC srBinOStream
// class srBinOStream
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#else
    __declspec(novtable)
#endif
    srBinOStream : public virtual srBinStream {
public:
    SR_DLL_IMPORT srBinOStream();
    SR_DLL_IMPORT srBinOStream(const srBinOStream& stream);
    virtual SR_DLL_IMPORT ~srBinOStream() override;
    SR_DLL_IMPORT srBinOStream& operator=(const srBinOStream& stream);

    SR_DLL_IMPORT srBinOStream& putChar(char value);
    SR_DLL_IMPORT srBinOStream& putDWord(unsigned long value);
    SR_DLL_IMPORT srBinOStream& putDouble(double value);
    SR_DLL_IMPORT srBinOStream& putFloat(float value);
    SR_DLL_IMPORT srBinOStream& putQWord(srQuadWord value);
    SR_DLL_IMPORT srBinOStream& putWord(unsigned short value);
    SR_DLL_IMPORT srBinOStream& write(const void* source, unsigned long size);

    // SYNTHETIC: SURRENDER 0x10032110
    // srBinOStream::`vbase destructor'

protected:
    // Exported by SR.DLL as ?vput@srBinOStream@@MAEGD@Z. Besides completing
    // the ABI, this gives the directional interface its own primary vptr;
    // its virtual-base pointer consequently lives at +4 as observed in the
    // original JPEG exporter.
    virtual SR_DLL_IMPORT unsigned short vput(char value);

private:
    virtual unsigned long vwrite(const void* source, unsigned long size) = 0;
};

// The memory-backed output stream keeps its buffer in an srArray<unsigned
// char>; getPtr/vwrite show the canonical capacity + 8 + index growth, and
// the copy constructor performs the deep array copy. Wiz8.exe imports the
// constructor, the destructor family, getPtr, and getSize.
// VTABLE: SURRENDER 0x10076BB0 srBinStream
// VTABLE: SURRENDER 0x10076BC4 srBinOStream
// class srBinOMStream
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#else
    __declspec(novtable)
#endif
    srBinOMStream : public srBinOStream {
public:
    SR_DLL_IMPORT srBinOMStream();
    SR_DLL_IMPORT srBinOMStream(const srBinOMStream& stream);
    virtual SR_DLL_IMPORT ~srBinOMStream() override;

    /* Implicit assignment: retail emits it via the class-level dllexport as a
       memberwise copy (the srArray assignment owns the buffer reallocation). */
    // SYNTHETIC: SURRENDER 0x10031250
    // srBinOMStream::operator=

    SR_DLL_IMPORT void* getPtr();
    virtual SR_DLL_IMPORT unsigned long getSize() override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position,
                                            srBinStream::e_seekDir direction) override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position) override;
    virtual SR_DLL_IMPORT unsigned long tell() override;

private:
    virtual SR_DLL_IMPORT unsigned long vwrite(const void* source, unsigned long size) override;

    // SYNTHETIC: SURRENDER 0x10031380
    // srBinOMStream::`vbase destructor'

    srArray<unsigned char> buffer_08;
    unsigned long position_10;
    unsigned long size_14;
};

static_assert(sizeof(srBinOStream) == 0x18, "srBinOStream_must_be_0x18");
static_assert(sizeof(srBinOMStream) == 0x2c, "srBinOMStream_must_be_0x2c");
