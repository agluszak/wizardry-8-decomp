#pragma once

#include "wiz8/engine_code/particle_modes.h"

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
    /* 0x00497C57 compares count signed, while particle_count is stored and
       compared unsigned, so the parameter is not the member's type. */
    stParticle(srNode* parent, int count); /* 0x00497AF0 */
    stParticle(const stParticle& other);   /* 0x00498180 */
    void SetActive(unsigned char active);
    void SetTraversalEnabled(bool enabled);
    void DeactivateParticle(unsigned int index);
    unsigned char ActivateParticle(unsigned int* out_index, bool replace_when_full);
    void InitializeParticlePosition(srVector3T<float>* output);
    void SetTexture(srTextureIFace* texture);
    void SetRetainedObject(srMaterialIFace* material);
    void SetRenderFlags(srShader flags);
    void SetFlutter(W8ParticleFlutterMode mode);
    void SetParticleScale(float scale);
    void SubmitToRenderer(srGERD* renderer);
    /* The per-particle age/cull/move step and billboard-corner expansion used
       by the submitted batch. Their retail names remain unavailable. */
    void Update();                                 /* 0x00499FA0 */
    void PrepareRenderer(srMatrix4T<float>& view); /* 0x00498DD0 */
    srShader GetRenderFlags() const;
    unsigned char ReplaceTexture(const char* old_name, srTextureIFace* replacement);
    virtual srClass* vInstance() override;                      /* 0x004980E0 */
    virtual void traverse(srNode::TraverseInfo& info) override; /* 0x00498C40 */
    virtual void process(const srNode::ProcessInfo& info,
                         srNode::e_processType type) override; /* 0x00498D60 */

protected:
    virtual ~stParticle() override; /* 0x00498A20 */

public:
    unsigned int requires_sorted_renderer;
    unsigned char padding_13c[4];
    double particle_size; /* 0x140: billboard quad scale from particle_size */
    /* Per-particle world positions; the retail allocation assert spells the
       buffer pParticle. */
    srVector3T<float>* particle_positions;
    srMaterialIFace* retained;
    srShader render_flags;
    srTextureIFace* texture_154;
    unsigned int vertex_count;
    /* particle_count * 2 - the billboard triangle count, and the length of
       texture_frames where consecutive pairs share one frame. */
    unsigned int texture_frame_count;
    /* Per-vertex billboard corners (assert pVertex), texture UVs (pTexCoord)
       and triangle index triples; srHeap-allocated, vertex_count /
       texture_frame_count long. */
    srVector3T<float>* vertex_positions;
    srVector2T<float>* texcoords;
    srVector3i* triangles;
    /* Optional per-vertex arrays handed to the record/pipeline color and
       extra slots (dig-format vec3 colors and vertex extras/normals). Retail
       never allocates them - both stay null in every recovered path. */
    srVector3T<float>* colors;
    srVector3T<float>* vertex_extras;
    float* alphas;
    stTextureAnim** texture_frames;
    float* m_pflFlutterAngle; /* 0x17c */
    unsigned int particle_count;
    /* Both unsigned: 0x004994D0 gates the particle off with the unsigned
       `emission_limit != 0 && emission_limit <= emission_count` pair. */
    unsigned int emission_limit;
    unsigned int emission_count;
    unsigned int active_particle_count;
    bool release_when_done;
    bool replace_when_full_191;
    bool persisted;
    unsigned char padding_193;
    /* Per-particle liveness flag byte; the update loop retires it when the
       birth tick plus lifetime expires. */
    bool* particle_active;
    /* Per-particle velocity; acceleration_1f4 integrates it each update. */
    srVector3T<float>* velocities;
    /* Unsigned millisecond birth ticks, one per particle. */
    unsigned int* birth_ticks;
    unsigned char emitting;
    bool traversal_enabled;
    unsigned char padding_1a2[2];
    W8ParticleBoundsMode bounds_mode;
    int has_acceleration;
    W8ParticleExpiryMode expiry_mode;
    W8ParticleEmissionMode emission_mode;
    int los_check_enabled;
    W8ParticleDirectionMode direction_mode;
    W8ParticleSpeedMode speed_mode;
    W8ParticleFlutterMode flutter_mode;
    int camera_relative;
    /* Emission interval; elapsed comparisons use unsigned subtraction. */
    unsigned int emission_interval;
    /* Lifetime added to each absolute unsigned birth tick. */
    unsigned int lifetime_ms;
    srVector3T<float> minimum_1d0;
    srVector3T<float> maximum_1dc;
    srVector3T<float> direction_1e8;
    srVector3T<float> acceleration_1f4;
    float flutter_amplitude;
    /* 0x00498DD0 uses this as an unsigned modulus period. */
    unsigned int flutter_period;
    float cone_yaw;
    float cone_pitch;
    float initial_speed;
    float speed_min;
    float speed_max;
    srVector3T<float> minimum_21c;
    srVector3T<float> maximum_228;
    srVector3T<float> bounds_origin;
    float bounds_radius;
    /* Added to the camera position when camera_relative selects camera-relative
       placement. */
    srVector3T<float> camera_offset;
    unsigned int update_flags;
    /* Index pairs, two per still-active particle, rebuilt whenever
       update_flags carries bit 1. */
    unsigned long* active_triangles;
    /* Last accepted particle-integration tick. */
    unsigned int last_integration_tick;
    /* Emission schedule tick. */
    unsigned int last_emission_tick;
    short attachment_key;
    unsigned char padding_262[2];
    int start_frame;
    int end_frame;
    W8MonsterShakeCallback* callback;
    unsigned int emission_gap;
    unsigned int last_emitted_at;
    float size_scale;
    unsigned char padding_27c[4];
};

static_assert(sizeof(stParticle) == 0x280, "stParticle_size_must_be_0x280");

stParticle* FindRegisteredParticle(const char* name);
void SaveParticleStates(unsigned int handle);
void LoadParticleStates(int handle);
