#pragma once

#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/targeting.h"

struct W8AIMissile;
struct W8GrCycleReadInfo004A6970;
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
    virtual srModelInstance* SetCycleFrameLod(
        signed char cycle, signed char frame, signed char lod) override;
    virtual unsigned int ApplyEmitterSetting(char emitter) override;
    virtual W8AniMesh* GetEmitterAniMesh(char emitter) override;
    unsigned char ReadCycleData004A3300(
        W8GrCycleReadInfo004A6970* info,
        W8Missile* missile,
        int positional_2,
        int emitter_index);

    unsigned char unknown_0ac[0x2c];
    W8AnimObj* emitters[2];
    float emitter_values[2];
    W8GrowableVector<W8LightVector*> light_lists[2];
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
    virtual unsigned char IsCycleSupported(signed char cycle) override;
    virtual signed char GetTotalAnimationCount() override;
    virtual float GetCurrentAnimationScale() override;
    virtual W8EmitterHost* GetRepresentation() override;
    virtual void SetCycle(signed char cycle) override;
    virtual W8AnimObj* GetCurrentAnimation() override;
    virtual void AdvanceAnimationFrame(int value, int flags) override;
    virtual W8AniMesh* GetCurrentAniMesh() override;

    void Function4A5410(const float* values);

public:
    int missile_table_index_1d8;
    W8MissileRep* m_pRep;
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
    unsigned char unknown_280[0x58];
    W8GrowableVector<void*> vector_2d8;
    unsigned char unknown_2e8[0x3a];
    unsigned char flag_322;
    unsigned char unknown_323[5];
};

static_assert(sizeof(W8Missile) == 0x328, "W8Missile_size_must_be_0x328");

W8Missile* Function4A2D30(
    unsigned int missile_table_index, srVector3T<float>* source,
    srVector3T<float>* target, unsigned int value_4,
    unsigned int value_5, unsigned int value_6,
    unsigned int value_7);
