#ifndef WIZ8_ENGINE_CODE_MONSTER_H
#define WIZ8_ENGINE_CODE_MONSTER_H

#include "surrender/srMath.h"
#include "surrender/srHeap.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/geometry.h"
#include "wiz8/grcycle.h"
#include "wiz8/local_code/MonsterGroup.h"

struct W8AnimObj;
struct W8PList;

typedef struct W8MonsterRep W8MonsterRep;

enum { W8_MONSTER_CYCLE_COUNT = 27 };

/* Sixteen bytes the cycle runtime record carries at 0x04c, written as one block
   by the setter at 0x004C5AD0. That setter takes the block by value and VC6
   copies it with the interleaved two-register rotation it uses for a struct
   assignment, rather than the sequential load/store pairs four separate scalar
   parameters would emit - which is what makes this one object and not four.
   Nothing observed so far types its contents. */
typedef W8AnimRepValue4 W8MonsterRuntimeBlock4C;

class W8VectorElement005EC018 : public srClass {
public:
    virtual ~W8VectorElement005EC018() override;
};

class W8MonsterReleasable005C8 {
public:
    virtual ~W8MonsterReleasable005C8();
};

/* MonsterRep owns three ordinary arrays of growable vectors.  The first is
   proven as AnimObj* by its consumers; the second element identity remains
   unknown; the third owns vectors of light nodes. */
typedef W8GrowableVector<W8AnimObj*> W8MonsterAnimationVector;
typedef W8GrowableVector<float> W8MonsterAnimationScaleVector;
typedef W8LightVector W8MonsterLightVector;
typedef W8GrowableVector<W8MonsterLightVector*> W8MonsterLightVectorList;

/* W8MonsterRep's constructor calls the 0xac-byte W8EmitterHost constructor at
   offset zero, constructs its three cycle arrays at +0xac/+0x25c/+0x40c,
   and only then installs the six-slot 0x005ED200 vtable. The inherited five
   slots are followed by one Monster-specific extension. */
struct W8MonsterRep : public W8EmitterHost {
    W8MonsterRep();
    W8MonsterRep(const W8MonsterRep& other);
    virtual ~W8MonsterRep() override;
    virtual W8AnimRepBase005EC1D8* Clone() override; /* 0x004CA9E0 */
    virtual void SetCycleFrameLod(
        signed char cycle, int frame, int lod) override; /* 0x004BF8C0 */
    virtual void ApplyEmitterSetting(char cycle) override; /* 0x004BF970 */
    virtual void StopEmitter(char cycle) override; /* 0x004BF920 */
    virtual void Method004BF0F0(
        signed char cycle,
        const W8MonsterRep* other,
        signed char other_cycle);

    W8MonsterAnimationVector animations[W8_MONSTER_CYCLE_COUNT]; /* 0x0ac */
    W8MonsterAnimationScaleVector animation_scales[W8_MONSTER_CYCLE_COUNT]; /* 0x25c */
    W8MonsterLightVectorList light_lists[W8_MONSTER_CYCLE_COUNT];/* 0x40c */
    unsigned char flag_5bc;                    /* 0x5bc */
    unsigned char unknown_5bd[3];
    char* name_5c0;                            /* 0x5c0: owned copy */
    int value_5c4;
    W8MonsterReleasable005C8* objects_5c8[8]; /* 0x5c8 */
    W8PList* linked_objects_5e8;               /* 0x5e8 */
    int value_5ec;
    float scale_5f0;
    float minimum_scale_5f4;
    float maximum_scale_5f8;
    float value_5fc;
    unsigned char flag_600;
    unsigned char flag_601;
    unsigned char unknown_602[2];
    float value_604;
    int value_608;
    int value_60c;
    int value_610;
    W8GrowableVector<W8VectorElement005EC018*> linked_runtime_objects_614;
    class MonsterLight* monster_light_624;

    unsigned char GetNumSubsPerCycle(signed char bCycle);
    /* 0x004C4660. A method, not the free function an earlier reading assumed:
       it takes its receiver in ECX and IsDying calls it without reloading ECX
       at all, relying on `this` already being there. The query selector is
       bounded at nine by the body's own `ja` against the jump table. */
};

/* The constructor at 0x004BEA20 initialises through 0x624 and its sole caller
   allocates this much, so the extent is proven even though most of it is not.
   Asserting it here is what stops a field edit from silently shortening the
   object. */
static_assert(sizeof(W8MonsterRep) == 0x628, "W8MonsterRep_size_must_be_0x628");

static_assert(sizeof(W8MonsterAnimationVector) == 0x10, "W8MonsterAnimationVector_size_must_be_0x10");

/* Five twelve-byte records are allocated from SurRender's heap by the
   embedded vector at +0x29c. The record contents are not yet identified. */
struct W8MonsterVectorElement005ECDC8 {
    unsigned int values[3];
};

class W8MonsterVector005ECDC8 {
public:
    W8MonsterVector005ECDC8()
        : count(0), capacity(0), data(static_cast<W8MonsterVectorElement005ECDC8*>(
              srHeap.allocate(5 * sizeof(W8MonsterVectorElement005ECDC8))))
    {
        if (data != 0) {
            capacity = 5;
        }
    }

    virtual ~W8MonsterVector005ECDC8()
    {
        srHeap.free(data);
    }

    int count;
    int capacity;
    W8MonsterVectorElement005ECDC8* data;
};

struct W8MonsterState28C {
    unsigned char flag_00;
    unsigned char flag_01;
    signed char value_02;
    unsigned char flag_03;
    unsigned char flag_04;
    unsigned char flag_05;
    unsigned char unknown_06[0x0a];
};

struct W8MonsterState2AC {
    unsigned char flag_00;
    unsigned char unknown_01[3];
    int value_04;
    int value_08;
    int value_0c;
    unsigned char unknown_10[0x0c];
    int value_1c;
    int value_20;
    int value_24;
    unsigned char flag_28;
    unsigned char unknown_29[3];
};

struct W8MonsterState2FC {
    float scale_00;
    float scale_04;
    unsigned char unknown_08[8];
};

struct W8MonsterFlags330 {
    unsigned char flag_00;
    unsigned char flag_01;
    unsigned char copied_flag_02;
    unsigned char unknown_03;
};

/* The GrCycle factory allocates 0x348 bytes and calls the constructor at
   0x004BFB00 for object type zero. Both constructors and the destructor install
   primary vtable 0x005ED22C and the W8GrCycle secondary table at +0x18. */
class W8Monster : public W8GrCycle {
public:
    W8Monster();
    W8Monster(const W8Monster& rhs);
    virtual ~W8Monster() override;

    virtual unsigned char CanEnterCycle(signed char cycle) override;
    virtual void vslot4() override;
    virtual signed char GetNumSubCycles() override;
    virtual unsigned char IsCycleSupported(signed char cycle) override;
    virtual signed char GetTotalAnimationCount() override;
    virtual float GetCurrentAnimationScale() override;
    virtual W8EmitterHost* GetRepresentation() override;
    virtual unsigned char GetAnimationBounds(
        W8Position* minimum, W8Position* maximum) override;
    virtual unsigned char GetAnimationRadius(float* radius) override;
    virtual void vslot12() override;
    virtual W8AnimObj* GetCurrentAnimation() override;
    virtual void AdvanceAnimationFrame(int value, int flags) override;
    virtual W8AniMesh* GetCurrentAniMesh() override;
    virtual void vslot16();
    virtual void SetCurrentAnimationScale(float scale);
    virtual void vslot18();
    virtual unsigned char GetAnimationCenter(W8Position* center);
    virtual void SetPosition(const W8Position* position) override;

    int Query(int query);                              /* 0x004C4660 */
    void SetRuntimeValueA6(unsigned char value);       /* 0x004C6C00 */
    unsigned char IsDying();                           /* 0x004CA4C0 */
    unsigned char Function4C2CF0(signed char cycle);
    void Function4C50F0();
    int Function4C6A50();
    void Function4C6990(int value);
    void HandleAnimationThreshold004C75C0();
    void HandleAnimationFrame004C74D0(unsigned char frame);
    void UpdateShakeEvents004C3380(unsigned char frame);

public:
    W8MonsterRep* m_pRep;
    unsigned int flags_1dc;
    int value_1e0;
    int propagated_value_1e4;
    float value_1e8;
    float value_1ec;
    float value_1f0;
    int value_1f4;
    int value_1f8;
    unsigned char flag_1fc;
    unsigned char flag_1fd;
    unsigned char unknown_1fe[2];
    int value_200;
    int value_204;
    int value_208;
    int value_20c;
    int value_210;
    unsigned char unknown_214;
    unsigned char flag_215;
    unsigned char flag_216;
    unsigned char flag_217;
    unsigned char flag_218;
    unsigned char unknown_219[3];
    int value_21c;
    int value_220;
    int value_224;
    int value_228;
    unsigned char flag_22c;
    unsigned char flag_22d;
    unsigned char state_22e;
    unsigned char unknown_22f;
    unsigned char unknown_230[8];
    srClass* object_238;
    int value_23c;
    int value_240;
    W8GrowableVector<unsigned char> bytes_244;
    W8Timer005EC0A4 timer_254;
    int value_278;
    int registry_weight_27c;
    W8MonsterFormation formation;
    W8MonsterState28C state_28c;
    W8MonsterVector005ECDC8 vector_29c;
    W8MonsterState2AC state_2ac;
    W8Timer005EC0A4 timer_2d8;
    W8MonsterState2FC state_2fc;
    W8Timer005EC0A4 timer_30c;
    W8MonsterFlags330 flags_330;
    srClass* object_334;
    W8GrowableVector<int> values_338;
};

static_assert(
    sizeof(W8Monster) == 0x348,
    "W8Monster_size_must_be_0x348");
static_assert(sizeof(W8MonsterState28C) == 0x10, "W8MonsterState28C_size_must_be_0x10");
static_assert(sizeof(W8MonsterState2AC) == 0x2c, "W8MonsterState2AC_size_must_be_0x2c");
static_assert(sizeof(W8MonsterState2FC) == 0x10, "W8MonsterState2FC_size_must_be_0x10");
static_assert(sizeof(W8MonsterFlags330) == 4, "W8MonsterFlags330_size_must_be_4");

#endif
