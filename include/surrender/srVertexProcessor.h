#pragma once

#include "srFlags.h"
#include "srHeap.h"
#include "srMath.h"
#include "srShader.h"

class srMaterialIFace;
class srVertexPipe;

struct srVertexArray {
    enum {
        ATTRIBUTE_DIFFUSE = 0x01u,
        ATTRIBUTE_SPECULAR = 0x02u,
        ATTRIBUTE_SPECULAR_ALPHA = 0x04u,
        ATTRIBUTE_ST0 = 0x08u,
        ATTRIBUTE_ST1 = 0x10u,
        ATTRIBUTE_Q0 = 0x20u,
        ATTRIBUTE_Q1 = 0x40u
    };
    srVector4T<float>* eye_locations;
    srVector4T<float>* diffuse;
    srVector4T<float>* specular;
    srVector2T<float>* st0;
    srVector2T<float>* st1;
    float* q0;
    float* q1;
    unsigned char* attributes;
};

W8_ABI_ASSERT(sizeof(srVertexArray) == 0x20, "srVertexArray_must_be_0x20");

/* Interface with no data beyond its vptr. */
#pragma pack(push, 4)
// VTABLE: SURRENDER 0x10076c74 srVertexProcessor
class srVertexProcessor {
public:
    struct MaterialInfo {
        // FUNCTION: WIZ8 0x00424A80
        inline MaterialInfo() : disabled_channels(0) {}

        srVector4T<float> diffuse;  /* 0x00 */
        srVector4T<float> ambient;  /* 0x10 */
        srVector4T<float> specular; /* 0x20 */
        float translucency;         /* 0x30 */
        float shininess;            /* 0x34 */
        float value_38;             /* 0x38 */
        srVector4T<float> emissive; /* 0x3c */
        float fog_scale;            /* 0x4c */
        w8_ulong disabled_channels; /* 0x50 */
    };

    /* Bit indices for enableChannel/getChannelMask. */
    enum e_channel {
        CHANNEL_DIFFUSE = 1,
        CHANNEL_SPECULAR = 2,
        CHANNEL_ALPHA = 3,
        CHANNEL_FOG = 4,
        CHANNEL_ST0 = 5,
        CHANNEL_ST1 = 6,
        CHANNEL_Q0 = 7,
        CHANNEL_Q1 = 8,
        CHANNEL_LIGHT_AMBIENT = 9,
        CHANNEL_LIGHT_DIFFUSE = 10
    };

protected:
    // FUNCTION: WIZ8 0x0042A360
    virtual ~srVertexProcessor() {}

public:
    // The provider's base vtable contains these bodies, not _purecall.
    // FUNCTION: SURRENDER 0x10035470
    virtual int isActive(srVertexPipe& pipe)
    {
        return 1;
    }
    // FUNCTION: SURRENDER 0x10035480
    virtual void process(srVertexPipe& pipe) {}

public:
};

#pragma pack(pop)

W8_ABI_ASSERT(sizeof(srVertexProcessor) == 0x04, "srVertexProcessor_must_be_0x04");
W8_ABI_ASSERT(sizeof(srVertexProcessor::MaterialInfo) == 0x54,
              "srVertexProcessor_MaterialInfo_must_be_0x54");
