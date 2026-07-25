#pragma once

#include "srHeap.h"

class srBinStream {
public:
    enum e_seekDir {
        SR_SEEK_BEGIN = 0,
        SR_SEEK_CURRENT = 1,
        SR_SEEK_END = 2
    };

    virtual ~srBinStream() {}
    virtual unsigned long getSize() = 0;
    virtual srBinStream& seek(unsigned long position) = 0;
    virtual srBinStream& seek(unsigned long position, e_seekDir direction) = 0;
    virtual unsigned long tell() = 0;

    SR_DLL_IMPORT bool good() const;
};

// The vbtable accesses in the JPEG extension prove that srBinStream is a
// virtual base of both directional stream interfaces.
class srBinIStream : public virtual srBinStream {
public:
    virtual ~srBinIStream() {}
    SR_DLL_IMPORT srBinIStream& read(void* destination, unsigned long size);
};
