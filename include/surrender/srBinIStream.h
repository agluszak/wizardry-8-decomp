#pragma once

#include "srHeap.h"

class srBinStream {
public:
    enum e_seekDir {
        SR_SEEK_BEGIN = 0,
        SR_SEEK_CURRENT = 1,
        SR_SEEK_END = 2
    };

    virtual SR_DLL_IMPORT ~srBinStream();
    virtual unsigned long getSize() = 0;
    virtual srBinStream& seek(unsigned long position) = 0;
    virtual srBinStream& seek(unsigned long position, e_seekDir direction) = 0;
    virtual unsigned long tell() = 0;

    SR_DLL_IMPORT bool good() const;

protected:
    SR_DLL_IMPORT srBinStream();
};

// The vbtable accesses in the JPEG extension prove that srBinStream is a
// virtual base of both directional stream interfaces.
class srBinIStream : public virtual srBinStream {
public:
    virtual ~srBinIStream() {}
    SR_DLL_IMPORT srBinIStream& read(void* destination, unsigned long size);

protected:
    virtual SR_DLL_IMPORT unsigned char vget();
};

class srBinIMStream : public srBinIStream {
public:
    SR_DLL_IMPORT srBinIMStream(const void* data, unsigned long size);
    virtual SR_DLL_IMPORT unsigned long getSize();
    virtual SR_DLL_IMPORT srBinStream& seek(unsigned long position);
    virtual SR_DLL_IMPORT srBinStream& seek(
        unsigned long position, srBinStream::e_seekDir direction);
    virtual SR_DLL_IMPORT unsigned long tell();

protected:
    virtual SR_DLL_IMPORT unsigned long vread(
        void* destination, unsigned long size);

private:
    // The imported implementation owns the prefix through +0x13. The ZIP
    // extension's zero-data wrapper adds its malloc owner at +0x14 and places
    // the virtual srBinStream base at +0x1c.
    unsigned char unknown_08_[0x0c];
};
