#pragma once

/* Engine Code\GrCycle.cpp owns these declarations, split out of the combined
   grcycle.h. The two bases keep their original relative order: W8GrObject is
   W8GrCycle's first base and W8Navigator its second. */

#include "surrender/srMath.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/GrObject.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/geometry.h"
#include "wiz8/vector.h"
#include "wiz8/vector_005ec294.h"

class srModelInstance;
struct W8World;

class stLight;
class stParticle;
class stGroundShadow;
struct W8AnimObj;
struct W8AniMesh;

/* Engine Code\GrCycle.cpp's camera-shake effect. 0x004AE080 allocates 0x4c and
   hands it to the constructor at 0x004ADED0.

   One class, two consumers. GrCycle keeps a vector of them in m_plsShakeEvents
   and fires the ones whose cycle/frame/subcycle key matches; Trigger.cpp's shake
   event creates one directly. Trigger clears bit 1 so the effect is not released
   by the live list and deletes it itself - the same bit 0x004AE270 tests before
   deleting. That shared ownership bit, the shared factory and the shared 0x4c
   allocation are what prove the two are one class rather than two of a size.

   flags_00: bit 0 says the effect is in the live list, bit 1 that the live list
   owns it. Bits 2, 3 and 4 are set together by the constructor's own flag, and
   Trigger sets bit 4 on its own to reverse the shake. */
class W8CameraShakeEffect {
public:
    W8CameraShakeEffect(
        float duration, char preset, float intensity, int value_08,
        const srVector3T<float>* position);   /* 0x004ADED0 */
    W8CameraShakeEffect(const W8CameraShakeEffect& other); /* 0x004AE000 */

    unsigned int flags_00;               /* 0x00 */
    float intensity_04;                  /* 0x04 */
    int value_08;                        /* 0x08 */
    srVector3T<float> position_0c;       /* 0x0c */
    W8GameTimer timer_18;            /* 0x18 */
    /* The key 0x004AE170 matches an animation event against. */
    int cycle_3c;                        /* 0x3c */
    int frame_40;                        /* 0x40 */
    int subcycle_44;                     /* 0x44 */
    int value_48;                        /* 0x48 */
};

static_assert(sizeof(W8CameraShakeEffect) == 0x4c,
              "W8CameraShakeEffect_must_be_0x4c");

/* The live list every active effect is on, and the timer the first effect
   creates alongside it. Both are built lazily by the constructor. */
extern W8GrowableVector<W8CameraShakeEffect*>* g_shake_effects_0065be2c;
extern W8GameTimer* g_shake_timer_0065be30;

W8CameraShakeEffect* CreateCameraShakeEffect004AE080(
    float duration, char preset, float intensity, int value_08,
    const srVector3T<float>* position);
/* Fire every effect in one cycle's vector whose key matches, moving it onto the
   live list and restarting its timer. */
void TriggerShakeEffects004AE170(
    W8GrowableVector<W8CameraShakeEffect*>* effects,
    int cycle, unsigned int frame, int subcycle,
    const srVector3T<float>* position);
/* Take every active effect in one cycle's vector back off the live list, and
   release the ones that list owned. */
void StopShakeEffects004AE270(
    W8GrowableVector<W8CameraShakeEffect*>* effects);

/* 0x004A5F20 allocates 0x3c for each of these and copies them field by field:
   a leading dword, the byte after it, an owned stParticle rebuilt through
   0x00498180, a vector at +0x0c, and the 0x24-byte tail wholesale. */
class W8GrCycleShakeEvent {
public:
    int cycle_00;
    signed char subcycle_04;
    unsigned char unknown_05[3];
    stParticle* particle_08;
    srVector3T<float> position_0c;
    /* 0x004A7E50 composes this into the model instance's own rotation with
       method_00421A40, which is what makes it a matrix rather than 0x24
       opaque bytes. */
    srMatrix3T<float> rotation_18;
};

static_assert(sizeof(W8GrCycleShakeEvent) == 0x3c,
              "W8GrCycleShakeEvent_must_be_0x3c");

class W8GrCycle :
    public W8GrObject,
    public W8Navigator {
public:
    W8GrCycle();
    W8GrCycle(const W8GrCycle& other);      /* 0x004A5F20 */
    virtual ~W8GrCycle() override;
    // FUNCTION: WIZ8 0x004a7140
    virtual unsigned char CanEnterCycle(signed char) { return 1; }
    virtual void TickAnimation(float scale);             /* 0x004A6E20 */
    virtual unsigned char ApplyPendingCycle();           /* 0x004A6FC0 */
    virtual void UpdateRepresentation(W8World* world); /* 0x004A7470 */
    virtual signed char GetNumSubCycles() = 0;
    virtual unsigned char IsCycleSupported(signed char cycle) = 0;
    virtual signed char GetTotalAnimationCount() = 0;
    virtual float GetCurrentAnimationScale() = 0;
    virtual W8EmitterHost* GetRepresentation() = 0;
    virtual unsigned char GetAnimationBounds(
        W8Position* minimum, W8Position* maximum);
    virtual unsigned char GetAnimationRadius(float* radius);
    virtual void SetCycle(signed char cycle) = 0;
    virtual W8AnimObj* GetCurrentAnimation() = 0;
    virtual void AdvanceAnimationFrame(int value, int flags);
    virtual W8AniMesh* GetCurrentAniMesh() = 0;

    void SetSubCycle(unsigned char subcycle);
    void SetBehaviour(signed char bBehaviour);
    void SetLights(W8LightVector* lights);
    void AddShakeEffect004A8530(W8CameraShakeEffect* effect);
    void CreateGroundShadow(int value_140, int value_13c);
    void SetGroundShadowVisible(char visible);
    void ResetRepresentation004A7420();
    /* Runs at the end of every representation update; its own body is the
       shake/particle event walk and is not recovered yet. */
    void UpdateParticleAttachments004A7E50();
    void SelectLOD004A7BE0(const float* position);   /* 0x004A7BE0 */
    void UpdateLights004A7150();
    srModelInstance* SelectCycleFrameLod004A8360(
        signed char cycle, signed char frame, signed char lod);
    srModelInstance* GetCurrentModelInstance004A8250();
    unsigned char ReplacePath004A8400(void* path);
    void SubmitTargetValue004A84A0();

public:
    srModelInstance* current_model_instance_1a8;
    W8LightVector* m_plsLights; /* 0x1ac */
    W8GrowableVector<W8CameraShakeEffect*>* m_plsShakeEvents; /* 0x1b0 */
    unsigned char m_fDeleteLights;        /* 0x1b4: named by GrCycle.cpp:1656 */
    unsigned char unknown_1b5;
    unsigned char unknown_1b6[2];
    W8GrowableVector<W8GrCycleShakeEvent*>* m_plsParticles; /* 0x1b8 */
    unsigned char unknown_1bc;
    unsigned char enabled_1bd;
    unsigned char unknown_1be;
    unsigned char unknown_1bf;
    /* The axis 0x004A7E50 aims a mode-three particle along. */
    srVector3T<float> m_axis_1c0;
    float scale_1cc;
    stGroundShadow* m_ground_shadow;       /* 0x1d0: typed runtime class stGroundShadow */
    float unknown_1d4;
};                                      /* 0x1d8 */

static_assert(sizeof(W8GrCycle) == 0x1d8, "W8GrCycle_size_must_be_0x1d8");

unsigned char __fastcall IsSoleGrCycleForName(W8GrCycle* cycle);
unsigned char UnregisterGrCycle(W8GrCycle* cycle);
const char* __fastcall GetGrCycleName(W8GrCycle* cycle);
