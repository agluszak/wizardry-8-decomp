#pragma once

#include "surrender/srModelInstance.h"

class srMaterial;
class srTextureIFace;
class stTextureAnim;

/* Engine Code\stModelInstance.cpp. The 0x005EC814/0x005EC804 tables are the
   construction-phase tables of the canonical srClassSupport instantiation,
   not a separate authored 0x190-byte base. */
// VTABLE: WIZ8 0x005ec7d0 stModelInstance
// VTABLE: WIZ8 0x005ec7c0 srModel::Client
class stModelInstance
    : public srClassSupport<stModelInstance, srModelInstance, false, 0x10004> {
public:
    static const char* sGetClassName() { return "stModelInstance"; }

    explicit stModelInstance(srNode* parent);          /* 0x0047EC80 */
    stModelInstance& operator=(const stModelInstance& other); /* 0x0047EDF0 */
    stTextureAnim* FindMouthTexture00481080();     /* 0x00481080 */
    int AddDamageStage00480560(const char* name);
    int AddExistingDamageStage00480670(const char* name);
    int FindDamageStage00480790(const char* name);
    unsigned char ReplaceDamageStageTexture004807B0(
        int stage, const char* old_name, srTextureIFace* replacement);
    srClass* vInstance() override;                     /* 0x00481DD0 */
    void traverse(TraverseInfo& info) override;        /* 0x004803F0 */
    void process(const ProcessInfo& info, e_processType type) override; /* 0x0047F560 */

    void Render0047F930(srGERD* renderer);             /* 0x0047F930 */

    unsigned char displayState() const { return state_170; }
    void setRenderDepth(unsigned long depth) { render_depth_164 = depth; }

    virtual ~stModelInstance() override;               /* 0x0047EF70 */

public:
    unsigned long state_160;
    union {
        unsigned long render_depth_164;
        float emissive_x_164;
    };
    union {
        unsigned long state_168;
        float emissive_y_168;
        struct {
            short left_168;
            short top_16a;
        };
    };
    union {
        unsigned long state_16c;
        float emissive_z_16c;
        struct {
            short right_16c;
            short bottom_16e;
        };
    };
    union {
        unsigned long state_170_173;
        float emissive_w_170;
        struct {
            unsigned char state_170;
            unsigned char state_171;
            unsigned char padding_172[2];
        };
    };
    srMaterial* material_174;
    unsigned long state_178;
    unsigned long state_17c;
    unsigned int frame_index_180;
    int damage_stage_184;
    srHeapArray<int> damage_stage_tables_188;
    int value_190;
    srVector3T<float> scale_194;
    unsigned char flag_1a0;
    unsigned char flag_1a1;
    unsigned char unknown_1a2[2];
    /* stParticle's constructor stores the integer 2 here while GrCycle's
       0x004A7470 stores a float; the storage carries both views, so name
       both rather than pick one. */
    union {
        int value_1a4;
        float scale_1a4;
    };
    int value_1a8;
    float value_1ac;
};

static_assert(sizeof(stModelInstance) == 0x1b0,
              "stModelInstance_size_must_be_0x1b0");

/* Concrete 2D model instance. Slot 5 and the secondary slot-0 adjustor are
   SYNTHETIC compiler-generated deleting destructors; no source body owns
   either address. */
// VTABLE: WIZ8 0x005ec858 srClassSupport<srModelInstance, class srNode, 0, 4352>
// VTABLE: WIZ8 0x005ec848 srModel::Client
class stModelInstance2D
    : public srClassSupport<stModelInstance2D, srModelInstance, false, 0x10005> {
public:
    static const char* sGetClassName() { return "stModelInstance2D"; }

    // FUNCTION: WIZ8 0x0047F0F0
    explicit stModelInstance2D(srNode* parent)
        : srClassSupport<stModelInstance2D, srModelInstance, false, 0x10005>(
              static_cast<srNode*>(0))
    {
        state_170 = 0;
        left_168 = 0;
        top_16a = 0;
        right_16c = 0;
        bottom_16e = 0;
        state_160 = 0;
        state_171 = 0;
        render_depth_164 = 2000;
        vector_174 = 0;
        vector_178 = 0;
        m_pGlowMaterial_17c = 0;
        if (parent != 0) {
            setParent(parent, 1);
        }
    }

    stModelInstance2D& operator=(const stModelInstance2D& other); /* 0x0047F290 */
    void SetModel0047F3A0(srModel* model);              /* 0x0047F3A0 */
    void SetGlowEnabled00480EB0(unsigned char enabled); /* 0x00480EB0 */
    unsigned short GetScaledWidth00480EF0() const;      /* 0x00480EF0 */
    unsigned short GetScaledHeight00480F70() const;     /* 0x00480F70 */
    void SetGlowColors00480FF0(
        const srVector4T<float>* first,
        const srVector4T<float>* second);               /* 0x00480FF0 */

    srClass* vInstance() override;                 /* 0x00481E30 */
    void process(const ProcessInfo& info, e_processType type) override; /* 0x00480920 */

    unsigned char displayState() const { return state_170; }
    void configure2D(short width, short height)
    {
        state_160 = 0;
        render_depth_164 = 2000;
        left_168 = width;
        top_16a = height;
        right_16c = 0;
        bottom_16e = 0;
        state_170 = 0;
        state_171 = 0;
        vector_174 = 0;
        vector_178 = 0;
        m_pGlowMaterial_17c = 0;
    }
    void setRenderDepth(unsigned long depth) { render_depth_164 = depth; }

    unsigned long state_160;
    unsigned long render_depth_164;
    union {
        unsigned long state_168;
        struct {
            short left_168;
            short top_16a;
        };
    };
    union {
        unsigned long state_16c;
        struct {
            short right_16c;
            short bottom_16e;
        };
    };
    union {
        unsigned long state_170_173;
        struct {
            unsigned char state_170;
            unsigned char state_171;
            unsigned char padding_172[2];
        };
    };
    srVector4T<float>* vector_174;
    srVector4T<float>* vector_178;
    srMaterial* m_pGlowMaterial_17c;
    virtual ~stModelInstance2D() override;         /* 0x0047F410 */
};

static_assert(sizeof(stModelInstance2D) == 0x180,
              "stModelInstance2D_must_be_0x180");
