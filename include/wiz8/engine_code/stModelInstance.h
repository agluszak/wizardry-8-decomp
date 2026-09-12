#pragma once

#include "surrender/srArray.h"
#include "surrender/srModelInstance.h"
#include "wiz8/engine_code/AnimRep.hpp"

#include <stddef.h>

class srMaterial;
class srTextureIFace;
class stTextureAnim;

/* Engine Code\stModelInstance.cpp. */
// VTABLE: WIZ8 0x005ec7d0 stModelInstance
// VTABLE: WIZ8 0x005ec7c0 stModelInstance::srModel::Client
/* The following are the construction-phase tables for the support base. */
// VTABLE: WIZ8 0x005ec814 srClassSupport<stModelInstance,srModelInstance,0,65540>
// VTABLE: WIZ8 0x005ec804 srClassSupport<stModelInstance,srModelInstance,0,65540>::srModel::Client
class stModelInstance : public srClassSupport<stModelInstance, srModelInstance, false, 0x10004> {
public:
    static const char* sGetClassName()
    {
        return "stModelInstance";
    }

    explicit stModelInstance(srNode* parent);                 /* 0x0047EC80 */
    stModelInstance& operator=(const stModelInstance& other); /* 0x0047EDF0 */

    stTextureAnim* FindMouthTexture00481080(); /* 0x00481080 */
    int AddDamageStage00480560(const char* name);
    int AddExistingDamageStage00480670(const char* name);
    int FindDamageStage00480790(const char* name);
    unsigned char ReplaceDamageStageTexture004807B0(int stage, const char* old_name,
                                                    srTextureIFace* replacement);
    unsigned char displayState() const
    {
        return render_state_164.display_state;
    }
    void setRenderDepth(unsigned long depth)
    {
        render_state_164.render_depth = depth;
    }

    virtual ~stModelInstance() override; /* 0x0047EF70 */

public:
    unsigned long state_160;
    W8ModelInstanceRenderState render_state_164;
    srClass* retained_174;
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

static_assert(offsetof(stModelInstance, render_state_164) == 0x164,
              "stModelInstance_render_state_offset");
static_assert(sizeof(stModelInstance) == 0x1b0, "stModelInstance_size_must_be_0x1b0");

/* Concrete 2D model instance. Slot 5 and the secondary slot-0 adjustor are
   SYNTHETIC compiler-generated deleting destructors; no source body owns
   either address. */
// VTABLE: WIZ8 0x005ec858 srClassSupport<srModelInstance, class srNode, 0, 4352>
// VTABLE: WIZ8 0x005ec848 srModel::Client
class stModelInstance2D
    : public srClassSupport<stModelInstance2D, srModelInstance, false, 0x10005> {
public:
    static const char* sGetClassName()
    {
        return "stModelInstance2D";
    }

    // FUNCTION: WIZ8 0x0047F0F0
    explicit stModelInstance2D(srNode* parent)
        : srClassSupport<stModelInstance2D, srModelInstance, false, 0x10005>(
              static_cast<srNode*>(0))
    {
        render_state_164.display_state = 0;
        render_state_164.left = 0;
        render_state_164.top = 0;
        render_state_164.right = 0;
        render_state_164.bottom = 0;
        state_160 = 0;
        render_state_164.state_0d = 0;
        render_state_164.render_depth = 2000;
        vector_174 = 0;
        vector_178 = 0;
        m_pGlowMaterial_17c = 0;
        if (parent != 0) {
            setParent(parent, 1);
        }
    }

    stModelInstance2D& operator=(const stModelInstance2D& other); /* 0x0047F290 */
    void SetModel0047F3A0(srModel* model);                        /* 0x0047F3A0 */

    srClass* vInstance() override;                                      /* 0x00481E30 */
    void process(const ProcessInfo& info, e_processType type) override; /* 0x00480920 */
    int GetWidth00480EF0();                                             /* 0x00480EF0 */
    int GetHeight00480F70();                                            /* 0x00480F70 */

    unsigned char displayState() const
    {
        return render_state_164.display_state;
    }
    void configure2D(short width, short height)
    {
        state_160 = 0;
        render_state_164.render_depth = 2000;
        render_state_164.left = width;
        render_state_164.top = height;
        render_state_164.right = 0;
        render_state_164.bottom = 0;
        render_state_164.display_state = 0;
        render_state_164.state_0d = 0;
        vector_174 = 0;
        vector_178 = 0;
        m_pGlowMaterial_17c = 0;
    }
    void setRenderDepth(unsigned long depth)
    {
        render_state_164.render_depth = depth;
    }

    unsigned long state_160;
    W8ModelInstanceRenderState render_state_164;
    srVector4T<float>* vector_174;
    srVector4T<float>* vector_178;
    srMaterial* m_pGlowMaterial_17c;
    virtual ~stModelInstance2D() override; /* 0x0047F410 */
};

static_assert(offsetof(stModelInstance2D, render_state_164) == 0x164,
              "stModelInstance2D_render_state_offset");
static_assert(sizeof(stModelInstance2D) == 0x180, "stModelInstance2D_must_be_0x180");
