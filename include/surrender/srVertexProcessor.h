#pragma once

#include "srFlags.h"
#include "srHeap.h"
#include "srMath.h"

class srMaterialIFace;
class srShader;
class srVertexPipe;

/* SR.DLL exports secondary vtables qualified as srVertexProcessor for
   srIlluminator, srFog and srLight. Each has exactly three slots: a destructor,
   isActive and process. Wiz8's concrete fog allocation proves that this base
   begins at +0x138 and occupies 0x30 bytes; the larger light-only tail belongs
   to srLight rather than to this common base. */
#pragma pack(push, 4)
class srVertexProcessor {
public:
    struct MaterialInfo {
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
    virtual ~srVertexProcessor();

public:
    virtual int isActive(srVertexPipe& pipe) = 0;
    virtual void process(srVertexPipe& pipe) = 0;

public:
    unsigned char unknown_04_[0x14];
    union {
        int m_positional_18;
        double m_positional_double_18;
    };
    union {
        double m_positional_double_20;
        struct {
            float m_positional_20;
            float m_positional_24;
        };
    };
    float m_positional_28;
    unsigned int unknown_2c;
};
#pragma pack(pop)

static_assert(
    sizeof(srVertexProcessor) == 0x30,
    "srVertexProcessor_must_be_0x30");
static_assert(
    sizeof(srVertexProcessor::MaterialInfo) == 0x54,
    "srVertexProcessor_MaterialInfo_must_be_0x54");

class srVertexPipe {
public:
    struct Input;

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
