#ifndef WIZ8_ENGINE_CODE_MONSTER_H
#define WIZ8_ENGINE_CODE_MONSTER_H

#include <stddef.h>

#include "surrender/srMath.h"
#include "surrender/srHeap.h"
#include "surrender/srTypeRegistry.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/geometry.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/local_code/MonsterGroup.h"

struct W8AnimObj;
struct W8ReadLevelInfo;
struct W8PList;
struct W8Item;
class stModelInstance;
class stLight;
class stParticle;
class stScript;
class stSound3D;
class Trigger;
class W8Monster;

struct W8MonsterRep;

enum { W8_MONSTER_CYCLE_COUNT = 27 };

extern const float g_monster_rotation_offset_005ec04c;
extern const double g_monster_facing_tolerance_005ec2b0;
extern const double g_monster_poster_max_distance_005ec3d8;
extern int g_monster_cycle_registry_weight_0065ba4c;
extern float g_light_scale_0060bfe0;

/* Sixteen bytes the cycle runtime record carries at 0x04c, written as one block
   by the setter at 0x004C5AD0. That setter takes the block by value and VC6
   copies it with the interleaved two-register rotation it uses for a struct
   assignment, rather than the sequential load/store pairs four separate scalar
   parameters would emit - which is what makes this one object and not four.
   GrCycle copies the same 0x10 bytes onto a model instance at +0x164. */
typedef W8ModelInstance3DRenderState W8MonsterRuntimeBlock4C;

/* One spell/condition icon attached to a monster: the icon id and the
   billboard object created for it. Field names come from the
   SetMonsterSpellIcon asserts "pSpellMI->psrBMO" and its icon compares. */
struct W8MonsterSpellIcon {
    int icon;
    W8Item* psrBMO;
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
    virtual srModelInstance* SetCycleFrameLod(signed char cycle, signed char frame,
                                              signed char lod) override; /* 0x004BF8C0 */
    virtual unsigned int ApplyEmitterSetting(char cycle) override;       /* 0x004BF970 */
    virtual W8AniMesh* GetEmitterAniMesh(char cycle) override;           /* 0x004BF920 */
    virtual void CopyCycle004BF0F0(signed char cycle, const W8MonsterRep* other,
                                   signed char other_cycle);
    unsigned char ReadCycleData004BF520(W8ReadLevelInfo* info, W8Monster* monster, int cycle_index,
                                        int value);

    /* The spell-icon list; named by the SetMonsterSpellIcon assert
       "pMonRep->GetSpellIcons()". */
    W8PList* GetSpellIcons()
    {
        return spell_icons_5e8;
    }

    W8GrowableVector<W8AnimObj*> animations[W8_MONSTER_CYCLE_COUNT];                   /* 0x0ac */
    W8GrowableVector<float> animation_scales[W8_MONSTER_CYCLE_COUNT];                  /* 0x25c */
    W8GrowableVector<W8GrowableVector<stLight*>*> light_lists[W8_MONSTER_CYCLE_COUNT]; /* 0x40c */
    /* 0x5bc: per-party-member highlight bitmask - bit N set while party
       member N has the monster highlighted/targeted. */
    unsigned char highlight_mask_5bc;
    unsigned char padding_5bd[3];
    char* name_5c0; /* 0x5c0: owned copy */
    /* 0x5c4: number of populated party-icon entries in objects_5c8; the
       attachment layout read by UpdateAttachedObjects. */
    int icon_count_5c4;
    W8Item* objects_5c8[8];   /* 0x5c8 */
    W8PList* spell_icons_5e8; /* 0x5e8: W8MonsterSpellIcon records */
    float standing_height_5ec;
    float scale_5f0;
    float minimum_scale_5f4;
    float maximum_scale_5f8;
    /* 0x5fc: death shrink factor multiplying scale_5f0 on the death path. */
    float death_scale_5fc;
    /* 0x600: the rep carries a random idle cycle - animations[1] gets a
       per-update random playback scale. */
    unsigned char random_idle_600;
    /* 0x601: flies, swims or full-transitions - suppresses the grounded
       transition check at 0x004C0000. */
    unsigned char special_movement_601;
    unsigned char padding_602[2];
    /* 0x604: the idle cycle's own playback scale, added to the per-update
       random roll. */
    float idle_playback_scale_604;
    float random_idle_fps_min;
    float random_idle_fps_max;
    /* 0x610: left-handed strike chance percent; Random(100) is rolled
       against it for the mirrored attack anim. */
    int left_handed_610;
    W8GrowableVector<stModelInstance*> linked_runtime_objects_614;
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
static_assert(offsetof(W8MonsterRep, random_idle_fps_min) == 0x608,
              "W8MonsterRep_idle_fps_min_offset");
static_assert(offsetof(W8MonsterRep, random_idle_fps_max) == 0x60c,
              "W8MonsterRep_idle_fps_max_offset");

static_assert(sizeof(W8GrowableVector<W8AnimObj*>) == 0x10,
              "W8Monster_animation_vector_must_be_0x10");

/* The GrCycle factory allocates 0x348 bytes and calls the constructor at
   0x004BFB00 for object type zero. Both constructors and the destructor install
   primary vtable 0x005ED22C and the W8Navigator secondary-base table at +0x18. */
class W8Monster : public W8GrCycle {
public:
    typedef void(__cdecl* CycleCallback)(W8Monster* monster);

    W8Monster();
    W8Monster(const W8Monster& rhs);
    virtual ~W8Monster() override;

    virtual unsigned char CanEnterCycle(signed char cycle) override;
    virtual void UpdateRepresentation(W8World* world) override;
    virtual signed char GetNumSubCycles() override;
    virtual bool IsCycleSupported(signed char cycle) override;
    virtual signed char GetTotalAnimationCount() override;
    virtual float GetCurrentAnimationScale() override;
    virtual W8EmitterHost* GetRepresentation() override;
    virtual unsigned char GetAnimationBounds(srVector3T<float>* minimum,
                                             srVector3T<float>* maximum) override;
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

    int Query(int query);                        /* 0x004C4660 */
    void SetForcedSubcycleA6(signed char value); /* 0x004C6C00 */
    void SpawnDamageNumber(unsigned int amount); /* 0x004C6C30 */
    bool IsDying();                              /* 0x004CA4C0 */
    unsigned char IsCycleInterruptable(signed char cycle);
    void ApplyRemovalStateEffects();
    void CollectModelInstances004C6350(W8GrowableVector<stModelInstance*>* instances);
    void SetDamageStage004C6990(int stage);
    int GetDamageStageCount004C6A50();
    unsigned char ReplaceSkinTexture004C6700(int stage, const char* old_name, const char* new_name);
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
    void SetCycleCallback004CA340(int cycle, CycleCallback callback);
    unsigned char GetPatrolPoint004CA360(srVector3T<float>* point);
    void TrackSoundHandle004CA6E0(int handle);
    float GetDistanceToPlayer004C7CB0();
    float GetPointDistanceToPlayer004C7D50(srVector3T<float> point);
    float GetDistanceToMonster004C7DD0(W8Monster* monster);
    float GetPointDistanceToMonster004C7E80(W8Monster* monster, srVector3T<float> point);
    unsigned char SetScript004C7F10(const char* script_name, unsigned char reset_orders);
    void ProcessScript004C80E0();
    unsigned char GetProjectilePosition004C77F0(srVector3T<float>* position);
    unsigned char GetSpellPosition004C78E0(srVector3T<float>* position);
    unsigned char GetCycleMappedPosition004C7960(signed char cycle, int mapped_index,
                                                 srVector3T<float>* position);
    unsigned char EvaluateScriptCondition004C9DC0(const char* expression);
    bool CanContinueScript004CA0F0();
    unsigned char SetScriptLabel004CA260(const char* label);
    bool IsPendingFinalize004CA290() const;
    bool IsWithinWorldRange004CA2A0();
    unsigned char CheckLineOfSightToPlayer004C4810();
    void GetPlayerSightFlags004C4870(unsigned char* primary, unsigned char* secondary);
    unsigned char IsVisibleToPlayer004C4920(unsigned char use_bounds);
    void GetPlayerToMonsterSightFlags004C4A20(unsigned char* primary, unsigned char* secondary,
                                              const srVector3T<float>* source);
    unsigned char HasLineOfSightToMonster004C4AF0(W8Monster* monster);
    void GetMonsterSightFlags004C4B70(W8Monster* monster, unsigned char* primary,
                                      unsigned char* secondary);
    unsigned char HasLineOfSightFromPoint004C4C40(srVector3T<float> point);
    int IsFacingMonster004C4CA0(W8Monster* monster);
    int IsFacingPlayer004C4D40();
    void ApplyRepresentationScale();
    void RefreshStandingHeight();

public:
    /* Assertion-backed original spelling. Distinct from GrObject::m_pRep at
       +0x14; GetRepresentation() returns this GrCycle-tail slot at +0x1d8. */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow-field"
#endif
    W8MonsterRep* m_pRep;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    unsigned int flags_1dc;
    int value_1e0;
    /* 0x1e4: the monster's location id, stored by MonsterSetLocationId and
       used throughout for MonsterInfo lookups. */
    int location_id_1e4;
    /* 0x1e8/0x1f0: the X/Z siblings of scale_y_1ec; mirror_x_1be flips the
       X term for left-handed strikes. */
    float scale_x_1e8;
    /* Y-axis squash scale applied while flags_1dc bit 8 is set (decayed per
       frame by g_float_005ebc3c). */
    float scale_y_1ec;
    float scale_z_1f0;
    /* Attack-animation frame that triggers the missile launch. */
    int missile_frame_1f4;
    /* Cycle-25 animation frame that triggers the attached spell effect. */
    int spell_frame_1f8;
    /* 0x1fc: talking state armed by StartTalking; cleared by StopTalking. */
    unsigned char talking;
    /* 0x1fd: the StartTalking argument; mouth texture animation only runs
       while it is set. */
    unsigned char animate_mouth;
    unsigned char padding_1fe[2];
    /* 0x200/0x204: the 120 ms clock and the last frame of the random mouth
       flicker used while the gap track reports the mouth closed. */
    int mouth_frame_clock;
    int mouth_frame;
    int talk_start_208;
    int talk_duration_20c;
    int talk_state_210;
    /* 0x214: the current mouth state the dialogue update copies out of the
       active W8MouthGapTrack; forces mouth frame 0 while open. */
    unsigned char mouth_open;
    /* 0x215: set while the monster is deactivated (active_088 cleared). */
    unsigned char inactive_215;
    /* 0x216: raised at construction; cleared once AddMonsterToWorld and the
       spawn bookkeeping finish - iteration skips monsters still pending. */
    bool pending_finalize_216;
    /* 0x217: suppresses rendering and radar/automap display. */
    bool disabled_217;
    unsigned char nearest_to_party_218;
    unsigned char padding_219[3];
    /* 0x21c/0x220: hover base-height random range (scaled by
       g_world_scale_005ebc40 into movement_0c0.vertical_base_07c). */
    int hover_base_min_21c;
    int hover_base_max_220;
    /* 0x224/0x228: bob-amplitude random range (scaled into
       movement_0c0.vertical_amplitude_080). */
    int bob_amplitude_min_224;
    int bob_amplitude_max_228;
    /* 0x22c: the missing spell-launch-vertex warning already fired once. */
    unsigned char spell_vertex_warned_22c;
    /* 0x22d: the missing missile-start-point warning already fired once. */
    unsigned char missile_point_warned_22d;
    signed char removal_state_22e;
    unsigned char padding_22f;
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
    unsigned char defining_orders_28c;
    unsigned char orders_finished_28d;
    signed char order_mode_28e;
    unsigned char deaf_28f;
    unsigned char face_party_290;
    unsigned char stay_home_291;
    unsigned char padding_292[2];
    float patrol_distance_294;
    float patrol_variation_298;
    W8GrowableVector<srVector3T<float> > vector_29c;
    signed char patrol_index_2ac;
    unsigned char padding_2ad[3];
    float direction_x_2b0;
    float direction_y_2b4;
    float direction_z_2b8;
    /* 0x2bc: the direction the real-time AI moves the monster along, added to
       its position to pick the aim point (mode 0xa). */
    srVector3T<float> move_direction_2bc;
    float look_frequency_2c8;
    float look_duration_2cc;
    /* 0x2d0: sun-visibility state for the model light-scale lerp: -1
       uninitialized, 1 lit (scale toward 0.75), 0 shadowed (toward 0). */
    int sunlit_state_2d0;
    unsigned char position_dirty_2d4;
    unsigned char padding_2d5[3];
    W8GameTimer timer_2d8;
    float target_scale_2fc;
    float current_scale_300;
    /* 0x304: one-shot latch; the cycle-25 spell frame fires
       CreateAttachedSpellEffect once then clears it. */
    unsigned char spell_effect_armed_304;
    unsigned char padding_305[3];
    srNode* node_308;
    W8GameTimer timer_30c;
    signed char fade_state_330;
    /* 0x331: this monster is the highlighted target; exempt from the
       attachment distance-scale clamp. */
    unsigned char target_highlighted_331;
    /* 0x332: copied from the source monster; blocks hostility recompute in
       Targeting and Combat Hostility. */
    unsigned char hostility_preserved_332;
    unsigned char padding_333;
    stSound3D* sound_334;
    W8GrowableVector<int> values_338;
};

int ParseMonsterCycleName004C2010(const char* name, signed char* subcycle = 0);
unsigned char MonsterReadAllCycles004C0300(const W8GrCycleLoadContext* context,
                                           const char* monster_name, W8Monster** monster,
                                           int load_value, int location_id);
unsigned char MonsterReadAllCycles004C58E0(const W8GrCycleLoadContext* context,
                                           const char* monster_name, W8Monster** monster,
                                           int load_value, int location_id);
unsigned short ChooseDifferentMonsterDirection004C2E00(unsigned short previous_direction);

unsigned char MonsterGetWorldAnimationBounds004CA4F0(W8Monster* monster, srVector3T<float>* minimum,
                                                     srVector3T<float>* maximum);
unsigned char LoadMonsterCycle004C5910(const W8GrCycleLoadContext* context, const char* mon_name,
                                       W8Monster** monster, int cycle, int value);

bool MonsterUsesCurrentModelInstance(W8GrCycle* cycle);
void MonsterGetLocation(W8Monster* monster, srVector3T<float>* location);
/* Expose the first Navigator angle through the enclosing Monster. */
float MonsterGetAngleD4004C5770(W8Monster* monster); /* 0x004C5770 */
void MonsterGetLocalLocation(W8Monster* monster, srVector3T<float>* location);
void UpdateMonster(W8Monster* monster);
bool MonsterIsCycleSupported(W8Monster* monster, signed char cycle);
unsigned char MonsterReplacePath(W8Monster* monster, W8PathAI* path);
unsigned char MonsterGetAnimationRadius(W8Monster* monster, float* radius);
void MonsterSetFacing004C5B60(W8Monster* monster, float angle);
unsigned char MonsterGetMirrorX(W8Monster* monster);
void MonsterSetMirrorX(W8Monster* monster, unsigned char state);
float MonsterGetScale(W8Monster* monster);
void MonsterSetScale(W8Monster* monster, float scale);
void MonsterGetScaleRange(W8Monster* monster, float* minimum, float* maximum);
void MonsterSetAdjustedPosition004C5F00(W8Monster* monster, const srVector3T<float>* position);
unsigned short MonsterApproachStartupNavigator004C5FF0(W8Monster* monster, double separation);
unsigned char MonsterLinkToStartupNavigator004C6030(W8Monster* monster);
unsigned short MonsterConfigureMovementToPlayer004C6070(W8Monster* monster, float separation,
                                                        float maximum_distance,
                                                        srVector3T<float> position, int trace_mode,
                                                        unsigned char* probe_result);
unsigned short MonsterConfigureMovementToMonster004C60D0(W8Monster* monster, W8Monster* target,
                                                         float separation, float maximum_distance,
                                                         srVector3T<float> position, int trace_mode,
                                                         unsigned char* probe_result);
void MonsterAimAtMonster004C62C0(W8Monster* monster, W8Monster* target, char alternate);
void MonsterSetCycle(W8Monster* monster, signed char cycle);
void MonsterSetStateA0(W8Monster* monster, unsigned char state); /* 0x004C6160 */
void MonsterSetCycleBehaviour(W8GrCycle* cycle, signed char behaviour);
void MonsterSetCycleSubCycle(W8GrCycle* cycle, unsigned char subcycle);
void SetCombatInactiveFlag(unsigned char value);
void UpdateNearestMonsterGroupMembers004CA570();
void ApplyMonsterRepresentationScale(W8Monster* monster);

static_assert(sizeof(W8Monster) == 0x348, "W8Monster_size_must_be_0x348");
/* Secondary vftable 0x005ed218 keeps the W8Navigator subobject at +0x18. */
W8_ASSERT_BASE_OFFSET(W8Monster, W8Navigator, padding_004, 0x18);

/* A particle temporarily takes over a monster animation while its shake event
   runs. The derived callback restores the saved representation state when the
   particle finishes and then deletes itself.

   Class-triage: two vtables plus the derived-to-base vptr swap are real ABI
   facts. The empty base keeps its own table at 0x005ed290; the derived
   ordinary destructor at 0x004c3730 is the seven-byte vptr swap onto that
   table. Nothing recovered stores or dispatches through the base pointer. */
class W8MonsterShakeCallbackBase {
public:
    virtual ~W8MonsterShakeCallbackBase() {}
};

class W8MonsterShakeCallback : public W8MonsterShakeCallbackBase {
public:
    W8MonsterShakeCallback() : m_pMonster(0), m_pParticles(0) {}
    virtual ~W8MonsterShakeCallback();

    virtual void RestoreAnimation();

    W8Monster* m_pMonster;
    stParticle* m_pParticles;
    unsigned char saved_behaviour;
    signed char saved_frame_method;
    unsigned char padding_0e[2];
};

static_assert(sizeof(W8MonsterShakeCallback) == 0x10, "W8MonsterShakeCallback_size_must_be_0x10");

void MonsterForward453160(void);
unsigned char MonsterGetHighlightMask(W8Monster* monster);
void MonsterSetHighlightMask(W8Monster* monster, unsigned char flag);
void MonsterSetRuntimeBlock4C(W8Monster* monster, W8MonsterRuntimeBlock4C block);
unsigned char MonsterSetAnimating(W8Monster* monster, unsigned char animating);
unsigned char MonsterIsAnimating(W8Monster* monster);
bool MonsterHasPendingCycle(W8Monster* monster);          /* 0x004C5710 */
unsigned char MonsterHasCycle19Flag3(W8Monster* monster); /* 0x004C5EE0 */
void MonsterSetPendingCycle(W8Monster* monster, int cycle);
int MonsterQuery(W8Monster* monster, int query);
void MonsterForward4537E0(W8Monster* monster);
void MonsterSetRuntimeBehaviour(W8Monster* monster, signed char behaviour);
void MonsterForward4A84A0(W8Monster* monster);
void DetachMonsterRepresentation(W8Monster* monster, W8World* world);
void DeleteMonster004C5860(W8Monster* monster);
void RefreshMonsterStandingHeight(W8Monster* monster);
void MonsterSetLocationId004C5870(W8Monster* monster, int value);
void MonsterForward4A7BE0(W8Monster* monster, const srVector3T<float>* position);
/* The shared forwarder four call sites use to advance a cycle's
   representation; it stays free because its callers pass the object on the
   stack. */
void UpdateCycleRepresentation004C59B0(W8GrCycle* cycle, W8World* world);
void MonsterSetNavigatorFlag25(W8Monster* monster, char value);
W8AIRecord* MonsterGetObject0C(W8Monster* monster);                /* 0x004C5B30 */
void MonsterSetNavigatorValue120(W8Monster* monster, float value); /* 0x004C5F50 */
float MonsterGetNavigatorValue120(W8Monster* monster);             /* 0x004C5F70 */
unsigned char MonsterForward452630(W8Monster* monster,
                                   const srVector3T<float>* position);            /* 0x004C5F90 */
void MonsterForward453690(W8Monster* monster, const srVector3T<float>* argument); /* 0x004C5FB0 */
void MonsterSetNavigatorObjectFlag38(W8Monster* monster, char value);             /* 0x004C5FD0 */
void MonsterForward4531A0(void);

void SetMonsterHighlightColour(W8Monster* monster, float r, float g, float b, float a);
void NotifyMonsterOfSound(W8Monster* monster, int arg_2);
void NotifyMonsterIdle(W8Monster* monster, int arg_2);
void NotifyMonsterFacing(W8Monster* monster, W8Monster* target, int arg_3);

void SetMonsterPartySlotMarker004C4DE0(int party_slot, int location_id, char on);
void MonsterForwardReferencePosition(W8Monster* monster, char alternate); /* 0x004C6240 */
void NotifyMonsterHighlight(int party_slot, int location_id, int on);

W8Item* CreateMonsterIconItem004C5500(W8World* world, const char* path, int flag);
/* One eight-byte row per animation cycle at 0x0060EA08. The parser at
   0x004C2010 compares exactly prefix_length characters and then uses the same
   offset to read an optional numeric subcycle suffix. */
struct W8CycleNameRow {
    const char* name;
    int prefix_length;
};
extern W8CycleNameRow g_cycle_names[];
extern unsigned char g_monster_shadow_updates_enabled_0065970c;

#endif
