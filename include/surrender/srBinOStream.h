#pragma once

#include "srBinIStream.h"

// Like srBinIStream, this is an abstract directional interface.  Its own
// virtual slot provides the primary vptr at +0, and the original JPEG exporter
// consequently reaches the virtual srBinStream base through the vbptr at +4.
// novtable prevents this SDK boundary from emitting an abstract local table.
class __declspec(novtable) srBinOStream : public virtual srBinStream {
public:
    virtual ~srBinOStream() {}
    SR_DLL_IMPORT srBinOStream& write(const void* source, unsigned long size);

protected:
    // Exported by SR.DLL as ?vput@srBinOStream@@MAEGD@Z.  Besides completing
    // the ABI, this gives the directional interface its own primary vptr;
    // its virtual-base pointer consequently lives at +4 as observed in the
    // original JPEG exporter.
    virtual SR_DLL_IMPORT unsigned short vput(char value);
};
