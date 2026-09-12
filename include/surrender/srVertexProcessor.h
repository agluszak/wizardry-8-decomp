#pragma once

#include "srFlags.h"
#include "srHeap.h"
#include "srMath.h"
#include "srShader.h"

class srMaterialIFace;
class srVertexPipe;

/* Exact exported renderer APIs pass this 0x20-byte value by reference, and
   srTriMeshPipeline's array instantiation advances by the same 0x20 stride.
   Slot order follows getEyeSpaceLocation/getDiffuse/getSpecular/getST/getQ. */
struct srVertexArray {
    srVector4T<float>* eye_locations_00;
    srVector4T<float>* diffuse_04;
    srVector4T<float>* specular_08;
    srVector2T<float>* st0_0c;
    srVector2T<float>* st1_10;
    float* q0_14;
    float* q1_18;
    unsigned char* packed_1c;
};

static_assert(sizeof(srVertexArray) == 0x20, "srVertexArray_must_be_0x20");

/* SR.DLL exports secondary vtables qualified as srVertexProcessor for
   srIlluminator, srFog and srLight. Each has exactly three slots: a destructor,
   isActive and process. Wizardry's four-byte global processor at 0x0065BEA8 is
   followed by independently used storage at 0x0065BEAF, and its constructor at
   0x004B89A0 writes only the vptr. That complete-object allocation proves this
   interface has no data beyond its vptr. The 0x2c-byte tail previously placed
   here belongs to srIlluminator, whose secondary base starts at +0x138. */
#pragma pack(push, 4)
// VTABLE: SURRENDER 0x10076c74 srVertexProcessor
class srVertexProcessor {
public:
    struct MaterialInfo {
        // FUNCTION: WIZ8 0x00424A80
        inline MaterialInfo() : flags(0) {}

        srVector4T<float> diffuse;  /* 0x00 */
        srVector4T<float> ambient;  /* 0x10 */
        srVector4T<float> specular; /* 0x20 */
        float translucency;         /* 0x30 */
        float shininess;            /* 0x34 */
        float value_38;             /* 0x38 */
        srVector4T<float> emissive; /* 0x3c */
        float value_4c;             /* 0x4c */
        unsigned long flags;        /* 0x50 */
    };

    /* Bit indices for enableChannel/getChannelMask. setupDiffuse enables 1,
       setupSpecular 2, setupAlpha 3, setupFog 4, setupST(0) 5, setupST(1) 6,
       setupQ(0) 7, setupQ(1) 8. srLight::process uses 9 and 10. Wizardry's
       environment mapper requires channel 5 before getST(0). */
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
    /* Header-visible, like srIlluminator's and srLight's: the srIlluminator
       level of 0x0049C430 stores this subobject's vptr through the guarded
       pointer and falls straight into the registry teardown, with no call to
       a secondary-base destructor in between. The out-of-line copy below is
       the COMDAT the secondary vtables need, not a separate definition. */
    // FUNCTION: WIZ8 0x0042A360
    virtual ~srVertexProcessor() {}

public:
    virtual int isActive(srVertexPipe& pipe) = 0;
    virtual void process(srVertexPipe& pipe) = 0;

public:
};

// SYNTHETIC: WIZ8 0x0042B890
// srVertexProcessor::`scalar deleting destructor'

#pragma pack(pop)

static_assert(sizeof(srVertexProcessor) == 0x04, "srVertexProcessor_must_be_0x04");
static_assert(sizeof(srVertexProcessor::MaterialInfo) == 0x54,
              "srVertexProcessor_MaterialInfo_must_be_0x54");
