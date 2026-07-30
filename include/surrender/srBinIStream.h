#pragma once

#include "srBinStream.h"
#include "srQuadWord.h"

// The vbtable accesses in the JPEG extension prove that srBinStream is a
// virtual base of both directional stream interfaces.
// ??_7srBinIStream@@6B0@@ has two slots: vget, which the library implements,
// and one holding the pure-virtual stub. Both are introduced here rather than
// inherited, because the destructor override lands in the srBinStream subobject
// table instead. The pure slot is what every reader supplies - srBinIMStream
// with its own vread, and Wizardry's virtual-file adapter with its.
class __declspec(novtable) srBinIStream : public virtual srBinStream {
public:
    SR_DLL_IMPORT srBinIStream();
    SR_DLL_IMPORT srBinIStream(const srBinIStream& stream);
    virtual SR_DLL_IMPORT ~srBinIStream() override;
    SR_DLL_IMPORT srBinIStream& operator=(const srBinIStream& stream);

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

class __declspec(novtable) srBinIMStream : public srBinIStream {
public:
    SR_DLL_IMPORT srBinIMStream(const void* data, unsigned long size);
    SR_DLL_IMPORT srBinIMStream(const srBinIMStream& stream);
    virtual SR_DLL_IMPORT ~srBinIMStream() override;
    SR_DLL_IMPORT srBinIMStream& operator=(const srBinIMStream& stream);

    virtual SR_DLL_IMPORT unsigned long getSize() override;
    virtual SR_DLL_IMPORT srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction) override;
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position) override;
    virtual SR_DLL_IMPORT unsigned long tell() override;

private:
    virtual SR_DLL_IMPORT unsigned long vread(
        void* destination, unsigned long size) override;
    const unsigned char* data_08;
    unsigned long size_0c;
    unsigned long position_10;
};

static_assert(sizeof(srBinIStream) == 0x18,
              "srBinIStream_must_be_0x18");
static_assert(sizeof(srBinIMStream) == 0x28,
              "srBinIMStream_must_be_0x28");
