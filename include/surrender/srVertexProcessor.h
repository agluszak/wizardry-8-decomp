#pragma once

#include "srHeap.h"
#include "srMath.h"

class srVertexPipe;

/* SR.DLL exports secondary vtables qualified as srVertexProcessor for
   srIlluminator, srFog and srLight. Each has exactly three slots: a destructor,
   isActive and process. Wiz8's concrete fog allocation proves that this base
   begins at +0x138 and occupies 0x30 bytes; the larger light-only tail belongs
   to srLight rather than to this common base. */
#pragma pack(push, 4)
class srVertexProcessor {
public:
    struct MaterialInfo;

protected:
    virtual ~srVertexProcessor() {}

public:
    virtual int isActive(srVertexPipe& pipe) = 0;
    virtual void process(srVertexPipe& pipe) = 0;

public:
    unsigned char unknown_04_[0x14];
    union {
        int m_positional_18;
        double m_positional_double_18;
    };
    double m_positional_double_20;
    float m_positional_28;
    unsigned char unknown_2c_[4];
};
#pragma pack(pop)

static_assert(
    sizeof(srVertexProcessor) == 0x30,
    "srVertexProcessor_must_be_0x30");
