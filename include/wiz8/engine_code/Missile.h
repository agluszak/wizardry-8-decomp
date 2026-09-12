#pragma once

unsigned char LoadMissileDatabase(void);
void ReleaseMissileDatabase(void);

#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/spell_effect.h"
#include "wiz8/targeting.h"

struct W8AIMissile;
struct W8ReadLevelInfo;
class stLight;
class W8Missile;

W8AIMissile* CopyAIMissile004A53A0(const W8AIMissile* source);

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

    unsigned long GetAnimationState004A4640(int mode);
    void Function4A49E0();

    void SetLaunchValues004A5410(const float* values);
    /* 0x004A5790: true while this in-flight missile still blocks ending combat. */
    unsigned char BlocksEndingCombat004A5790();

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
    unsigned char flag_1e0;
    unsigned char flag_1e1;
    unsigned char flag_1e2;
    unsigned char flag_1e3;
    unsigned char flag_1e4;
    unsigned char flag_1e5;
    unsigned char flag_1e6;
    unsigned char flag_1e7;
    void* value_1e8;
    void* value_1ec;
    float lifetime_1f0;
    int value_1f4;
    unsigned char unknown_1f8[4];
    float values_1fc[12];
    unsigned char unknown_22c[0x34];
    W8CombatSlot combat_slot_260;
    /* 0x280: the damage this missile has dealt, folded into the owning spell
       effect by 0x00500460. */
    W8SpellEffectResult result_280;
    unsigned char unknown_2e8[0x3a];
    unsigned char flag_322;
    unsigned char unknown_323[5];
};

static_assert(sizeof(W8Missile) == 0x328, "W8Missile_size_must_be_0x328");

W8Missile* FireMissile004A2D30(unsigned int missile_table_index, srVector3T<float>* source,
                               srVector3T<float>* target, unsigned int value_4,
                               unsigned int value_5, unsigned int value_6, unsigned int value_7);
void DestroyMissile(W8Missile* missile); /* 0x004A4180 */
void DestroyAllMissiles(W8World* world); /* 0x004A4210 */
void DetachMissileReferences005019A0(W8Missile* missile);

extern float g_navigator_largest_extent_6081e8;
extern unsigned int g_missile_table_count_65bddc;

/* One 0x1e5-byte MissileTables.dbs runtime row. Only the fields reached by
   recovered consumers are named. */
#pragma pack(push, 1)
struct W8MissileTableRecord {
    unsigned char unknown_000[0x140];
    float value_140;
    unsigned char unknown_144[0x10];
    unsigned char flag_154; /* 0x154: blocks ending combat while set */
    unsigned char unknown_155[0x90];
};
#pragma pack(pop)

static_assert(sizeof(W8MissileTableRecord) == 0x1e5, "W8MissileTableRecord_must_be_0x1e5");

extern W8MissileTableRecord* g_missile_table_65bde0;

W8Missile* NextMissile004A2760(char restart);

W8Missile* CreateMissile004A28D0(unsigned int missile_table_index, srVector3T<float>* source,
                                 float value_3, float value_4, unsigned int value_5,
                                 unsigned int value_6, unsigned int value_7, unsigned int value_8);
