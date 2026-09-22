#pragma once

unsigned char LoadMissileDatabase(void);
void ReleaseMissileDatabase(void);

#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/local_code/Targeting.h"

struct W8ReadLevelInfo;
struct W8World;
class stLight;
class W8Missile;

/* The kind-3 record of the tagged AI family W8GrObject holds at +0x0c (the
   path record is kind 0). UpdateMissileAI004A4CF0 ticks it: +0x04 is the
   per-step distance scale, +0x08 the remaining turn budget, +0x0c the missile
   it steers (CopyAIMissile004A53A0 deliberately leaves it unset), +0x10 the
   half-tick baseline, +0x14 the elapsed flight clock, +0x18 the early-impact
   limit and +0x1c a trailing flag. */
struct W8AIMissile : W8AIRecord {
    /* Gravity latch copied from the missile's gravity_1e3: when set, the
       vertical fall rate decays each step. */
    unsigned char gravity_01;
    unsigned char padding_02[2];
    /* Per-step advance scale (speed units per step tick). */
    float speed_per_step_04;
    /* Current vertical fall rate, seeded from launch pitch and decayed by
       gravity_01. */
    float fall_speed_08;
    W8Missile* missile_0c;
    /* Half-tick baseline (getMsTime()>>1) the delta against the current
       half-tick is clamped to 0xfa. */
    int last_half_tick_10;
    float elapsed_14;
    float limit_18;
    unsigned char padding_1c;
    unsigned char padding_1d[3];
};

W8AIMissile* CopyAIMissile004A53A0(const W8AIMissile* source);
unsigned char UpdateMissileAI004A4CF0(W8AIMissile* record);
float AdvanceMissileAI004A50A0(W8AIMissile* record, srVector3T<float>* out, unsigned int steps);

/* The missile constructor allocates this complete 0x108-byte representation,
   invokes W8EmitterHost on the same receiver, constructs the two light-list
   vectors, and installs vtable 0x005ECDE0. Its copy constructor and destructor
   at 0x004A2DB0 and 0x004A3230 own the same storage. */
class W8MissileRep : public W8EmitterHost {
public:
    W8MissileRep();
    W8MissileRep(const W8MissileRep& other);
    virtual ~W8MissileRep() override;
    virtual W8AnimRepBase005EC1D8* Clone() override;
    virtual srModelInstance* SetCycleFrameLod(signed char cycle, signed char frame,
                                              signed char lod) override;
    virtual unsigned int ApplyEmitterSetting(char emitter) override;
    virtual W8AniMesh* GetEmitterAniMesh(char emitter) override;
    unsigned char ReadCycleData004A3300(W8ReadLevelInfo* info, W8Missile* missile, int positional_2,
                                        int emitter_index);

    unsigned int value_0ac;
    unsigned int value_0b0;
    unsigned char unknown_0b4[0x24];
    W8AnimObj* emitters[2];
    float emitter_values[2];
    W8GrowableVector<W8GrowableVector<stLight*>*> light_lists[2];
};

static_assert(sizeof(W8MissileRep) == 0x108, "W8MissileRep_size_must_be_0x108");

/* The copy path allocates 0x328 bytes and invokes W8GrCycle's copy constructor
   on the same receiver before installing the missile's primary and navigator
   tables. The source assertion for its AI names the object `pMissile`. */
class W8Missile : public W8GrCycle {
public:
    W8Missile();
    W8Missile(const W8Missile& other); /* 0x004A3E50 */
    virtual ~W8Missile() override;

    virtual void UpdateRepresentation(W8World* world) override;
    virtual signed char GetNumSubCycles() override;
    virtual bool IsCycleSupported(signed char cycle) override;
    virtual signed char GetTotalAnimationCount() override;
    virtual float GetCurrentAnimationScale() override;
    virtual W8EmitterHost* GetRepresentation() override;
    virtual void SetCycle(signed char cycle) override;
    virtual W8AnimObj* GetCurrentAnimation() override;
    virtual void AdvanceAnimationFrame(int value, int flags) override;
    virtual W8AniMesh* GetCurrentAniMesh() override;
    virtual void StartIfHostActive(); /* 0x004A4050 */
    /* Decide what the missile struck - the party or a live monster - record it
       in combat_slot_260, roll the target's missile deflection and either hand
       the hit to Combat Attack.cpp or leave it for the combat engine. */
    virtual bool OnCollision(W8Navigator* other) override; /* 0x004A4720 */

    unsigned long GetAnimationState004A4640(int mode);
    void DetonateMissileSpell004A49E0();
    void AnnounceCollisionTarget(); /* 0x004A4AC0 */
    /* Switch the representation to its impact cycle, or end the flight when the
       missile has no such cycle. */
    void EnterImpactCycle(); /* 0x004A4C20 */

    void SetEffectDefinition(const W8SpellEffectDefinition* definition); /* 0x004A5410 */
    /* 0x004A5790: true while this in-flight missile still blocks ending combat. */
    bool BlocksEndingCombat004A5790();

public:
    int missile_table_index_1d8;
    /* Assertion-backed original spelling. Distinct from GrObject::m_pRep at
       +0x14; GetRepresentation() returns this GrCycle-tail slot at +0x1dc. */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow-field"
#endif
    W8MissileRep* m_pRep;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    /* Flight exhausted its duration; the world updater destroys the missile
       once block_released_1e2 is also set. */
    bool flight_done_1e0;
    /* Set by EnterImpactCycle while the impact animation plays. */
    unsigned char impacting_1e1;
    /* Set once the missile no longer blocks combat end (BlocksEndingCombat
       returned 0 or combat already resolved). */
    unsigned char block_released_1e2;
    unsigned char gravity_1e3;
    unsigned char align_camera_1e4;
    unsigned char explode_ground_1e5;
    unsigned char align_explosion_1e6;
    unsigned char flag_1e7;
    void* value_1e8;
    void* value_1ec;
    float lifetime_1f0;
    int flags_1f4;
    float duration_1f8;
    W8SpellEffectDefinition definition_1fc;
    W8TargetSource m_Source;
    W8CombatSlot combat_slot_260;
    /* 0x280: the damage and condition results this missile has accumulated,
       folded into the owning spell effect by 0x00500460. */
    W8SpellEffectResult result_280;
    bool retargeted_322; /* the missile struck something other than its intended target */
    unsigned char unknown_323[5];
};

static_assert(sizeof(W8Missile) == 0x328, "W8Missile_size_must_be_0x328");
/* Secondary vftable 0x005ecdf4 keeps the W8Navigator subobject at +0x18. */
W8_ASSERT_BASE_OFFSET(W8Missile, W8Navigator, padding_004, 0x18);

W8Missile* FireMissile004A2D30(unsigned int missile_table_index, srVector3T<float>* source,
                               srVector3T<float>* target, float value_4, unsigned int value_5,
                               unsigned int value_6, float speed);
void DestroyMissile(W8Missile* missile); /* 0x004A4180 */
/* The world position `character_index`'s current hand fires a missile from:
   the camera's launch point swung to the wielding side. */
void GetCharacterProjectilePosition004A57B0(unsigned int character_index,
                                            srVector3T<float>* position);
void DestroyAllMissiles(W8World* world); /* 0x004A4210 */

extern unsigned int g_missile_table_count_65bddc;

/* One 0x1e5-byte MissileTables.dbs runtime row. Only the fields reached by
   recovered consumers are named. */
#pragma pack(push, 1)
struct W8MissileTableRecord {
    unsigned char unknown_000[0x100];
    /* 0x100: the GrCycle resource name the launcher loads through the
       "Data\\Missiles" script path. */
    char cycle_name_100[0x40];
    float radius_140;    /* 0x140: replaces the launched effect's radius */
    int attack_mode_144; /* 0x144: the attack mode the hit is resolved with */
    unsigned char unknown_148[8];
    /* 0x150: copied into the launched effect block's magnitude_base_1c. */
    int magnitude_base_150;
    /* 0x154: nonzero marks a spell missile - its hits resolve through
       ResolveSpellMissileHit instead of the physical hit/deflect path. */
    bool spell_missile_154;
    /* 0x155: the percentage chances the missile's hit effect assigns each
       condition; CastSpellFromSource copies them into its effect block. */
    unsigned char condition_chances_155[0x10];
    /* 0x165: the missile weapon's impact sound class; MakePCHitSound bounds it
       against the 28 material-impact rows. */
    int weapon_sound_class_165;
    unsigned char unknown_169[0x7c];
};
#pragma pack(pop)

static_assert(sizeof(W8MissileTableRecord) == 0x1e5, "W8MissileTableRecord_must_be_0x1e5");

extern W8MissileTableRecord* g_missile_table_65bde0;

W8Missile* NextMissile004A2760(char restart);

W8Missile* AllocateMissile004A5450(int missile_table_index);
unsigned char LoadMissileCycle004A3550(W8GrCycleLoadContext* context, const char* name,
                                       W8Missile** ppMissile, int unused);

W8Missile* CreateMissile004A28D0(unsigned int missile_table_index, srVector3T<float>* source,
                                 float value_3, float value_4, float value_5, unsigned int value_6,
                                 unsigned char value_7, float speed);
void UpdateWorldMissiles004A27C0(W8World* world);
