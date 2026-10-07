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
    unsigned long getDWord();
    double getDouble();
    float getFloat();
    srQuadWord getQuadWord();
    unsigned short getWord();
    srBinIStream& read(void* destination, unsigned long size);

protected:
    virtual unsigned short vget();

private:
    virtual unsigned long vread(void* destination, unsigned long size) = 0;
};

srBinIStream& operator>>(srBinIStream& stream, int& value);
srBinIStream& operator>>(srBinIStream& stream, char& value);
SR_DLL_IMPORT srBinIStream& operator>>(srBinIStream& stream, unsigned char& value);
srBinIStream& operator>>(srBinIStream& stream, short& value);
SR_DLL_IMPORT srBinIStream& operator>>(srBinIStream& stream, unsigned short& value);
srBinIStream& operator>>(srBinIStream& stream, long& value);
SR_DLL_IMPORT srBinIStream& operator>>(srBinIStream& stream, unsigned long& value);
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
    srBinIMStream(const void* data, unsigned long size);

#if !defined(SURRENDER_BUILD)
    virtual ~srBinIMStream() override {}
#endif

    virtual unsigned long getSize() override;
    virtual srBinStream& seek(unsigned long position, srBinStream::e_seekDir direction) override;
    virtual srBinStream& seek(unsigned long position) override;
    virtual unsigned long tell() override;

private:
    virtual unsigned long vread(void* destination, unsigned long size) override;

    const unsigned char* data;
    unsigned long size;
    unsigned long position0;
};

static_assert(sizeof(srBinIStream) == 0x18, "srBinIStream_must_be_0x18");
static_assert(sizeof(srBinIMStream) == 0x28, "srBinIMStream_must_be_0x28");
