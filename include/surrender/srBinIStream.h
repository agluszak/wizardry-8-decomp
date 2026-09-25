#pragma once

#include "srBinStream.h"
#include "srMath.h"
#include "srQuadWord.h"

// The vbtable accesses in the JPEG extension prove that srBinStream is a
// virtual base of both directional stream interfaces.
// ??_7srBinIStream@@6B0@@ has two slots: vget, which the library implements,
// and one holding the pure-virtual stub. Both are introduced here rather than
// inherited, because the destructor override lands in the srBinStream subobject
// table instead. The pure slot is what every reader supplies - srBinIMStream
// with its own vread, and Wizardry's virtual-file adapter with its.
/* The provider exports the full member surface including the lifecycle bodies
   and both vftables (??_7srBinIStream@@6B0@@ and {for `srBinStream'}), so the
   declaration is dllexport under SURRENDER_BUILD. Consumers keep novtable: they only
   import the member surface and expand the inline lifecycle locally. */
// VTABLE: SURRENDER 0x100769FC srBinStream
// VTABLE: SURRENDER 0x10076A10 srBinIStream
// class srBinIStream
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#else
    __declspec(novtable)
#endif
    srBinIStream : public virtual srBinStream {
public:
    /* The Wiz8 stream-adapter constructor expands this body inline instead of
       calling the imported emission. */
    // FUNCTION: SURRENDER 0x10031C40
    srBinIStream() {}
    SR_DLL_IMPORT srBinIStream(const srBinIStream& stream);

    /* The retail emission is a bare ret; the srBinStream subobject destructor
       owns the vbase table store. */
    // FUNCTION: SURRENDER 0x1002EF30
    virtual ~srBinIStream() override {}
    SR_DLL_IMPORT srBinIStream& operator=(const srBinIStream& stream);

    SR_DLL_IMPORT unsigned short getChar();
    SR_DLL_IMPORT unsigned long getDWord();
    SR_DLL_IMPORT double getDouble();
    SR_DLL_IMPORT float getFloat();
    SR_DLL_IMPORT srQuadWord getQuadWord();
    SR_DLL_IMPORT unsigned short getWord();
    SR_DLL_IMPORT srBinIStream& read(void* destination, unsigned long size);

    // SYNTHETIC: SURRENDER 0x10031D30
    // srBinIStream::`vbase destructor'

protected:
    virtual SR_DLL_IMPORT unsigned short vget();

private:
    virtual unsigned long vread(void* destination, unsigned long size) = 0;
};

// srEXT_LWO imports the unsigned char/unsigned short/unsigned long/float
// overloads and srHXImporter the unsigned long/float pair; the rest are
// provider-only. The provider exports all of them through sr.def like the
// free srDebugPrintf entry points.
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

// No known consumer imports the srBinIMStream vftable, so the consumer side
// stays member-level; every declared member below is imported by Wiz8.exe or
// srEXT_Unzip.
// VTABLE: SURRENDER 0x10076B80 srBinStream
// VTABLE: SURRENDER 0x10076B94 srBinIStream
// class srBinIMStream
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#else
    __declspec(novtable)
#endif
    srBinIMStream : public srBinIStream {
public:
    SR_DLL_IMPORT srBinIMStream(const void* data, unsigned long size);

    /* Implicit copy constructor/assignment: retail emits them via the
       class-level dllexport as memberwise copies. */
    // SYNTHETIC: SURRENDER 0x10030BB0
    // ??0srBinIMStream@@QAE@ABV0@@Z
    // SYNTHETIC: SURRENDER 0x10030C50
    // srBinIMStream::operator=

    /* The retail emission is a bare ret while the vbase destructor owns the
       table stores; consumers expand it inline rather than calling the import. */
    // FUNCTION: SURRENDER 0x10030CE0
    virtual ~srBinIMStream() override {}

    virtual SR_DLL_IMPORT unsigned long getSize() override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position,
                                            srBinStream::e_seekDir direction) override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position) override;
    virtual SR_DLL_IMPORT unsigned long tell() override;

private:
    virtual SR_DLL_IMPORT unsigned long vread(void* destination, unsigned long size) override;

    // SYNTHETIC: SURRENDER 0x10030CF0
    // srBinIMStream::`vbase destructor'

    const unsigned char* data_08;
    unsigned long size_0c;
    unsigned long position_10;
};

static_assert(sizeof(srBinIStream) == 0x18, "srBinIStream_must_be_0x18");
static_assert(sizeof(srBinIMStream) == 0x28, "srBinIMStream_must_be_0x28");
