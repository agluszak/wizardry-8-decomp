#ifndef WIZ8_ENGINE_CODE_MONSTER_H
#define WIZ8_ENGINE_CODE_MONSTER_H

#include "wiz8/monster_spell_icons.h"
#include "wiz8/monster_actions.h"
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

#include "wiz8/monster_cycles.h"

enum W8MonsterFlag {
    W8_MONSTER_KEEP_FRAME_DIRECTION = 0x1,
    W8_MONSTER_TEXTURE_CHECKED = 0x2,
    W8_MONSTER_ANIMATED_TEXTURE = 0x4,
    W8_MONSTER_SCALING_Y = 0x8,
    W8_MONSTER_KEEP_SUBCYCLE = 0x10,
    W8_MONSTER_SCRIPT_WAIT = 0x20,
    W8_MONSTER_PARKED = 0x40,
    W8_MONSTER_REMOVE_AFTER_FADE = 0x100,
    W8_MONSTER_REMOVE_NOW = 0x200,
    W8_MONSTER_FADED_OUT = 0x400,
};

extern const float g_monster_rotation_offset;
extern const double g_monster_facing_tolerance;
extern const double g_monster_poster_max_distance;
extern int g_monster_cycle_registry_weight;
extern float g_monster_light_scale;

/* Sixteen bytes the cycle runtime record carries at 0x04c, written as one block
   by the setter at 0x004C5AD0. That setter takes the block by value and VC6
   copies it with the interleaved two-register rotation it uses for a struct
   assignment, rather than the sequential load/store pairs four separate scalar
   parameters would emit - which is what makes this one object and not four.
   GrCycle copies the same 0x10 bytes onto a model instance at +0x164. */

/* One spell/condition icon attached to a monster: the icon id and the
   billboard object created for it. Field names come from the
   SetMonsterSpellIcon asserts "pSpellMI->psrBMO" and its icon compares. */
struct W8MonsterSpellIcon {
    W8MonsterSpellIconId icon;
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
    virtual W8AnimRepBase* Clone() override; /* 0x004CA9E0 */
    virtual srModelInstance* SetCycleFrameLod(signed char cycle, signed char frame,
                                              signed char lod) override;  /* 0x004BF8C0 */
    virtual unsigned int ApplyEmitterSetting(signed char cycle) override; /* 0x004BF970 */
    virtual W8AniMesh* GetEmitterAniMesh(signed char cycle) override;     /* 0x004BF920 */
    virtual void CopyCycle(signed char cycle, const W8MonsterRep* other, signed char other_cycle);
    unsigned char ReadCycleData(W8ReadLevelInfo* info, W8Monster* monster, int cycle_index,
                                int value);

    /* The spell-icon list; named by the SetMonsterSpellIcon assert
       "pMonRep->GetSpellIcons()". */
    W8PList* GetSpellIcons()
    {
        return spell_icons;
    }

    W8GrowableVector<W8AnimObj*> animations[W8_MONSTER_CYCLE_COUNT];                   /* 0x0ac */
    W8GrowableVector<float> animation_scales[W8_MONSTER_CYCLE_COUNT];                  /* 0x25c */
    W8GrowableVector<W8GrowableVector<stLight*>*> light_lists[W8_MONSTER_CYCLE_COUNT]; /* 0x40c */
    /* 0x5bc: per-party-member highlight bitmask - bit N set while party
       member N has the monster highlighted/targeted. */
    unsigned char highlight_mask;
    unsigned char padding_5bd[3];
    char* name; /* 0x5c0: owned copy */
    /* 0x5c4: number of populated party-icon entries in objects; the
       attachment layout read by UpdateAttachedObjects. */
    int icon_count;
    W8Item* objects[8];   /* 0x5c8 */
    W8PList* spell_icons; /* 0x5e8: W8MonsterSpellIcon records */
    float standing_height;
    float scale;
    float minimum_scale;
    float maximum_scale;
    /* 0x5fc: death shrink factor multiplying scale on the death path. */
    float death_scale;
    /* 0x600: the rep carries a random idle cycle - animations[1] gets a
       per-update random playback scale. */
    bool random_idle;
    /* 0x601: flies, swims or full-transitions - suppresses the grounded
       transition check at 0x004C0000. */
    bool special_movement;
    unsigned char padding_602[2];
    /* 0x604: the idle cycle's own playback scale, added to the per-update
       random roll. */
    float idle_playback_scale;
    float random_idle_fps_min;
    float random_idle_fps_max;
    /* 0x610: left-handed strike chance percent; Random(100) is rolled
       against it for the mirrored attack anim. */
    int left_handed;
    W8Vector<stModelInstance*> linked_runtime_objects;
    class MonsterLight* monster_light;

    unsigned char GetNumSubsPerCycle(signed char bCycle);
    /* 0x004C4660. A method, not the free function an earlier reading assumed:
       it takes its receiver in ECX and IsDying calls it without reloading ECX
       at all, relying on `this` already being there. The query selector is
       bounded at nine by the body's own `ja` against the jump table. */
};

/* The constructor at 0x004BEA20 initializes through 0x624; the observed
   allocation is 0x628 bytes. These bound the modeled extent, not the roles
   of every byte or the complete set of callers. */
W8_ABI_ASSERT(sizeof(W8MonsterRep) == 0x628, "W8MonsterRep_size_must_be_0x628");
W8_ABI_ASSERT(offsetof(W8MonsterRep, random_idle_fps_min) == 0x608,
              "W8MonsterRep_idle_fps_min_offset");
W8_ABI_ASSERT(offsetof(W8MonsterRep, random_idle_fps_max) == 0x60c,
              "W8MonsterRep_idle_fps_max_offset");

W8_ABI_ASSERT(sizeof(W8GrowableVector<W8AnimObj*>) == 0x10,
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
    virtual void GetMappedPosition(srVector3T<float>* position);
    virtual bool GetAnimationCenter(srVector3T<float>* center);
    virtual void SetPosition(const srVector3T<float>* position) override;

    int Query(W8MonsterQueryKind query);         /* 0x004C4660 */
    void SetForcedSubcycle(signed char value);   /* 0x004C6C00 */
    void SpawnDamageNumber(unsigned int amount); /* 0x004C6C30 */
    bool IsDying();                              /* 0x004CA4C0 */
    bool IsCycleInterruptable(signed char cycle);
    void ApplyRemovalStateEffects();
    void CollectModelInstances(W8GrowableVector<stModelInstance*>* instances);
    void SetDamageStage(int stage);
    int GetDamageStageCount();
    bool ReplaceSkinTexture(int stage, const char* old_name, const char* new_name);
    int AddDamageStage(const char* base_name, int stage);
    void RemoveCycleSkinTables();
    void RandomizeAppearanceAndMotion();
    bool IsRenderable(bool alternate);
    void InitializeAnimatedTexture();
    void HandleAnimationThreshold();
    void HandleAnimationFrame(unsigned char frame);
    void UpdateShakeEvents(unsigned char frame);
    void SetShakeEventVisibility(signed char cycle);
    void UpdateAttachedObjects();
    void BeginFadeIn(float duration);
    void BeginDelayedRemoval();
    void BeginFadeOutAndRemove(W8MonsterRemovalState state);
    void BeginFadeOut(float duration);
    void StartTalking(bool animate_mouth);
    void StopTalking();
    void SetCycleCallback(int cycle, CycleCallback callback);
    bool GetPatrolPoint(srVector3T<float>* point);
    void TrackSoundHandle(int handle);
    float GetDistanceToPlayer();
    float GetPointDistanceToPlayer(srVector3T<float> point);
    float GetDistanceToMonster(W8Monster* monster);
    float GetPointDistanceToMonster(W8Monster* monster, srVector3T<float> point);
    bool SetScript(const char* script_name, bool reset_orders);
    void ProcessScript();
    bool ResolveScriptPosition(const char* name, srVector3T<float>* position);
    bool GetProjectilePosition(srVector3T<float>* position);
    bool GetSpellPosition(srVector3T<float>* position);
    bool GetCycleMappedPosition(signed char cycle, int mapped_index, srVector3T<float>* position);
    bool EvaluateScriptCondition(const char* expression);
    bool CanContinueScript();
    bool SetScriptLabel(const char* label);
    bool IsPendingFinalize() const;
    bool IsWithinWorldRange();
    bool CheckLineOfSightToPlayer();
    void GetPlayerSightFlags(bool* primary, bool* secondary);
    bool IsVisibleToPlayer(bool use_bounds);
    void GetPlayerToMonsterSightFlags(bool* primary, bool* secondary,
                                      const srVector3T<float>* source);
    bool HasLineOfSightToMonster(W8Monster* monster);
    void GetMonsterSightFlags(W8Monster* monster, bool* primary, bool* secondary);
    bool HasLineOfSightFromPoint(srVector3T<float> point);
    int IsFacingMonster(W8Monster* monster);
    int IsFacingPlayer();
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
    unsigned int runtime_flags;
    int value_1e0;
    /* 0x1e4: the monster's location id, stored by MonsterSetLocationId and
       used throughout for MonsterInfo lookups. */
    int location_id;
    /* 0x1e8/0x1f0: the X/Z siblings of scale_y; mirror_x flips the
       X term for left-handed strikes. */
    float scale_x;
    /* Y-axis scale applied while W8_MONSTER_SCALING_Y is set (decayed per
       frame by g_float_one_tenth). */
    float scale_y;
    float scale_z;
    /* Attack-animation frame that triggers the missile launch. */
    int missile_frame;
    /* Cycle-25 animation frame that triggers the attached spell effect. */
    int spell_frame;
    /* 0x1fc: talking state armed by StartTalking; cleared by StopTalking. */
    bool talking;
    /* 0x1fd: the StartTalking argument; mouth texture animation only runs
       while it is set. */
    bool animate_mouth;
    unsigned char padding_1fe[2];
    /* 0x200/0x204: the 120 ms clock and the last frame of the random mouth
       flicker used while the gap track reports the mouth closed. */
    int mouth_frame_clock;
    int mouth_frame;
    unsigned int talk_start;
    unsigned int talk_duration;
    int talk_state;
    /* 0x214: the current mouth state the dialogue update copies out of the
       active W8MouthGapTrack; forces mouth frame 0 while open. */
    unsigned char mouth_open; // bool-byte-ok: copied raw from the C gap track byte
    /* 0x215: set while the monster is deactivated (active cleared). */
    bool inactive;
    /* 0x216: raised at construction; cleared once AddMonsterToWorld and the
       spawn bookkeeping finish - iteration skips monsters still pending. */
    bool pending_finalize;
    /* 0x217: suppresses rendering and radar/automap display. */
    bool disabled;
    bool nearest_to_party;
    unsigned char padding_219[3];
    /* 0x21c/0x220: hover base-height random range (scaled by
       g_world_scale into movement.vertical_base). */
    float hover_base_min;
    float hover_base_max;
    /* 0x224/0x228: bob-amplitude random range (scaled into
       movement.vertical_amplitude). */
    float bob_amplitude_min;
    float bob_amplitude_max;
    /* 0x22c: the missing spell-launch-vertex warning already fired once. */
    bool spell_vertex_warned;
    /* 0x22d: the missing missile-start-point warning already fired once. */
    bool missile_point_warned;
    W8MonsterRemovalState removal_state;
    unsigned char padding_22f;
    CycleCallback cycle_callback;
    int callback_cycle;
    stScript* script;
    int script_line;
    W8MonsterScriptCommand script_wait;
    W8GrowableVector<bool> script_conditions;
    W8GameTimer script_delay_timer;
    Trigger* trigger;
    int registry_weight;
    srVector3T<float> formation;
    bool defining_orders;
    bool orders_finished;
    W8MonsterOrderMode order_mode;
    bool deaf;
    bool face_party;
    bool stay_home;
    unsigned char padding_292[2];
    float patrol_distance;
    float patrol_variation;
    W8GrowableVector<srVector3T<float> > vector;
    signed char patrol_index;
    unsigned char padding_2ad[3];
    float direction_x;
    float direction_y;
    float direction_z;
    /* 0x2bc: the direction the real-time AI moves the monster along, added to
       its position to pick the aim point (mode 0xa). */
    srVector3T<float> move_direction;
    int look_frequency;
    int look_duration;
    /* 0x2d0: sun-visibility state for the model light-scale lerp: -1
       uninitialized, 1 lit (scale toward 0.75), 0 shadowed (toward 0). */
    W8MonsterSunlightState sunlit_state;
    bool move_dirty;
    unsigned char padding_2d5[3];
    W8GameTimer light_scale_timer;
    float target_scale;
    float current_scale;
    /* 0x304: one-shot latch; the cycle-25 spell frame fires
       CreateAttachedSpellEffect once then clears it. */
    bool spell_effect_armed;
    unsigned char padding_305[3];
    /* The render mesh of the world sector this monster currently occupies,
       cached by W8Octree::UpdateMonsterLocation (null while unplaced).
       IsWithinWorldRange answers from its disable flag when present.
       This is not the inherited navigator scene node. */
    srNode* sector_mesh;
    W8GameTimer fade_timer;
    W8MonsterFadeState fade_state;
    /* 0x331: this monster is the highlighted target; exempt from the
       attachment distance-scale clamp. */
    bool target_highlighted;
    /* 0x332: copied from the source monster; blocks hostility recompute in
       Targeting and Combat Hostility. */
    bool hostility_preserved;
    unsigned char padding_333;
    stSound3D* sound;
    W8GrowableVector<int> values;
};

W8MonsterCycle ParseMonsterCycleName(const char* name, signed char* subcycle = 0);
unsigned char ReadOrCloneMonsterCycles(const W8GrCycleLoadContext* context,
                                       const char* monster_name, W8Monster** monster,
                                       int load_value, int location_id);
unsigned char MonsterReadAllCycles(const W8GrCycleLoadContext* context, const char* monster_name,
                                   W8Monster** monster, int load_value, int location_id);
unsigned short ChooseDifferentMonsterDirection(unsigned short previous_direction);

bool MonsterGetWorldAnimationBounds(W8Monster* monster, srVector3T<float>* minimum,
                                    srVector3T<float>* maximum);
unsigned char LoadMonsterCycle(const W8GrCycleLoadContext* context, const char* mon_name,
                               W8Monster** monster, int cycle, int value);

bool MonsterUsesCurrentModelInstance(W8GrCycle* cycle);
void MonsterGetLocation(W8Monster* monster, srVector3T<float>* location);
/* Expose the first Navigator angle through the enclosing Monster. */
float MonsterGetYaw(W8Monster* monster); /* 0x004C5770 */
void MonsterGetLocalLocation(W8Monster* monster, srVector3T<float>* location);
void UpdateMonster(W8Monster* monster);
bool MonsterIsCycleSupported(W8Monster* monster, signed char cycle);
unsigned char MonsterReplacePath(W8Monster* monster, W8PathAI* path);
unsigned char MonsterGetAnimationRadius(W8Monster* monster, float* radius);
void MonsterSetFacing(W8Monster* monster, float angle);
bool MonsterGetMirrorX(W8Monster* monster);
void MonsterSetMirrorX(W8Monster* monster, bool state);
float MonsterGetScale(W8Monster* monster);
void MonsterSetScale(W8Monster* monster, float scale);
void MonsterGetScaleRange(W8Monster* monster, float* minimum, float* maximum);
void MonsterSetAdjustedPosition(W8Monster* monster, const srVector3T<float>* position);
unsigned short MonsterApproachStartupNavigator(W8Monster* monster, double separation);
unsigned char MonsterLinkToStartupNavigator(W8Monster* monster);
unsigned short MonsterConfigureMovementToPlayer(W8Monster* monster, float separation,
                                                float maximum_distance, srVector3T<float> position,
                                                int trace_mode, unsigned char* probe_result);
unsigned short MonsterConfigureMovementToMonster(W8Monster* monster, W8Monster* target,
                                                 float separation, float maximum_distance,
                                                 srVector3T<float> position, int trace_mode,
                                                 unsigned char* probe_result);
void MonsterAimAtMonster(W8Monster* monster, W8Monster* target, bool alternate);
void MonsterSetCycle(W8Monster* monster, signed char cycle);
void MonsterSetActive(W8Monster* monster, bool state); /* 0x004C6160 */
void MonsterSetCycleBehaviour(W8GrCycle* cycle, signed char behaviour);
void MonsterSetCycleSubCycle(W8GrCycle* cycle, unsigned char subcycle);
void SetCombatInactiveFlag(unsigned char value);
void UpdateNearestMonsterGroupMembers();
void ApplyMonsterRepresentationScale(W8Monster* monster);

W8_ABI_ASSERT(sizeof(W8Monster) == 0x348, "W8Monster_size_must_be_0x348");
/* Secondary vftable 0x005ed218 keeps the W8Navigator subobject at +0x18. */
W8_ASSERT_BASE_OFFSET(W8Monster, W8Navigator, navigation_mode, 0x18);

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

    virtual void RestoreAnimation();

    W8Monster* m_pMonster;
    stParticle* m_pParticles;
    unsigned char saved_behaviour;
    signed char saved_frame_method;
    unsigned char padding_0e[2];
};

W8_ABI_ASSERT(sizeof(W8MonsterShakeCallback) == 0x10, "W8MonsterShakeCallback_size_must_be_0x10");

void MonsterStopAllNavigators(void);
unsigned char MonsterGetHighlightMask(W8Monster* monster);
void MonsterSetHighlightMask(W8Monster* monster, unsigned char flag);
void MonsterSetHighlightColour(W8Monster* monster, srVector4T<float> block);
unsigned char MonsterSetAnimating(W8Monster* monster, bool animating);
unsigned char MonsterIsAnimating(W8Monster* monster);
bool MonsterHasPendingCycle(W8Monster* monster); /* 0x004C5710 */
bool MonsterIsScalingY(W8Monster* monster);      /* 0x004C5EE0 */
void MonsterSetPendingCycle(W8Monster* monster, int cycle);
int MonsterQuery(W8Monster* monster, W8MonsterQueryKind query);
void MonsterClearMovement(W8Monster* monster);
void MonsterSetRuntimeBehaviour(W8Monster* monster, signed char behaviour);
void MonsterSubmitTargetValue(W8Monster* monster);
void DetachMonsterRepresentation(W8Monster* monster, W8World* world);
void DeleteMonster(W8Monster* monster);
void RefreshMonsterStandingHeight(W8Monster* monster);
void MonsterSetLocationId(W8Monster* monster, int value);
void MonsterSelectLOD(W8Monster* monster, const srVector3T<float>* position);
/* The shared forwarder four call sites use to advance a cycle's
   representation; it stays free because its callers pass the object on the
   stack. */
void UpdateCycleRepresentation(W8GrCycle* cycle, W8World* world);
void MonsterSetNavigatorHalted(W8Monster* monster, bool value);
W8AIRecord* MonsterGetAIRecord(W8Monster* monster);                     /* 0x004C5B30 */
void MonsterSetNavigatorMovementScale(W8Monster* monster, float value); /* 0x004C5F50 */
float MonsterGetNavigatorMovementScale(W8Monster* monster);             /* 0x004C5F70 */
unsigned char
MonsterConfigureMovementToPosition(W8Monster* monster,
                                   const srVector3T<float>* position);           /* 0x004C5F90 */
void MonsterAddPathPoint(W8Monster* monster, const srVector3T<float>* argument); /* 0x004C5FB0 */
void MonsterSetPathLooping(W8Monster* monster, char value);                      /* 0x004C5FD0 */
void MonsterResumeAllNavigators(void);

void SetMonsterPartySlotMarker(int party_slot, int location_id, char on);
void MonsterForwardReferencePosition(W8Monster* monster, char alternate); /* 0x004C6240 */
void NotifyMonsterHighlight(int party_slot, int location_id, int on);

W8Item* CreateMonsterIconItem(W8World* world, const char* path, int flag);
/* One eight-byte row per animation cycle at 0x0060EA08. The parser at
   0x004C2010 reads prefix_length as a signed byte. Its optional atoi offset
   comes from the search index, which can differ from the returned cycle. */
struct W8CycleNameRow {
    const char* name;
    signed char prefix_length;
};
extern W8CycleNameRow g_cycle_names[];
// bool-byte-ok: automap save/restore (0x0057E660/0x0057FB40) copies it through a byte array unnormalized
extern unsigned char g_monster_shadow_updates_enabled;

#endif
