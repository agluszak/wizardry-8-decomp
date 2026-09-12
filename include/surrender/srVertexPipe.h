#pragma once

#include "srFlags.h"
#include "srMath.h"
#include "srVertexProcessor.h"

class srMaterialIFace;
class srVP;

#pragma pack(push, 4)
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

    SR_DLL_IMPORT void applyDiffuseLight(const srVector4T<float>& light);
    SR_DLL_IMPORT void applyDiffuseLight(const float* values, const srVector4T<float>& light);
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
    SR_DLL_IMPORT void getEyeSpaceBoundingSphere(srVector3T<float>& center, float& radius) const;
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
    getShaderDisableMask(const srShader* shader, const unsigned long* channels,
                         unsigned long channel_count);
    SR_DLL_IMPORT srVector4T<float>* getSpecular();
    SR_DLL_IMPORT void* getUserArray(unsigned long index);
    SR_DLL_IMPORT unsigned long getVertexCount() const;
    SR_DLL_IMPORT int isChannelAvailable(srVertexProcessor::e_channel channel) const;
    SR_DLL_IMPORT void process(const Input& input);
    SR_DLL_IMPORT void swapDiffuseAndSpecular();
    SR_DLL_IMPORT int testEyeSpaceBounds(const srVector3T<float>& center, float radius) const;

private:
    SR_DLL_IMPORT void finishDiffuseAlpha();
    SR_DLL_IMPORT void finishSpecularFog();
    SR_DLL_IMPORT void processVertexBuffer();
    static SR_DLL_IMPORT unsigned long scanChangeIndexed(const unsigned long* first,
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

    /* Getter/setup/process bodies in sr.dll. Scratch is operator_new(0xb04)
       with a flags dword at +0xb00. */
    void* scratch_00;                                 /* 0x00 */
    srVertexProcessor** processor_heap_04;            /* 0x04 */
    unsigned long processor_heap_capacity_08;         /* 0x08 */
    unsigned long channel_mask_0c;                    /* 0x0c */
    unsigned long lazy_setup_mask_10;                 /* 0x10 */
    srVertexProcessor::MaterialInfo material_info_14; /* 0x14 */
    unsigned long extra_disable_mask_64;              /* 0x64 */
    srMaterialIFace* material_68;                     /* 0x68 */
    const Input* input_6c;                            /* 0x6c */
    const unsigned long* avt_70;                      /* 0x70 */
    const void* current_record_74;                    /* 0x74 */
    srVertexArray* vertex_array_78;                   /* 0x78 */
    srVector4T<float>* eye_space_locations_7c;        /* 0x7c */
    unsigned long batch_base_80;                      /* 0x80 */
    unsigned long sub_batch_offset_84;                /* 0x84 */
    unsigned long vertex_count_88;                    /* 0x88 */
    unsigned long batch_count_8c;                     /* 0x8c */
    unsigned long active_processor_count_90;          /* 0x90 */
    srVertexProcessor** active_processors_94;         /* 0x94 */
    srVP* vector_processor_98;                        /* 0x98 */
};
#pragma pack(pop)

static_assert(sizeof(srVertexPipe) == 0x9c, "srVertexPipe_must_be_0x9c");
static_assert(sizeof(srVertexPipe::Input) == 0x64, "srVertexPipe_Input_must_be_0x64");
