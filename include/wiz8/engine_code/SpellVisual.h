#pragma once

#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/SpellEmitterHost.h"

/* Engine Code\Spells.cpp's 0x1f8-byte GrCycle specialization.  Construction
   installs the primary 0x005ECF40 table and the ordinary W8Navigator secondary
   table at +0x18, then owns one 0x37c-byte emitter host at +0x1e0. */
class W8SpellVisual : public W8GrCycle {
public:
    W8SpellVisual();
    virtual ~W8SpellVisual() override;

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

    void StartIfHostActive();            /* 0x004ABDC0 */

    int value_1d8;
    int target_location_id_1dc;
    W8SpellEmitterHost* host;            /* 0x1e0 */
    unsigned char started;               /* 0x1e4 */
    unsigned char flag_1e5;
    unsigned char flag_1e6;
    unsigned char flag_1e7;
    float value_1e8;
    int value_1ec;
    unsigned char unknown_1f0[8];
};

static_assert(sizeof(W8SpellVisual) == 0x1f8,
              "W8SpellVisual_size_must_be_0x1f8");

void DestroyAllSpellVisuals(W8World* world);        /* 0x004AC3D0 */
