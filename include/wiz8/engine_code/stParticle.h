#pragma once

#include "surrender/srMaterialIFace.h"
#include "surrender/srScene.h"
#include "surrender/srShader.h"
#include "surrender/srTextureIFace.h"

class W8MonsterShakeCallback;
class srGERD;
class stTextureAnim;

class stParticle : public srClassSupport<stParticle, srNode, 0, 0x10009> {
public:
    static const char* sGetClassName()
    {
        return "stParticle";
    }
    stParticle(srNode* parent, unsigned int count); /* 0x00497AF0 */
    stParticle(const stParticle& other);            /* 0x00498180 */
    void SetActive(unsigned char active);
    void SetTraversalEnabled00498D90(unsigned char enabled);
    void DeactivateParticle00499F70(unsigned int index);
    unsigned char ActivateParticle00499A50(unsigned int* out_index,
                                           unsigned char replace_when_full);
    void InitializeParticlePosition0049A990(srVector3T<float>* output);
    void SetTexture0049AB00(srTextureIFace* texture);
    void SetRetainedObject0049ACA0(srMaterialIFace* material);
    void SetRenderFlags004925A0(srShader flags);
    void SetFlutter0049AD10(int enabled);
    void SetParticleScale(float scale);
    void SubmitToRenderer(srGERD* renderer);
    /* The per-particle age/cull/move step and billboard-corner expansion used
       by the submitted batch. Their retail names remain unavailable. */
    void Update00499FA0();                                 /* 0x00499FA0 */
    void PrepareRenderer00498DD0(srMatrix4T<float>& view); /* 0x00498DD0 */
    srShader GetRenderFlags00498A10() const;
    unsigned char ReplaceTexture0049AC30(const char* old_name, srTextureIFace* replacement);
    virtual srClass* vInstance() override;                      /* 0x004980E0 */
    virtual void traverse(srNode::TraverseInfo& info) override; /* 0x00498C40 */
    virtual void process(const srNode::ProcessInfo& info,
                         srNode::e_processType type) override; /* 0x00498D60 */

protected:
    virtual ~stParticle() override; /* 0x00498A20 */

public:
    unsigned int requires_positional_138;
    unsigned char padding_13c[4];
    double particle_size_140; /* 0x140: billboard quad scale from particle_size */
    /* Per-particle world positions; the retail allocation assert spells the
       buffer pParticle. */
    srVector3T<float>* particle_positions_148;
    srMaterialIFace* retained_14c;
    srShader render_flags_150;
    srTextureIFace* texture_154;
    unsigned int vertex_count_158;
    /* particle_count_180 * 2 - the billboard triangle count, and the length of
       texture_frames_178 where consecutive pairs share one frame. */
    unsigned int texture_frame_count_15c;
    /* Per-vertex billboard corners (assert pVertex), texture UVs (pTexCoord)
       and triangle index triples; srHeap-allocated, vertex_count_158 /
       texture_frame_count_15c long. */
    srVector3T<float>* vertex_positions_160;
    srVector2T<float>* texcoords_164;
    srVector3i* triangles_168;
    /* Optional per-vertex arrays handed to the record/pipeline color and
       extra slots (dig-format vec3 colors and vertex extras/normals). Retail
       never allocates them - both stay null in every recovered path. */
    srVector3T<float>* colors_16c;
    srVector3T<float>* vertex_extras_170;
    float* alphas_174;
    stTextureAnim** texture_frames_178;
    float* m_pflFlutterAngle; /* 0x17c */
    unsigned int particle_count_180;
    /* Both unsigned: 0x004994D0 gates the particle off with the unsigned
       `emission_limit_184 != 0 && emission_limit_184 <= emission_count_188` pair. */
    unsigned int emission_limit_184;
    unsigned int emission_count_188;
    unsigned int active_particle_count_18c;
    bool release_when_done_190;
    unsigned char replace_when_full_191;
    unsigned char persisted_192;
    unsigned char padding_193;
    /* Per-particle liveness flag byte; the update loop retires it when the
       birth tick plus lifetime expires. */
    unsigned char* particle_active_194;
    /* Per-particle velocity; acceleration_1f4 integrates it each update. */
    srVector3T<float>* velocities_198;
    /* Unsigned millisecond birth ticks, one per particle. */
    unsigned int* birth_ticks_19c;
    unsigned char emitting_1a0;
    unsigned char traversal_enabled_1a1;
    unsigned char padding_1a2[2];
    int bounds_mode_1a4;
    int has_acceleration_1a8;
    int expiry_mode_1ac;
    int emission_mode_1b0;
    int los_check_enabled_1b4;
    int direction_mode_1b8;
    int placement_mode_1bc;
    int flutter_mode_1c0;
    int camera_relative_1c4;
    /* Emission interval; elapsed comparisons use unsigned subtraction. */
    unsigned int emission_interval_1c8;
    /* Lifetime added to each absolute unsigned birth tick. */
    unsigned int lifetime_ms_1cc;
    srVector3T<float> minimum_1d0;
    srVector3T<float> maximum_1dc;
    srVector3T<float> direction_1e8;
    srVector3T<float> acceleration_1f4;
    float flutter_amplitude_200;
    /* 0x00498DD0 uses this as an unsigned modulus period. */
    unsigned int flutter_period_204;
    float cone_yaw_208;
    float cone_pitch_20c;
    float initial_speed_210;
    float speed_min_214;
    float speed_max_218;
    srVector3T<float> minimum_21c;
    srVector3T<float> maximum_228;
    srVector3T<float> bounds_origin_234;
    float bounds_radius_240;
    /* Added to the camera position when camera_relative_1c4 selects camera-relative
       placement. */
    srVector3T<float> camera_offset_244;
    unsigned int update_flags_250;
    /* Index pairs, two per still-active particle, rebuilt whenever
       update_flags_250 carries bit 1. */
    unsigned long* active_triangles_254;
    /* Last accepted particle-integration tick. */
    unsigned int activated_at_258;
    /* Emission schedule tick. */
    unsigned int updated_at_25c;
    short attachment_key_260;
    unsigned char padding_262[2];
    int start_frame_264;
    int end_frame_268;
    W8MonsterShakeCallback* callback_26c;
    unsigned int emission_gap_270;
    unsigned int last_emitted_at_274;
    float size_scale_278;
    unsigned char padding_27c[4];
};

static_assert(sizeof(stParticle) == 0x280, "stParticle_size_must_be_0x280");

stParticle* FindRegisteredParticle0049ADB0(const char* name);
void SaveParticleStates0049B150(unsigned int handle);
void LoadParticleStates0049B3B0(int handle);
