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
   path record is kind 0). UpdateMissileAI ticks it: +0x04 is the
   per-step distance scale, +0x08 the remaining turn budget, +0x0c the missile
   it steers (CopyAIMissile deliberately leaves it unset), +0x10 the
   half-tick baseline, +0x14 the elapsed flight clock, +0x18 the early-impact
   limit and +0x1c a trailing flag. */
struct W8AIMissile : W8AIRecord {
    /* Gravity latch copied from the missile's gravity_1e3: when set, the
       vertical fall rate decays each step. */
    bool gravity;
    unsigned char padding_02[2];
    /* Per-step advance scale (speed units per step tick). */
    float speed_per_step;
    /* Current vertical fall rate, seeded from launch pitch and decayed by
       gravity. */
    float fall_speed;
    W8Missile* missile_0c;
    /* Half-tick baseline (getMsTime()>>1) the delta against the current
       half-tick is clamped to 0xfa. */
    int last_half_tick;
    float elapsed;
    float limit;
    bool unknown_1c;
    unsigned char padding_1d[3];
};

W8AIMissile* CopyAIMissile(const W8AIMissile* source);
unsigned char UpdateMissileAI(W8AIMissile* record);
float AdvanceMissileAI(W8AIMissile* record, srVector3T<float>* out, unsigned int steps);

/* The missile constructor allocates this complete 0x108-byte representation,
   invokes W8EmitterHost on the same receiver, constructs the two light-list
   vectors, and installs vtable 0x005ECDE0. Its copy constructor and destructor
   at 0x004A2DB0 and 0x004A3230 own the same storage. */
class W8MissileRep : public W8EmitterHost {
public:
    W8MissileRep();
    W8MissileRep(const W8MissileRep& other);
    virtual ~W8MissileRep() override;
    virtual W8AnimRepBase* Clone() override;
    virtual srModelInstance* SetCycleFrameLod(signed char cycle, signed char frame,
                                              signed char lod) override;
    virtual unsigned int ApplyEmitterSetting(signed char emitter) override;
    virtual W8AniMesh* GetEmitterAniMesh(signed char emitter) override;
    unsigned char ReadCycleData(W8ReadLevelInfo* info, W8Missile* missile, int cycle_index,
                                int positional_3);

    unsigned int value_0ac;
    unsigned int value_0b0;
    unsigned char unknown_0b4[0x24];
    W8AnimObj* emitters[2];
    float emitter_playback_scales[2];
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
       in combat_slot, roll the target's missile deflection and either hand
       the hit to Combat Attack.cpp or leave it for the combat engine. */
    virtual bool OnCollision(W8Navigator* other) override; /* 0x004A4720 */

    unsigned long GetAnimationState(int mode);
    void DetonateMissileSpell();
    void DestroyMissile(); /* 0x004A4180 */
    void AnnounceCollisionTarget(); /* 0x004A4AC0 */
    /* Switch the representation to its impact cycle, or end the flight when the
       missile has no such cycle. */
    void EnterImpactCycle(); /* 0x004A4C20 */

    void SetEffectDefinition(const W8SpellEffectDefinition* definition); /* 0x004A5410 */
    /* 0x004A5790: true while this in-flight missile still blocks ending combat. */
    bool BlocksEndingCombat();

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
    bool flight_done;
    /* Set by EnterImpactCycle while the impact animation plays. */
    bool impacting;
    /* Set once the missile no longer blocks combat end (BlocksEndingCombat
       returned 0 or combat already resolved). */
    unsigned char block_released_1e2;
    bool gravity_1e3;
    bool align_camera_1e4;
    bool explode_ground_1e5;
    bool align_explosion_1e6;
    bool flag;
    void* value_1e8;
    void* value_1ec;
    float lifetime;
    int flags_1f4;
    float duration_1f8;
    W8SpellEffectDefinition definition_1fc;
    W8TargetSource m_Source;
    W8CombatSlot combat_slot;
    /* 0x280: the damage and condition results this missile has accumulated,
       folded into the owning spell effect by 0x00500460. */
    W8SpellEffectResult result;
    bool retargeted; /* the missile struck something other than its intended target */
    unsigned char padding_323[5];
};

static_assert(sizeof(W8Missile) == 0x328, "W8Missile_size_must_be_0x328");
/* Secondary vftable 0x005ecdf4 keeps the W8Navigator subobject at +0x18. */
W8_ASSERT_BASE_OFFSET(W8Missile, W8Navigator, navigation_mode_008, 0x18);

W8Missile* FireMissile(unsigned int missile_table_index, srVector3T<float>* source,
                       srVector3T<float>* target, float flight_speed, unsigned int trace_mask,
                       unsigned int block_released, float duration);
/* The world position `character_index`'s current hand fires a missile from:
   the camera's launch point swung to the wielding side. */
void GetCharacterProjectilePosition(unsigned int character_index, srVector3T<float>* position);
void DestroyAllMissiles(W8World* world); /* 0x004A4210 */

extern unsigned int g_missile_table_count;

/* One 0x1e5-byte MissileTables.dbs runtime row. Only the fields reached by
   recovered consumers are named. */
#pragma pack(push, 1)
struct W8MissileTableRecord {
    /* The retained database row starts with a UTF-16 name, independently
       visible in all 36 canonical MissileTables.dbs rows. */
    wchar_t display_name[128];
    /* 0x100: the GrCycle resource name the launcher loads through the
       "Data\\Missiles" script path. */
    char cycle_name[0x40];
    float radius;    /* 0x140: replaces the launched effect's radius */
    int attack_mode; /* 0x144: the attack mode the hit is resolved with */
    unsigned char unknown_148[8];
    /* 0x150: copied into the launched effect block's magnitude_base. */
    int magnitude_base;
    /* 0x154: nonzero marks a spell missile - its hits resolve through
       ResolveSpellMissileHit instead of the physical hit/deflect path. */
    bool spell_missile;
    /* 0x155: the percentage chances the missile's hit effect assigns each
       condition; CastSpellFromSource copies them into its effect block. */
    unsigned char condition_chances[0x10];
    /* 0x165: the missile weapon's impact sound class; MakePCHitSound bounds it
       against the 28 material-impact rows. */
    int weapon_sound_class;
    unsigned char unknown_169[0x7c];
};
#pragma pack(pop)

static_assert(offsetof(W8MissileTableRecord, cycle_name) == 0x100,
              "W8MissileTableRecord_cycle_name_offset");
static_assert(sizeof(W8MissileTableRecord) == 0x1e5, "W8MissileTableRecord_must_be_0x1e5");

extern W8MissileTableRecord* g_missile_table;

W8Missile* NextMissile(bool restart);

W8Missile* AllocateMissile(int missile_table_index);
unsigned char LoadMissileCycle(W8GrCycleLoadContext* context, const char* name,
                               W8Missile** ppMissile, int unused);

W8Missile* CreateMissile(unsigned int missile_table_index, srVector3T<float>* source, float heading,
                         float pitch, float flight_speed, unsigned int trace_mask,
                         unsigned char block_released, float duration);
void UpdateWorldMissiles(W8World* world);
