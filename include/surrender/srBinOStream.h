#pragma once

#include "srArray.h"
#include "srBinStream.h"
#include "srQuadWord.h"

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
    /* The DLL build uses compiler-generated special members. Native clients use
       the same members; the Windows import declarations remain unchanged. */
#if !defined(SURRENDER_BUILD)
    SR_DLL_IMPORT srBinOStream();
    SR_DLL_IMPORT srBinOStream(const srBinOStream& stream);
    virtual SR_DLL_IMPORT ~srBinOStream() override;
    SR_DLL_IMPORT srBinOStream& operator=(const srBinOStream& stream);
#endif

    SR_DLL_IMPORT srBinOStream& putChar(char value);
    SR_DLL_IMPORT srBinOStream& putDWord(w8_ulong value);
    SR_DLL_IMPORT srBinOStream& putDouble(double value);
    SR_DLL_IMPORT srBinOStream& putFloat(float value);
    SR_DLL_IMPORT srBinOStream& putQWord(srQuadWord value);
    SR_DLL_IMPORT srBinOStream& putWord(unsigned short value);
    SR_DLL_IMPORT srBinOStream& write(const void* source, w8_ulong size);

protected:
    virtual SR_DLL_IMPORT unsigned short vput(char value);

private:
    virtual w8_ulong vwrite(const void* source, w8_ulong size) = 0;
};

// Memory-backed output stream.
// VTABLE: SURRENDER 0x10076BB0 srBinStream
// VTABLE: SURRENDER 0x10076BC4 srBinOStream
// class srBinOMStream
#if defined(SURRENDER_BUILD)
class SR_DLL_EXPORT srBinOMStream
#else
class __declspec(novtable) SR_DLL_IMPORT srBinOMStream
#endif
    : public srBinOStream {
public:
    srBinOMStream();
    /* Copy construction and destruction are consistent with ordinary member
       lifecycle. The default constructor initializes stream state. */

#if !defined(SURRENDER_BUILD)
    srBinOMStream(const srBinOMStream& stream);
    virtual ~srBinOMStream() override {}
    srBinOMStream& operator=(const srBinOMStream& stream);
#endif

    void* getPtr();
    virtual w8_ulong getSize() override;
    virtual srBinStream& seek(w8_ulong position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(w8_ulong position) override;
    virtual w8_ulong tell() override;

private:
    virtual w8_ulong vwrite(const void* source, w8_ulong size) override;

    srArray<unsigned char> buffer;
    w8_ulong position0;
    w8_ulong size;
};

W8_ABI_ASSERT(sizeof(srBinOStream) == 0x18, "srBinOStream_must_be_0x18");
W8_ABI_ASSERT(sizeof(srBinOMStream) == 0x2c, "srBinOMStream_must_be_0x2c");
