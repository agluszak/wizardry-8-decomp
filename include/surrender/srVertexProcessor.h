#pragma once

#include "srFlags.h"
#include "srHeap.h"
#include "srMath.h"

class srMaterialIFace;
class srShader;
class srVertexPipe;

/* Exact exported renderer APIs pass this 0x20-byte value by reference, and
   srTriMeshPipeline's array instantiation advances by the same 0x20 stride. */
struct srVertexArray {
    srVector4T<float>* values_00;
    srVector4T<float>* values_04;
    srVector4T<float>* values_08;
    srVector2T<float>* values_0c;
    srVector2T<float>* values_10;
    float* values_14;
    float* values_18;
    unsigned char* values_1c;
};

static_assert(sizeof(srVertexArray) == 0x20,
              "srVertexArray_must_be_0x20");

/* SR.DLL exports secondary vtables qualified as srVertexProcessor for
   srIlluminator, srFog and srLight. Each has exactly three slots: a destructor,
   isActive and process. Wizardry's four-byte global processor at 0x0065BEA8 is
   followed by independently used storage at 0x0065BEAF, and its constructor at
   0x004B89A0 writes only the vptr. That complete-object allocation proves this
   interface has no data beyond its vptr. The 0x2c-byte tail previously placed
   here belongs to srIlluminator, whose secondary base starts at +0x138. */
#pragma pack(push, 4)
class srVertexProcessor {
public:
    struct MaterialInfo {
// FUNCTION: WIZ8 0x00424A80
        inline MaterialInfo() : flags(0) {}

        srVector4T<float> diffuse;           /* 0x00 */
        srVector4T<float> ambient;           /* 0x10 */
        srVector4T<float> specular;          /* 0x20 */
        float translucency;                  /* 0x30 */
        float shininess;                     /* 0x34 */
        float value_38;                      /* 0x38 */
        srVector4T<float> emissive;          /* 0x3c */
        float value_4c;                      /* 0x4c */
        unsigned long flags;                 /* 0x50 */
    };

    enum e_channel {};

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
#pragma pack(pop)

static_assert(
    sizeof(srVertexProcessor) == 0x04,
    "srVertexProcessor_must_be_0x04");
static_assert(
    sizeof(srVertexProcessor::MaterialInfo) == 0x54,
    "srVertexProcessor_MaterialInfo_must_be_0x54");

class srVertexPipe {
public:
    struct Input {
        unsigned long record_count_00;
        unsigned long vertex_count_04;
        const unsigned long* indices_08;
        int position_is_float3_0c;
        const srVector3T<float>* positions_10;
        const void* values_14;
        srVector3T<float> eye_center_18;
        float eye_radius_24;
        const srMatrix4T<float>* model_view_28;
        const srMatrix4T<float>* normal_matrix_2c;
        srVertexArray* vertex_arrays_30;
        unsigned long exclusion_mask_34;
        srVector4T<float> ambient_light_38;
        const void* records_48;
        srVertexProcessor** processors_4c;
        unsigned long processor_count_50;
        float environment_minimum_54;
        float environment_maximum_58;
        float environment_scale_5c;
        float environment_inverse_scale_60;
    };

    SR_DLL_IMPORT srVertexPipe();
    SR_DLL_IMPORT ~srVertexPipe();
    SR_DLL_IMPORT srVertexPipe& operator=(const srVertexPipe& other);

    SR_DLL_IMPORT void applyDiffuseLight(
        const srVector4T<float>& light);
    SR_DLL_IMPORT void applyDiffuseLight(
        const float* values, const srVector4T<float>& light);
    SR_DLL_IMPORT void applyFog(const float* values);
    SR_DLL_IMPORT void copyDiffuseToSpecular();
    SR_DLL_IMPORT void copySpecularToDiffuse();
    SR_DLL_IMPORT void disableChannel(srVertexProcessor::e_channel channel);
    SR_DLL_IMPORT void enableChannel(srVertexProcessor::e_channel channel);
    SR_DLL_IMPORT const unsigned long* getAVT() const;
    SR_DLL_IMPORT float* getAlpha();
    SR_DLL_IMPORT srFlags<srVertexProcessor::e_channel> getChannelMask() const;
    SR_DLL_IMPORT const float* getDepthCue();
    SR_DLL_IMPORT srVector4T<float>* getDiffuse();
    SR_DLL_IMPORT unsigned long getExclusionMask() const;
    SR_DLL_IMPORT void getEyeSpaceBoundingSphere(
        srVector3T<float>& center, float& radius) const;
    SR_DLL_IMPORT const srVector3T<float>* getEyeSpaceDir();
    SR_DLL_IMPORT const float* getEyeSpaceDist();
    SR_DLL_IMPORT const srVector4T<float>* getEyeSpaceLocation();
    SR_DLL_IMPORT const srVector3T<float>* getEyeSpaceNormal();
    SR_DLL_IMPORT const float* getEyeSpaceZDist();
    SR_DLL_IMPORT float* getFog();
    SR_DLL_IMPORT const srVertexProcessor::MaterialInfo& getMaterialInfo() const;
    SR_DLL_IMPORT float* getQ(unsigned long index, int create);
    SR_DLL_IMPORT srVector2T<float>* getST(unsigned long index, int create);
    static SR_DLL_IMPORT srFlags<srVertexProcessor::e_channel>
    getShaderDisableMask(const srShader& shader);
    static SR_DLL_IMPORT srFlags<srVertexProcessor::e_channel>
    getShaderDisableMask(
        const srShader* shader,
        const unsigned long* channels,
        unsigned long channel_count);
    SR_DLL_IMPORT srVector4T<float>* getSpecular();
    SR_DLL_IMPORT void* getUserArray(unsigned long index);
    SR_DLL_IMPORT unsigned long getVertexCount() const;
    SR_DLL_IMPORT int isChannelAvailable(
        srVertexProcessor::e_channel channel) const;
    SR_DLL_IMPORT void process(const Input& input);
    SR_DLL_IMPORT void swapDiffuseAndSpecular();
    SR_DLL_IMPORT int testEyeSpaceBounds(
        const srVector3T<float>& center, float radius) const;

private:
    SR_DLL_IMPORT void finishDiffuseAlpha();
    SR_DLL_IMPORT void finishSpecularFog();
    SR_DLL_IMPORT void processVertexBuffer();
    static SR_DLL_IMPORT unsigned long scanChangeIndexed(
        const unsigned long* first,
        unsigned long first_count,
        const unsigned long* second,
        unsigned long second_count);
    SR_DLL_IMPORT void setMaterial(srMaterialIFace* material);
    SR_DLL_IMPORT void setupAlpha();
    SR_DLL_IMPORT void setupDepthCue();
    SR_DLL_IMPORT void setupDiffuse();
    SR_DLL_IMPORT void setupEyeSpaceDirAndDist();
    SR_DLL_IMPORT void setupEyeSpaceNormal();
    SR_DLL_IMPORT void setupEyeSpaceZDist();
    SR_DLL_IMPORT void setupFog();
    SR_DLL_IMPORT void setupQ(unsigned long index);
    SR_DLL_IMPORT void setupST(unsigned long index);
    SR_DLL_IMPORT void setupSpecular();

    unsigned char unknown_00_[0x9c];
};

static_assert(sizeof(srVertexPipe) == 0x9c, "srVertexPipe_must_be_0x9c");
static_assert(sizeof(srVertexPipe::Input) == 0x64,
              "srVertexPipe_Input_must_be_0x64");
