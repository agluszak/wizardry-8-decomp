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
struct W8GrCycleReadInfo004A6970;
struct W8PList;
struct W8Item;
class stModelInstance005EC7D0;
class stLight;
class stScript;
class stSound3D;
class Trigger;
class W8Monster;

typedef struct W8MonsterRep W8MonsterRep;

enum { W8_MONSTER_CYCLE_COUNT = 27 };

/* Sixteen bytes the cycle runtime record carries at 0x04c, written as one block
   by the setter at 0x004C5AD0. That setter takes the block by value and VC6
   copies it with the interleaved two-register rotation it uses for a struct
   assignment, rather than the sequential load/store pairs four separate scalar
   parameters would emit - which is what makes this one object and not four.
   Nothing observed so far types its contents. */
typedef W8AnimRepValue4 W8MonsterRuntimeBlock4C;

struct W8MonsterLinkedItem005E8 {
    int unknown_00;
    W8Item* item_04;
};

/* MonsterRep owns three ordinary arrays of growable vectors. The first is
   proven as AnimObj* by its consumers, the second holds animation scales, and
   the third owns vectors of light nodes. */

/* W8MonsterRep's constructor calls the 0xac-byte W8EmitterHost constructor at
   offset zero, constructs its three cycle arrays at +0xac/+0x25c/+0x40c,
   and only then installs the six-slot 0x005ED200 vtable. The inherited five
   slots are followed by one Monster-specific extension. */
struct W8MonsterRep : public W8EmitterHost {
    W8MonsterRep();
    W8MonsterRep(const W8MonsterRep& other);
    virtual ~W8MonsterRep() override;
    virtual W8AnimRepBase005EC1D8* Clone() override; /* 0x004CA9E0 */
    virtual srModelInstance* SetCycleFrameLod(
        signed char cycle, signed char frame, signed char lod) override; /* 0x004BF8C0 */
    virtual unsigned int ApplyEmitterSetting(char cycle) override; /* 0x004BF970 */
    virtual W8AniMesh* GetEmitterAniMesh(char cycle) override; /* 0x004BF920 */
    virtual void Method004BF0F0(
        signed char cycle,
        const W8MonsterRep* other,
        signed char other_cycle);
    unsigned char ReadCycleData004BF520(
        W8GrCycleReadInfo004A6970* info,
        W8Monster* monster,
        int cycle_index,
        int value);

    W8GrowableVector<W8AnimObj*> animations[W8_MONSTER_CYCLE_COUNT]; /* 0x0ac */
    W8GrowableVector<float> animation_scales[W8_MONSTER_CYCLE_COUNT]; /* 0x25c */
    W8GrowableVector<W8GrowableVector<stLight*>*> light_lists[W8_MONSTER_CYCLE_COUNT];/* 0x40c */
    unsigned char flag_5bc;                    /* 0x5bc */
    unsigned char unknown_5bd[3];
    char* name_5c0;                            /* 0x5c0: owned copy */
    int value_5c4;
    W8Item* objects_5c8[8];                   /* 0x5c8 */
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
    W8GrowableVector<stModelInstance005EC7D0*> linked_runtime_objects_614;
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

static_assert(sizeof(W8GrowableVector<W8AnimObj*>) == 0x10,
              "W8Monster_animation_vector_must_be_0x10");

struct W8MonsterState28C {
    unsigned char defining_orders;
    unsigned char orders_finished;
    signed char order_mode;
    unsigned char deaf;
    unsigned char face_party;
    unsigned char stay_home;
    unsigned char unknown_06[2];
    float patrol_distance;
    float patrol_variation;
};

struct W8MonsterState2AC {
    signed char flag_00;
    unsigned char unknown_01[3];
    float direction_x;
    float direction_y;
    float direction_z;
    unsigned char unknown_10[0x0c];
    float look_frequency;
    float look_duration;
    int value_24;
    unsigned char flag_28;
    unsigned char unknown_29[3];
};

struct W8MonsterState2FC {
    float scale_00;
    float scale_04;
    unsigned char unknown_08[4];
    srNode* node_0c;
};

struct W8MonsterFlags330 {
    signed char flag_00;
    unsigned char flag_01;
    unsigned char copied_flag_02;
    unsigned char unknown_03;
};

/* The GrCycle factory allocates 0x348 bytes and calls the constructor at
   0x004BFB00 for object type zero. Both constructors and the destructor install
   primary vtable 0x005ED22C and the W8Navigator secondary-base table at +0x18. */
class W8Monster : public W8GrCycle {
public:
    typedef void (__cdecl *CycleCallback)(W8Monster* monster);

    W8Monster();
    W8Monster(const W8Monster& rhs);
    virtual ~W8Monster() override;

    virtual unsigned char CanEnterCycle(signed char cycle) override;
    virtual void UpdateRepresentation(W8World* world) override;
    virtual signed char GetNumSubCycles() override;
    virtual unsigned char IsCycleSupported(signed char cycle) override;
    virtual signed char GetTotalAnimationCount() override;
    virtual float GetCurrentAnimationScale() override;
    virtual W8EmitterHost* GetRepresentation() override;
    virtual unsigned char GetAnimationBounds(
        srVector3T<float>* minimum, srVector3T<float>* maximum) override;
    virtual unsigned char GetAnimationRadius(float* radius) override;
    virtual void SetCycle(signed char cycle) override;
    virtual W8AnimObj* GetCurrentAnimation() override;
    virtual void AdvanceAnimationFrame(int value, int flags) override;
    virtual W8AniMesh* GetCurrentAniMesh() override;
    virtual void Update();
    virtual void SetCurrentAnimationScale(float scale);
    virtual void GetMappedPosition004C72A0(srVector3T<float>* position);
    virtual unsigned char GetAnimationCenter(srVector3T<float>* center);
    virtual void SetPosition(const srVector3T<float>* position) override;

    int Query(int query);                              /* 0x004C4660 */
    void SetRuntimeValueA6(signed char value);         /* 0x004C6C00 */
    unsigned char IsDying();                           /* 0x004CA4C0 */
    unsigned char IsCycleInterruptable(signed char cycle);
    void ApplyRemovalStateEffects();
    void CollectModelInstances004C6350(
        W8GrowableVector<stModelInstance005EC7D0*>* instances);
    void SetDamageStage004C6990(int stage);
    int GetDamageStageCount004C6A50();
    unsigned char ReplaceSkinTexture004C6700(
        int stage, const char* old_name, const char* new_name);
    int AddDamageStage004C6880(const char* base_name, int stage);
    void RemoveCycleSkinTables004C6B10();
    void RandomizeAppearanceAndMotion004C1D20();
    unsigned char IsRenderable004C7C00(char alternate);
    void InitializeAnimatedTexture004C51D0();
    void HandleAnimationThreshold004C75C0();
    void HandleAnimationFrame004C74D0(unsigned char frame);
    void UpdateShakeEvents004C3380(unsigned char frame);
    void SetShakeEventVisibility004BF9E0(signed char cycle);
    void UpdateAttachedObjects004C3F70();
    void BeginFadeIn004C4F80(float duration);
    void BeginDelayedRemoval004C5000();
    void BeginFadeOutAndRemove004C5040(signed char state);
    void BeginFadeOut004C5150(float duration);
    void StartTalking004C73F0(unsigned char animate_mouth);
    void StopTalking004C7470();
    void SetCycleCallback004CA340(
        int cycle, CycleCallback callback);
    unsigned char GetPatrolPoint004CA360(srVector3T<float>* point);
    void TrackSoundHandle004CA6E0(int handle);
    float GetDistanceToPlayer004C7CB0();
    float GetPointDistanceToPlayer004C7D50(float x, float y, float z);
    float GetDistanceToMonster004C7DD0(W8Monster* monster);
    float GetPointDistanceToMonster004C7E80(
        W8Monster* monster, float x, float y, float z);
    unsigned char SetScript004C7F10(
        const char* script_name, unsigned char reset_orders);
    void ProcessScript004C80E0();
    unsigned char GetProjectilePosition004C77F0(srVector3T<float>* position);
    unsigned char GetSpellPosition004C78E0(srVector3T<float>* position);
    unsigned char GetCycleMappedPosition004C7960(
        signed char cycle, int mapped_index, srVector3T<float>* position);
    unsigned char EvaluateScriptCondition004C9DC0(const char* expression);
    unsigned char CanContinueScript004CA0F0();
    unsigned char SetScriptLabel004CA260(const char* label);
    unsigned char GetFlag216004CA290() const;
    unsigned char IsWithinWorldRange004CA2A0();
    void CheckLineOfSightToPlayer004C4810();
    void GetPlayerSightFlags004C4870(
        unsigned char* primary, unsigned char* secondary);
    unsigned char IsVisibleToPlayer004C4920(unsigned char use_bounds);
    void GetPlayerToMonsterSightFlags004C4A20(
        unsigned char* primary,
        unsigned char* secondary,
        const srVector3T<float>* source);
    unsigned char HasLineOfSightToMonster004C4AF0(W8Monster* monster);
    void GetMonsterSightFlags004C4B70(
        W8Monster* monster,
        unsigned char* primary,
        unsigned char* secondary);
    unsigned char HasLineOfSightFromPoint004C4C40(srVector3T<float> point);
    int IsFacingMonster004C4CA0(W8Monster* monster);
    int IsFacingPlayer004C4D40();
    void Method4C5290();

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
    signed char state_22e;
    unsigned char unknown_22f;
    CycleCallback cycle_callback_230;
    int callback_cycle_234;
    stScript* script_238;
    int script_line_23c;
    int script_wait_240;
    W8GrowableVector<unsigned char> script_conditions_244;
    W8GameTimer timer_254;
    Trigger* trigger_278;
    int registry_weight_27c;
    srVector3T<float> formation;
    W8MonsterState28C state_28c;
    W8GrowableVector<srVector3T<float> > vector_29c;
    W8MonsterState2AC state_2ac;
    W8GameTimer timer_2d8;
    W8MonsterState2FC state_2fc;
    W8GameTimer timer_30c;
    W8MonsterFlags330 flags_330;
    stSound3D* sound_334;
    W8GrowableVector<int> values_338;
};

int ParseMonsterCycleName004C2010(
    const char* name, signed char* subcycle = 0);
unsigned char MonsterReadAllCycles004C0300(
    const W8GrCycleLoadContext* context,
    const char* monster_name,
    W8Monster** monster,
    int load_value,
    int location_id);
unsigned char MonsterReadAllCycles004C58E0(
    const W8GrCycleLoadContext* context,
    const char* monster_name,
    W8Monster** monster,
    int load_value,
    int location_id);
unsigned short ChooseDifferentMonsterDirection004C2E00(
    unsigned short previous_direction);

unsigned char MonsterGetWorldAnimationBounds004CA4F0(
    W8Monster* monster, srVector3T<float>* minimum, srVector3T<float>* maximum);
unsigned char LoadMonsterCycle004C5910(
    const W8GrCycleLoadContext* context,
    const char* mon_name,
    W8Monster** monster,
    int cycle,
    int value);

unsigned char MonsterUsesCurrentModelInstance(W8GrCycle* cycle);
void MonsterGetLocation(
    W8Monster* monster, srVector3T<float>* location);
void MonsterGetLocalLocation(
    W8Monster* monster, srVector3T<float>* location);
void UpdateMonster(W8Monster* monster);
unsigned char MonsterIsCycleSupported(
    W8Monster* monster, signed char cycle);
unsigned char MonsterReplacePath(W8Monster* monster, void* path);
unsigned char MonsterGetAnimationRadius(
    W8Monster* monster, float* radius);
void MonsterSetFacing004C5B60(W8Monster* monster, float angle);
unsigned char MonsterGetCycle17State(W8Monster* monster);
void MonsterSetCycle17State(W8Monster* monster, unsigned char state);
float MonsterGetScale(W8Monster* monster);
void MonsterSetScale(W8Monster* monster, float scale);
void MonsterGetScaleRange(
    W8Monster* monster, float* minimum, float* maximum);
void MonsterSetAdjustedPosition004C5F00(
    W8Monster* monster, const srVector3T<float>* position);
unsigned short MonsterApproachStartupNavigator004C5FF0(
    W8Monster* monster, double separation);
unsigned short MonsterLinkToStartupNavigator004C6030(W8Monster* monster);
unsigned short MonsterConfigureMovementToPlayer004C6070(
    W8Monster* monster,
    int value_1,
    int value_2,
    srVector3T<float> position,
    int value_3,
    int value_4);
unsigned short MonsterConfigureMovementToMonster004C60D0(
    W8Monster* monster,
    W8Monster* target,
    int value_1,
    int value_2,
    srVector3T<float> position,
    int value_3,
    int value_4);
void MonsterAimAtMonster004C62C0(
    W8Monster* monster, W8Monster* target, char alternate);
void MonsterSetCycle(W8Monster* monster, signed char cycle);
void UpdateNearestMonsterGroupMembers004CA570();
extern "C" void Function4C5810(W8Monster* monster);

static_assert(
    sizeof(W8Monster) == 0x348,
    "W8Monster_size_must_be_0x348");

/* A particle temporarily takes over a monster animation while its shake event
   runs. The derived callback restores the saved representation state when the
   particle finishes and then deletes itself. */
class W8MonsterShakeCallbackBase {
public:
    virtual ~W8MonsterShakeCallbackBase() {}
};

class W8MonsterShakeCallback : public W8MonsterShakeCallbackBase {
public:
    W8MonsterShakeCallback()
        : m_pMonster(0), m_pParticles(0)
    {
    }

    virtual void RestoreAnimation();

    W8Monster* m_pMonster;
    stParticle* m_pParticles;
    unsigned char saved_behaviour;
    signed char saved_frame_method;
    unsigned char unknown_0e[2];
};

static_assert(
    sizeof(W8MonsterShakeCallback) == 0x10,
    "W8MonsterShakeCallback_size_must_be_0x10");
static_assert(sizeof(W8MonsterState28C) == 0x10, "W8MonsterState28C_size_must_be_0x10");
static_assert(sizeof(W8MonsterState2AC) == 0x2c, "W8MonsterState2AC_size_must_be_0x2c");
static_assert(sizeof(W8MonsterState2FC) == 0x10, "W8MonsterState2FC_size_must_be_0x10");
static_assert(sizeof(W8MonsterFlags330) == 4, "W8MonsterFlags330_size_must_be_4");

#endif
