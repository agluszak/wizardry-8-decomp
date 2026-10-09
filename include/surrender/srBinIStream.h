#pragma once

#include "srBinStream.h"
#include "srMath.h"
#include "srQuadWord.h"

// srBinStream is a virtual base of both directional stream interfaces.
// VTABLE: SURRENDER 0x100769FC srBinStream
// VTABLE: SURRENDER 0x10076A10 srBinIStream
// class srBinIStream
class SR_DLL_IMPORT SR_DLL_EXPORT srBinIStream : public virtual srBinStream {
public:
#if !defined(SURRENDER_BUILD)
    srBinIStream() {}
    srBinIStream(const srBinIStream& stream);
    virtual ~srBinIStream() override {}
    srBinIStream& operator=(const srBinIStream& stream);
#endif

    unsigned short getChar();
    w8_ulong getDWord();
    double getDouble();
    float getFloat();
    srQuadWord getQuadWord();
    unsigned short getWord();
    srBinIStream& read(void* destination, w8_ulong size);

protected:
    virtual unsigned short vget();

private:
    virtual w8_ulong vread(void* destination, w8_ulong size) = 0;
};

srBinIStream& operator>>(srBinIStream& stream, int& value);
srBinIStream& operator>>(srBinIStream& stream, char& value);
SR_DLL_IMPORT srBinIStream& operator>>(srBinIStream& stream, unsigned char& value);
srBinIStream& operator>>(srBinIStream& stream, short& value);
SR_DLL_IMPORT srBinIStream& operator>>(srBinIStream& stream, unsigned short& value);
srBinIStream& operator>>(srBinIStream& stream, w8_long& value);
SR_DLL_IMPORT srBinIStream& operator>>(srBinIStream& stream, w8_ulong& value);
srBinIStream& operator>>(srBinIStream& stream, srQuadWord& value);
SR_DLL_IMPORT srBinIStream& operator>>(srBinIStream& stream, float& value);
srBinIStream& operator>>(srBinIStream& stream, double& value);
srBinIStream& operator>>(srBinIStream& stream, srVector2T<float>& value);
srBinIStream& operator>>(srBinIStream& stream, srVector2T<double>& value);
srBinIStream& operator>>(srBinIStream& stream, srVector3T<float>& value);
srBinIStream& operator>>(srBinIStream& stream, srVector3T<double>& value);
srBinIStream& operator>>(srBinIStream& stream, srVector4T<float>& value);
srBinIStream& operator>>(srBinIStream& stream, srVector4T<double>& value);
srBinIStream& operator>>(srBinIStream& stream, srVector2i& value);
srBinIStream& operator>>(srBinIStream& stream, srVector3i& value);
srBinIStream& operator>>(srBinIStream& stream, srVector4i& value);
srBinIStream& operator>>(srBinIStream& stream, srQuaternion& value);
srBinIStream& operator>>(srBinIStream& stream, srMatrix2T<float>& value);
srBinIStream& operator>>(srBinIStream& stream, srMatrix3T<float>& value);
srBinIStream& operator>>(srBinIStream& stream, srMatrix4T<float>& value);
srBinIStream& operator>>(srBinIStream& stream, srMatrix2T<double>& value);
srBinIStream& operator>>(srBinIStream& stream, srMatrix3T<double>& value);
srBinIStream& operator>>(srBinIStream& stream, srMatrix4T<double>& value);

// VTABLE: SURRENDER 0x10076B80 srBinStream
// VTABLE: SURRENDER 0x10076B94 srBinIStream
// class srBinIMStream
#if defined(SURRENDER_BUILD)
class SR_DLL_EXPORT srBinIMStream
#else
class __declspec(novtable) SR_DLL_IMPORT srBinIMStream
#endif
    : public srBinIStream {
public:
    srBinIMStream(const void* data, w8_ulong size);

#if !defined(SURRENDER_BUILD)
    virtual ~srBinIMStream() override {}
#endif

    virtual w8_ulong getSize() override;
    virtual srBinStream& seek(w8_ulong position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(w8_ulong position) override;
    virtual w8_ulong tell() override;

private:
    virtual w8_ulong vread(void* destination, w8_ulong size) override;

    const unsigned char* data;
    w8_ulong size;
    w8_ulong position0;
};

W8_ABI_ASSERT(sizeof(srBinIStream) == 0x18, "srBinIStream_must_be_0x18");
W8_ABI_ASSERT(sizeof(srBinIMStream) == 0x28, "srBinIMStream_must_be_0x28");
