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
    /* The reconstruction leaves provider lifecycle implicit. Wiz8 keeps the
       header-visible default/destructor and imports copy/assignment emissions. */

#if !defined(SURRENDER_BUILD)
    srBinIStream() {}
    SR_DLL_IMPORT srBinIStream(const srBinIStream& stream);
    virtual ~srBinIStream() override {}
    SR_DLL_IMPORT srBinIStream& operator=(const srBinIStream& stream);
#endif

    SR_DLL_IMPORT unsigned short getChar();
    SR_DLL_IMPORT unsigned long getDWord();
    SR_DLL_IMPORT double getDouble();
    SR_DLL_IMPORT float getFloat();
    SR_DLL_IMPORT srQuadWord getQuadWord();
    SR_DLL_IMPORT unsigned short getWord();
    SR_DLL_IMPORT srBinIStream& read(void* destination, unsigned long size);

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

    /* The emitted bodies are consistent with ordinary memberwise copying. */

    /* The reconstruction leaves provider destruction implicit; consumers retain the
       evidenced header-visible empty body. */

#if !defined(SURRENDER_BUILD)
    virtual ~srBinIMStream() override {}
#endif

    virtual SR_DLL_IMPORT unsigned long getSize() override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position,
                                            srBinStream::e_seekDir direction) override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position) override;
    virtual SR_DLL_IMPORT unsigned long tell() override;

private:
    virtual SR_DLL_IMPORT unsigned long vread(void* destination, unsigned long size) override;

    const unsigned char* data;
    unsigned long size;
    unsigned long position0;
};

static_assert(sizeof(srBinIStream) == 0x18, "srBinIStream_must_be_0x18");
static_assert(sizeof(srBinIMStream) == 0x28, "srBinIMStream_must_be_0x28");
