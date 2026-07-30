#pragma once

#include "surrender/srMath.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/geometry.h"
#include "wiz8/vector.h"
#include "wiz8/vector_005ec294.h"

class srNode;
struct W8ItemRep;
struct W8PathAI;

struct W8NavigatorAttachment {
    unsigned char unknown_00[0x10];
    srVector3T<float> position_10;
    unsigned char unknown_1c[0x18];
    srVector3T<float> position_34;
    unsigned char unknown_40[0x0c];
    srVector3T<float>* position_4c;
};

/* GrObject.cpp calls these sound events `pse`/`m_plsSoundEvents`; no stronger
   source witness for the concrete event class name is available yet. */
class W8VectorElement005ED094 {
public:
    ~W8VectorElement005ED094();          /* 0x004D5770 */

    unsigned char unknown_000[0x28];
    int value_028;
};

/* GrObject.cpp owns this base.  The original Item.cpp assertion
   `pMissile->GrObject::GetAI()` independently establishes the class name. */
class W8GrObject {
public:
    W8GrObject();                         /* 0x004B6900 */
    W8GrObject(const W8GrObject& other); /* 0x004B69A0 */
    virtual ~W8GrObject();                /* 0x004B6B60 */

    unsigned char AddSoundEvent(W8VectorElement005ED094* event);

public:
    unsigned char unknown_004;           /* 0x04 */
    unsigned char unknown_005[3];
    int unknown_008;                     /* 0x08 */
    void* m_pAI;                         /* 0x0c: GrObject::GetAI() assertion */
    W8GrowableVector<W8VectorElement005ED094*>* m_plsSoundEvents; /* 0x10 */
    W8ItemRep* m_pRep;                   /* 0x14: typed by Engine Code\Item.cpp */
};                                      /* 0x18 */

/* Navigator.cpp owns the path, position, orientation, and scene-node state
   below. It is GrCycle's ordinary second base, not a representation object. */
class W8Navigator {
public:
    W8Navigator();                        /* 0x00451EC0 */
    W8Navigator(const W8Navigator& other); /* 0x00452220 */
    virtual ~W8Navigator();               /* 0x00452120 */
    virtual void SetPathAI(W8PathAI* path_ai);
    virtual W8PathAI* GetPathAI();
    virtual unsigned char secondary_vslot3(int) const { return 1; }
    virtual void SetPosition(const W8Position* position); /* 0x00456020 */

    void configureStartupRange(float range);
    void configureStartupDepth(float near_depth, float far_depth);

    srVector3T<float> GetPosition();
    void SetValue120(float value);                         /* 0x00453C50 */
    float GetValue120();                                  /* 0x00453C60 */
    unsigned char Function452630(const W8Position* position); /* 0x00452630 */
    void Function453690(void* argument);                   /* 0x00453690 */
    void SetPositionInternal00453590(const W8Position* position);
    void SetObject68Flag38(char value);                    /* 0x004537C0 */
    void Function454040(const W8Position* position);       /* 0x00454040 */
    void Function453F30(const W8Position* position);       /* 0x00453F30 */
    void SetFlag25(char value);                            /* 0x004531F0 */

public:
    /* The constructor clears this payload as 98 dwords, while Monster.cpp
       reaches the same secondary base through `lea ecx,[monster+0x18]`.
       The union preserves the constructor's observed dword view and exposes
       only independently witnessed fields. */
    union {
        unsigned int unknown_004[98];
        struct {
            unsigned char unknown_004_to_00c[8];
            unsigned int flags_00c;
            unsigned char unknown_010_to_024[0x14];
            unsigned char flag_024;
            unsigned char flag_025;
            unsigned char unknown_026_to_05c[0x36];
            int value_05c;
            unsigned char unknown_060_to_068[8];
            W8PathAI* path_ai_068;
            unsigned char unknown_06c_to_080[0x14];
            signed char animation_index_080;
            unsigned char unknown_081[3];
            float radius_084;
            unsigned char state_088;
            unsigned char unknown_089[3];
            signed char current_cycle_08c;
            signed char current_subcycle_08d;
            unsigned char unknown_08e_to_09c[0x0e];
            unsigned char position_dirty_09c;
            unsigned char unknown_09d_to_0c4[0x27];
            unsigned short location_id_0c4;
            unsigned char unknown_0c6[2];
            int value_0c8;
            int value_0cc;
            int value_0d0;
            float angle_0d4;
            float angle_0d8;
            unsigned char unknown_0dc_to_0f4[0x18];
            srVector3T<float> position_0f4;
            srVector3T<float> position_100;
            srVector3T<float> position_10c;
            unsigned char unknown_118_to_120[8];
            float value_120;
            unsigned char unknown_124_to_16c[0x48];
            W8NavigatorAttachment* attachment_16c;
            unsigned char unknown_170_to_18c[0x1c];
        } fields;
    };
    srNode* node_18c;                    /* 0x18c: constructed srNode */
};                                      /* 0x190 */

class W8VectorElement005ECED4;
class stLight;
class stParticle;
class stGroundShadow;
struct W8AnimObj;
struct W8AniMesh;

class W8GrCycleShakeEvent {
public:
    int cycle_00;
    signed char subcycle_04;
    unsigned char unknown_05[3];
    stParticle* particle_08;
};

class W8GrCycle :
    public W8GrObject,
    public W8Navigator {
public:
    W8GrCycle();
    W8GrCycle(const W8GrCycle& other);
    virtual ~W8GrCycle() override;
    // FUNCTION: WIZ8 0x004a7140
    virtual unsigned char CanEnterCycle(signed char) { return 1; }
    virtual void vslot2();
    virtual void vslot3();
    virtual void vslot4();
    virtual signed char GetNumSubCycles() = 0;
    virtual unsigned char IsCycleSupported(signed char cycle) = 0;
    virtual signed char GetTotalAnimationCount() = 0;
    virtual float GetCurrentAnimationScale() = 0;
    virtual W8EmitterHost* GetRepresentation() = 0;
    virtual unsigned char GetAnimationBounds(
        W8Position* minimum, W8Position* maximum);
    virtual unsigned char GetAnimationRadius(float* radius);
    virtual void vslot12() = 0;
    virtual W8AnimObj* GetCurrentAnimation() = 0;
    virtual void AdvanceAnimationFrame(int value, int flags);
    virtual W8AniMesh* GetCurrentAniMesh() = 0;

    void SetSubCycle(unsigned char subcycle);
    void SetBehaviour(signed char bBehaviour);
    void SetLights(W8LightVector* lights);
    void AddVectorElement005ECED4(W8VectorElement005ECED4* element);
    void CreateGroundShadow(int value_140, int value_13c);
    void SetGroundShadowVisible(char visible);
    void ResetRepresentation004A7420();
    void SelectCycleFrameLod004A8360(signed char cycle, signed char frame, signed char lod);
    unsigned char ReplacePath004A8400(void* path);
    void SubmitTargetValue004A84A0();

public:
    int unknown_1a8;
    W8LightVector* m_plsLights; /* 0x1ac */
    W8GrowableVector<W8VectorElement005ECED4*>* m_vector_1b0; /* 0x1b0 */
    unsigned char m_fDeleteLights;        /* 0x1b4: named by GrCycle.cpp:1656 */
    unsigned char unknown_1b5;
    unsigned char unknown_1b6[2];
    W8GrowableVector<W8GrCycleShakeEvent*>* m_shake_events; /* 0x1b8 */
    unsigned char unknown_1bc;
    unsigned char enabled_1bd;
    unsigned char unknown_1be;
    unsigned char unknown_1bf;
    unsigned char unknown_1c0[0xc];
    float scale_1cc;
    stGroundShadow* m_ground_shadow;       /* 0x1d0: typed runtime class stGroundShadow */
    int unknown_1d4;
};                                      /* 0x1d8 */

// SYNTHETIC: WIZ8 0x004a7140 folded
// W8Navigator::secondary_vslot3

static_assert(sizeof(W8GrCycle) == 0x1d8, "W8GrCycle_size_must_be_0x1d8");

unsigned char __fastcall IsSoleGrCycleForName(W8GrCycle* cycle);
unsigned char UnregisterGrCycle(W8GrCycle* cycle);
