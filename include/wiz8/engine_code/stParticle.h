#pragma once

#include "surrender/srMeshModel.h"
#include "surrender/srScene.h"

class W8MonsterShakeCallback;
class srGERD;
class stTextureAnim;

class stParticle
    : public srClassSupport<stParticle, srNode, 0, 0x10009> {
public:
    static const char* sGetClassName() { return "stParticle"; }
    stParticle(srNode* parent, unsigned int count); /* 0x00497AF0 */
    stParticle(const stParticle& other);           /* 0x00498180 */
    void SetActive(unsigned char active);
    void SetTraversalEnabled00498D90(unsigned char enabled);
    void DeactivateParticle00499F70(unsigned int index);
    unsigned char ActivateParticle00499A50(
        unsigned int* out_index, unsigned char replace_when_full);
    void InitializeParticlePosition0049A990(srVector3T<float>* output);
    void SetTexture0049AB00(srTextureIFace* texture);
    void SetRetainedObject0049ACA0(srMaterialIFace* material);
    void SetRenderFlags004925A0(srShader flags);
    void SetFlutter0049AD10(int enabled);
    void Function4994D0(srGERD* renderer);
    /* The per-particle age/cull/move step and billboard-corner expansion used
       by the submitted batch. Their retail names remain unavailable. */
    void Update00499FA0();                          /* 0x00499FA0 */
    void PrepareRenderer00498DD0(srMatrix4T<float>& view); /* 0x00498DD0 */
    srShader GetRenderFlags00498A10() const;
    unsigned char ReplaceTexture0049AC30(
        const char* old_name, srTextureIFace* replacement);
    virtual srClass* vInstance() override;         /* 0x004980E0 */
    virtual void traverse(srNode::TraverseInfo& info) override; /* 0x00498C40 */
    virtual void process(
        const srNode::ProcessInfo& info, srNode::e_processType type) override; /* 0x00498D60 */

protected:
    virtual ~stParticle() override;                /* 0x00498A20 */

public:

    unsigned int value_138;
    unsigned char unknown_13c[4];
    double value_140;
    srVector3T<float>* allocation_148;
    srMaterialIFace* retained_14c;
    srShader render_flags_150;
    srTextureIFace* texture_154;
    unsigned int vertex_count_158;
    unsigned int texture_frame_count_15c;
    srVector3T<float>* allocation_160;
    srVector2T<float>* allocation_164;
    srVector3i* allocation_168;
    void* allocation_16c;
    void* allocation_170;
    float* allocation_174;
    stTextureAnim** texture_frames_178;
    float* m_pflFlutterAngle;                     /* 0x17c */
    unsigned int particle_count_180;
    /* Both unsigned: 0x004994D0 gates the particle off with the unsigned
       `state_184 != 0 && state_184 <= value_188` pair. */
    unsigned int state_184;
    unsigned int value_188;
    unsigned int active_particle_count_18c;
    unsigned char active_190;
    unsigned char unknown_191;
    unsigned char trigger_flag_192;
    unsigned char unknown_193;
    unsigned char* allocation_194;
    srVector3T<float>* allocation_198;
    /* Unsigned millisecond birth ticks, one per particle. */
    unsigned int* allocation_19c;
    unsigned char active_1a0;
    unsigned char flag_1a1;
    unsigned char unknown_1a2[2];
    int value_1a4;
    int value_1a8;
    int value_1ac;
    int value_1b0;
    int value_1b4;
    int value_1b8;
    int value_1bc;
    int value_1c0;
    int value_1c4;
    /* Emission interval; elapsed comparisons use unsigned subtraction. */
    unsigned int value_1c8;
    /* Lifetime added to each absolute unsigned birth tick. */
    unsigned int value_1cc;
    srVector3T<float> minimum_1d0;
    srVector3T<float> maximum_1dc;
    srVector3T<float> direction_1e8;
    srVector3T<float> acceleration_1f4;
    float value_200;
    /* 0x00498DD0 uses this as an unsigned modulus period. */
    unsigned int value_204;
    float value_208;
    float value_20c;
    float value_210;
    float value_214;
    float value_218;
    srVector3T<float> minimum_21c;
    srVector3T<float> maximum_228;
    srVector3T<float> value_234;
    float value_240;
    /* Added to the camera position when value_1c4 selects camera-relative
       placement. */
    srVector3T<float> camera_offset_244;
    unsigned int update_flags_250;
    /* Index pairs, two per still-active particle, rebuilt whenever
       update_flags_250 carries bit 1. */
    unsigned long* allocation_254;
    /* Last accepted particle-integration tick. */
    unsigned int activated_at_258;
    /* Emission schedule tick. */
    unsigned int updated_at_25c;
    short value_260;
    unsigned char unknown_262[2];
    int start_frame_264;
    int end_frame_268;
    W8MonsterShakeCallback* callback_26c;
    unsigned int value_270;
    unsigned int value_274;
    float value_278;
    unsigned char unknown_27c[4];
};

static_assert(sizeof(stParticle) == 0x280, "stParticle_size_must_be_0x280");

stParticle* FindRegisteredParticle0049ADB0(const char* name);
void SaveParticleStates0049B150(unsigned int handle);
void LoadParticleStates0049B3B0(int handle);
