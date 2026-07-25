#pragma once

#include "srHeap.h"

class SR_DLL_IMPORT srBinStream {
public:
    enum e_seekDir {
        SR_SEEK_BEGIN = 0,
        SR_SEEK_CURRENT = 1,
        SR_SEEK_END = 2
    };

    virtual ~srBinStream() {}
    virtual unsigned long getSize() = 0;
    virtual srBinStream& seek(unsigned long position, e_seekDir direction) = 0;
    virtual srBinStream& seek(unsigned long position) = 0;
    virtual unsigned long tell() = 0;

    bool good() const;

protected:
    srBinStream();

private:
    // The ZIP extension's owned memory-stream subclass proves that the
    // virtual srBinStream subobject is 0x10 bytes: a vptr followed by 0x0c
    // bytes of SR.DLL-owned stream state.
    unsigned char unknown_04_[0x0c];
};

typedef char srBinStream_must_be_0x10[
    (sizeof(srBinStream) == 0x10) ? 1 : -1];

// The vbtable accesses in the JPEG extension prove that srBinStream is a
// virtual base of both directional stream interfaces.
class __declspec(novtable) srBinIStream : public virtual srBinStream {
public:
    virtual ~srBinIStream() {}
    SR_DLL_IMPORT srBinIStream& read(void* destination, unsigned long size);

protected:
    virtual SR_DLL_IMPORT unsigned short vget();
};

class __declspec(novtable) srBinIMStream : public srBinIStream {
public:
    SR_DLL_IMPORT srBinIMStream(const void* data, unsigned long size);
    virtual SR_DLL_IMPORT unsigned long getSize();
    virtual SR_DLL_IMPORT srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction);
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position);
    virtual SR_DLL_IMPORT unsigned long tell();

private:
    virtual SR_DLL_IMPORT unsigned long vread(
        void* destination, unsigned long size);
    // The imported implementation owns the prefix through +0x13. The ZIP
    // extension's zero-data wrapper adds its malloc owner at +0x14 and places
    // the virtual srBinStream base at +0x1c.
    unsigned char unknown_08_[0x0c];
};
