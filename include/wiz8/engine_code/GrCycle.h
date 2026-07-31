#pragma once

/* Engine Code\GrCycle.cpp owns these declarations, split out of the combined
   grcycle.h. The two bases keep their original relative order: W8GrObject is
   W8GrCycle's first base and W8Navigator its second. */

#include "surrender/srMath.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/GrObject.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/geometry.h"
#include "wiz8/vector.h"
#include "wiz8/vector_005ec294.h"

class srModelInstance;
struct W8World;

class W8VectorElement005ECED4;
class stLight;
class stParticle;
class stGroundShadow;
struct W8AnimObj;
struct W8AniMesh;

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
    unsigned char unknown_18[0x24];
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
    void AddVectorElement005ECED4(W8VectorElement005ECED4* element);
    void CreateGroundShadow(int value_140, int value_13c);
    void SetGroundShadowVisible(char visible);
    void ResetRepresentation004A7420();
    void Function4A7BE0(const float* position);
    void UpdateLights004A7150();
    srModelInstance* SelectCycleFrameLod004A8360(
        signed char cycle, signed char frame, signed char lod);
    srModelInstance* GetCurrentModelInstance004A8250();
    unsigned char ReplacePath004A8400(void* path);
    void SubmitTargetValue004A84A0();

public:
    srModelInstance* current_model_instance_1a8;
    W8LightVector* m_plsLights; /* 0x1ac */
    W8GrowableVector<W8VectorElement005ECED4*>* m_plsShakeEvents; /* 0x1b0 */
    unsigned char m_fDeleteLights;        /* 0x1b4: named by GrCycle.cpp:1656 */
    unsigned char unknown_1b5;
    unsigned char unknown_1b6[2];
    W8GrowableVector<W8GrCycleShakeEvent*>* m_plsParticles; /* 0x1b8 */
    unsigned char unknown_1bc;
    unsigned char enabled_1bd;
    unsigned char unknown_1be;
    unsigned char unknown_1bf;
    unsigned char unknown_1c0[0xc];
    float scale_1cc;
    stGroundShadow* m_ground_shadow;       /* 0x1d0: typed runtime class stGroundShadow */
    float unknown_1d4;
};                                      /* 0x1d8 */

static_assert(sizeof(W8GrCycle) == 0x1d8, "W8GrCycle_size_must_be_0x1d8");

unsigned char __fastcall IsSoleGrCycleForName(W8GrCycle* cycle);
unsigned char UnregisterGrCycle(W8GrCycle* cycle);
const char* __fastcall GetGrCycleName(W8GrCycle* cycle);
