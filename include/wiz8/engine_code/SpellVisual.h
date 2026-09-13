#pragma once

#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/SpellEmitterHost.h"

/* Engine Code\Spells.cpp's 0x1f8-byte GrCycle specialization.  Construction
   installs the primary 0x005ECF40 table and the ordinary W8Navigator secondary
   table at +0x18, then owns one 0x37c-byte emitter host at +0x1e0. */
class W8SpellVisual : public W8GrCycle {
public:
    W8SpellVisual();
    /* Cloned from a registered shared visual: keeps the cycle type, the saved
       flags and the idle pacing, restarts the transient state, and gets its
       own emitter host and identity counter. Inlined at the 0x004AB580 clone
       site; no out-of-line copy constructor exists in the image. */
    W8SpellVisual(const W8SpellVisual& other);
    virtual ~W8SpellVisual() override;

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

    virtual void StartIfHostActive(); /* 0x004ABDC0 */
    /* Search backward from a subcycle for the first cycle this visual
       supports; returns -1 when none does. */
    virtual int FindSupportedCycle004AC530(signed char group, signed char subcycle);

    int value_1d8;
    int target_location_id_1dc;
    W8SpellEmitterHost* host; /* 0x1e0 */
    unsigned char started;    /* 0x1e4 */
    unsigned char flag_1e5;
    unsigned char flag_1e6;
    unsigned char flag_1e7;
    float value_1e8;
    int value_1ec;
    int value_1f0; /* 0x1f0 */
    int value_1f4; /* 0x1f4 */
};

static_assert(sizeof(W8SpellVisual) == 0x1f8, "W8SpellVisual_size_must_be_0x1f8");

inline W8SpellVisual::W8SpellVisual(const W8SpellVisual& other) : W8GrCycle(other)
{
    value_1d8 = other.value_1d8;
    started = 0;
    flag_1e5 = 0;
    flag_1e6 = other.flag_1e6;
    flag_1e7 = other.flag_1e7;
    value_1e8 = other.value_1e8;
    value_1ec = 0;
    host = static_cast<W8SpellEmitterHost*>(other.host->Clone());
    unknown_008 = IncrementValue60DFAC();
}

void DestroyAllSpellVisuals(W8World* world); /* 0x004AC3D0 */
