#ifndef WIZ8_ENGINE_CODE_SPELL_EMITTER_HOST_H
#define WIZ8_ENGINE_CODE_SPELL_EMITTER_HOST_H

#include "wiz8/engine_code/Emitter.h"
#include "wiz8/vector.h"

struct W8ReadLevelInfo;
class stLight;
class W8SpellVisual;

/* The 28 bitmap cycles held by one spell emitter host: four visual groups of
   seven power-level cycles each. */
enum {
    SPELL_CYCLE_FIRST = 0,
    SPELL_CYCLE_LAST = 27,
    SPELL_NUM_CYCLES = 28,
    SPELL_CYCLES_PER_GROUP = 7
};

/* Engine Code\Spells.cpp's concrete five-slot emitter host.  The allocation
   in clone slot 0x004ADE70 and the constructor/destructor pair at 0x004AAD20
   and 0x004AB1C0 prove the complete 0x37c-byte extent. */
class W8SpellEmitterHost : public W8EmitterHost {
public:
    /* Retail has no standalone default-constructor body: W8SpellVisual expands
       this initialization at its only construction site. */
    W8SpellEmitterHost() : value_0ac(0), value_0b0(0), billboard(0)
    {
        for (int emitter = 0; emitter < SPELL_NUM_CYCLES; ++emitter) {
            emitters[emitter] = 0;
            emitter_playback_scales[emitter] = 15.0f;
        }
    }
    W8SpellEmitterHost(const W8SpellEmitterHost& other);
    virtual ~W8SpellEmitterHost() override;
    virtual W8AnimRepBase* Clone() override;
    virtual srModelInstance* SetCycleFrameLod(signed char cycle, signed char frame,
                                              signed char lod) override;
    virtual unsigned int ApplyEmitterSetting(signed char emitter) override;
    virtual W8AniMesh* GetEmitterAniMesh(signed char emitter) override;
    unsigned char ReadCycleData(W8ReadLevelInfo* info, W8SpellVisual* visual, int,
                                int emitter_index);

    unsigned int value_0ac;
    unsigned int value_0b0;
    unsigned char unknown_0b4[0x24];
    W8AnimObj* emitters[SPELL_NUM_CYCLES];                                       /* 0x0d8 */
    float emitter_playback_scales[SPELL_NUM_CYCLES];                             /* 0x148 */
    W8GrowableVector<W8GrowableVector<stLight*>*> light_lists[SPELL_NUM_CYCLES]; /* 0x1b8 */
    /* Set for spawned visuals: UpdateRepresentation then adds the
       camera-facing yaw rotation on top of the mode's orientation. */
    bool billboard;
    unsigned char padding_379[3];
}; /* 0x37c */

static_assert(sizeof(W8SpellEmitterHost) == 0x37c, "W8SpellEmitterHost_size_must_be_0x37c");

#endif
