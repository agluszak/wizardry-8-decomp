#ifndef WIZ8_ENGINE_CODE_SPELL_EMITTER_HOST_H
#define WIZ8_ENGINE_CODE_SPELL_EMITTER_HOST_H

#include "wiz8/engine_code/Emitter.h"
#include "wiz8/vector_005ece60.h"

#pragma pack(push, 1)

/* Engine Code\Spells.cpp's concrete five-slot emitter host.  The allocation
   in clone slot 0x004ADE70 and the constructor/destructor pair at 0x004AAD20
   and 0x004AB1C0 prove the complete 0x37c-byte extent. */
class W8SpellEmitterHost : public W8EmitterHost {
public:
    W8SpellEmitterHost(const W8SpellEmitterHost& other);
    virtual ~W8SpellEmitterHost() override;
    virtual W8AnimRepBase005EC1D8* Clone() override;
    virtual void SetCycleFrameLod(
        signed char cycle, int frame, int lod) override;
    virtual void ApplyEmitterSetting(char emitter) override;
    virtual void StopEmitter(char emitter) override;

    unsigned char unknown_0ac[0x2c];
    W8Emitter* emitters[28];              /* 0x0d8 */
    float emitter_values[28];             /* 0x148 */
    W8GrowableVector<W8VectorElement005ECE60*> light_lists[28]; /* 0x1b8 */
    unsigned char flag_378;
    unsigned char unknown_379[3];
};                                        /* 0x37c */

static_assert(sizeof(W8SpellEmitterHost) == 0x37c, "W8SpellEmitterHost_size_must_be_0x37c");

#pragma pack(pop)

#endif
