#pragma once

#include "srBinIStream.h"

class srBinOStream : public virtual srBinStream {
public:
    virtual ~srBinOStream() {}
    SR_DLL_IMPORT srBinOStream& write(const void* source, unsigned long size);
};
