#pragma once

#include "surrender/srModelInstance.h"

class srMaterial;
class srTextureIFace;
class stTextureAnim;

srRegistry::ClassNode* GetSrModelInstanceClassNode00481D00();

/* Engine Code\stModelInstance.cpp. */
// VTABLE: WIZ8 0x005ec814 stModelInstance
// VTABLE: WIZ8 0x005ec804 srModel::Client
class stModelInstance : public srModelInstance {
public:
    stModelInstance()
        : srModelInstance(0)
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x10004);

        if (node == 0) {
            node = registry->registerClass(
                "stModelInstance",
                GetSrModelInstanceClassNode00481D00(),
                0x10004,
                0);
        }
        registry->registerInstance(node, this);
        damage_stage_tables_188 = 0;
        damage_stage_count_18c = 0;
    }

    const char* getClassName() const override;     /* 0x00481870 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x00481880 */
    unsigned long getClassID() const override;     /* 0x00481860 */
    stTextureAnim* FindMouthTexture00481080();     /* 0x00481080 */
    int AddDamageStage00480560(const char* name);
    int AddExistingDamageStage00480670(const char* name);
    int FindDamageStage00480790(const char* name);
    unsigned char ReplaceDamageStageTexture004807B0(
        int stage, const char* old_name, srTextureIFace* replacement);
    unsigned char displayState() const { return state_170; }
    void setRenderDepth(unsigned long depth) { render_depth_164 = depth; }

protected:
    virtual ~stModelInstance() override;           /* 0x00481940 */

public:
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
    srClass* retained_174;
    unsigned long state_178;
    unsigned long state_17c;
    unsigned int frame_index_180;
    int damage_stage_184;
    int* damage_stage_tables_188;
    int damage_stage_count_18c;
};

static_assert(sizeof(stModelInstance) == 0x190,
              "stModelInstance_size_must_be_0x190");

/* The constructor at 0x0047EC80 first builds the 0x190-byte
   stModelInstance base, then installs vtables 0x005EC7D0/0x005EC7C0 and
   initializes the fields below through +0x1ac. Its original derived-class
   name is not yet available. */
class stModelInstance005EC7D0 : public stModelInstance {
public:
    explicit stModelInstance005EC7D0(srNode* parent); /* 0x0047EC80 */
    stModelInstance005EC7D0& operator=(
        const stModelInstance005EC7D0& other);         /* 0x0047EDF0 */

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
    virtual ~stModelInstance005EC7D0() override; /* 0x0047EF70 */
};

static_assert(sizeof(stModelInstance005EC7D0) == 0x1b0,
              "stModelInstance005EC7D0_size_must_be_0x1b0");

/* The destructor-phase table at 0x005EC89C is the no-storage registry layer
   between imported srModelInstance and the concrete 2D implementation. Its
   methods all present the public runtime identity stModelInstance2D. */
// VTABLE: WIZ8 0x005ec89c stModelInstance2D registry base
// VTABLE: WIZ8 0x005ec88c srModel::Client
class W8ModelInstance2DRegistry005EC89C : public srModelInstance {
public:
    explicit W8ModelInstance2DRegistry005EC89C(srNode* parent)
        : srModelInstance(parent)
    {
        srRegistry* registry = srCore.getRegistry();
        srRegistry::ClassNode* node = registry->getClassNode(0x10005);

        if (node == 0) {
            srRegistry* instance_registry = srCore.getRegistry();

            node = instance_registry->getClassNode(0x1100);
            if (node == 0) {
                srRegistry* node_registry = srCore.getRegistry();

                node = node_registry->getClassNode(0x1000);
                if (node == 0) {
                    node = node_registry->registerClass(
                        srNode::sGetClassName(),
                        srClass::sGetClassNode(),
                        0x1000,
                        1);
                }
                node = instance_registry->registerClass(
                    "srModelInstance", node, 0x1100, 0);
            }
            node = registry->registerClass(
                "stModelInstance2D", node, 0x10005, 0);
        }
        registry->registerInstance(node, this);
    }

    const char* getClassName() const override;     /* 0x00481A50 */
    unsigned long getClassID() const override;     /* 0x00481A40 */
    srRegistry::ClassNode* getClassNode() const override; /* 0x00481A60 */

protected:
    virtual ~W8ModelInstance2DRegistry005EC89C() override; /* 0x00481B20 */
};

static_assert(sizeof(W8ModelInstance2DRegistry005EC89C) == 0x160,
              "W8ModelInstance2DRegistry005EC89C_must_be_0x160");

/* Concrete 2D model instance. Slot 5 and the secondary slot-0 adjustor are
   SYNTHETIC compiler-generated deleting destructors; no source body owns
   either address. */
// VTABLE: WIZ8 0x005ec858 stModelInstance2D
// VTABLE: WIZ8 0x005ec848 srModel::Client
class stModelInstance2D : public W8ModelInstance2DRegistry005EC89C {
public:
    // FUNCTION: WIZ8 0x0047F0F0
    explicit stModelInstance2D(srNode* parent)
        : W8ModelInstance2DRegistry005EC89C(0)
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

    srClass* vInstance() override;                 /* 0x00481E30 */
    srClass* clone() override;                      /* 0x00481B00 */
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
