/*
 * Engine Code\Spells.cpp.
 *
 * A spell's visual side. It hangs off the same emitter record a missile does,
 * one offset along, and the four accessors below are the missile's four
 * bodies with 0x1e0 in place of 0x1dc.
 */

#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/SpellEmitterHost.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

class W8SpellVisual {
public:
    virtual ~W8SpellVisual();
    virtual W8AnimObj* Method34();

    W8EmitterHost* GetHost();            /* 0x004AC890 */
    W8Emitter* GetActiveEmitter();       /* 0x004AC820 */
    float GetActiveEmitterValue();       /* 0x004AC870 */
    char GetEmitterCount();              /* 0x004AC840 */
    void ApplyHostSetting98();           /* 0x004AC360 */
    void StartIfHostActive();            /* 0x004ABDC0 */
    void* GetActiveEmitterEntry004AC8A0();

    unsigned char unknown_004[0x1dc];
    W8SpellEmitterHost* host;            /* 0x1e0 */
    unsigned char started;               /* 0x1e4 */
};

extern void DestroyEmitter(W8Emitter* emitter);                         /* 0x004A01E0 */
extern void Function4A6E20(float value);
extern int CountSpellsOfKind(int kind);                      /* 0x004AC8F0 */

/* The emitter record a spell's visual hangs off. */
// FUNCTION: WIZ8 0x004ac890
W8EmitterHost* W8SpellVisual::GetHost()
{
    return this->host;
}

/* The emitter it is currently coming out of. */
// FUNCTION: WIZ8 0x004ac820
W8Emitter* W8SpellVisual::GetActiveEmitter()
{
    return this->host->emitters[this->host->selection.emitter.emitter_index];
}

/* That emitter's own value. */
// FUNCTION: WIZ8 0x004ac870
float W8SpellVisual::GetActiveEmitterValue()
{
    return this->host->emitters[this->host->selection.emitter.emitter_index]->value_08;
}

// FUNCTION: WIZ8 0x004ac8a0
void* W8SpellVisual::GetActiveEmitterEntry004AC8A0()
{
    W8Emitter* emitter = this->host->emitters[
        this->host->selection.emitter.emitter_index];

    if (emitter == 0) {
        srAssertFail("pao", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x653, 0);
    }
    return emitter->values_18[this->host->setting_98];
}

/* How many emitters the host has, counted by testing each for null. */
// FUNCTION: WIZ8 0x004ac840
char W8SpellVisual::GetEmitterCount()
{
    char count = this->host->emitters[0] != 0;

    if (this->host->emitters[1] != 0) {
        ++count;
    }
    return count;
}

/* Hand the host's setting to whatever the visual's own virtual accessor
   answers with - the missile's body one offset along. */
// FUNCTION: WIZ8 0x004ac360
void W8SpellVisual::ApplyHostSetting98()
{
    AnimObjValue004A15D0(Method34(), this->host->setting_98);
}

/* Start the visual, but only while the host is live. A spell of the seventh
   kind already running takes the slot instead of a fresh start. */
// FUNCTION: WIZ8 0x004abdc0
void W8SpellVisual::StartIfHostActive()
{
    if (this->host->active == 0) {
        return;
    }
    if (CountSpellsOfKind(7) != 0) {
        this->started = 1;
        return;
    }
    Function4A6E20(1.0f);
}

/* Send something to one named emitter. The arguments are handed on in the
   reverse of the order they arrive. */
// FUNCTION: WIZ8 0x004ab290
srModelInstance* W8SpellEmitterHost::SetCycleFrameLod(
    signed char emitter, int frame, int lod)
{
    return AnimObjDispatch004A14D0(
        (W8AnimObj*)this->emitters[emitter], (signed char)lod, frame);
}

/* Apply the host setting to one required emitter.  The source assertion names
   that local pointer `pao`; assertions do not replace the following call. */
// FUNCTION: WIZ8 0x004ab2c0
unsigned int W8SpellEmitterHost::ApplyEmitterSetting(char emitter)
{
    W8Emitter* target = this->emitters[emitter];

    if (target == 0) {
        srAssertFail(
            "pao",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
            0x200,
            0);
    }
    return AnimObjValue004A15D0((W8AnimObj*)target, this->setting_98);
}

/* Stop one named emitter, passing the host's own setting; an empty slot is
   left alone. */
// FUNCTION: WIZ8 0x004ab310
void W8SpellEmitterHost::StopEmitter(char emitter)
{
    W8Emitter* target = this->emitters[emitter];

    if (target == 0) {
        return;
    }
    AnimObjEntry004A1660((W8AnimObj*)target, 0, this->setting_98, 0);
}

/* The clone slot owns both the 0x37c allocation and the copy-construction
   call.  The constructor body remains the next lifecycle member to recover. */
// FUNCTION: WIZ8 0x004ade70
W8AnimRepBase005EC1D8* W8SpellEmitterHost::Clone()
{
    return new W8SpellEmitterHost(*this);
}

/* Release each owned emitter and every per-emitter vector of cloned lights.
   The member-array and base destructors then run in reverse construction
   order, matching the two vector/base cleanup phases in the image. */
// SYNTHETIC: WIZ8 0x004aad00
// W8SpellEmitterHost::`scalar deleting destructor'
// FUNCTION: WIZ8 0x004ab1c0
W8SpellEmitterHost::~W8SpellEmitterHost()
{
    int emitter;
    int light_list;

    for (emitter = 0; emitter < 28; ++emitter) {
        if (emitters[emitter] != 0) {
            DestroyEmitter(emitters[emitter]);
            emitters[emitter] = 0;
        }
    }
    for (emitter = 0; emitter < 28; ++emitter) {
        for (light_list = 0; light_list < light_lists[emitter].GetCount();
             ++light_list) {
            DestroyLightVector(*light_lists[emitter].GetAt(light_list));
        }
        light_lists[emitter].Clear();
    }
}

/* Whether one spell id is among the six the caller singles out. */
// FUNCTION: WIZ8 0x004aca00
bool IsSpellInSingledOutSet(int spell_id)
{
    switch (spell_id) {
    case 0x30:
    case 0x31:
    case 0x4c:
    case 0x50:
    case 0x51:
    case 0x5d:
        return true;
    default:
        return false;
    }
}

/* Release the spell database and forget the version with it, so the two are
   never out of step. */
// FUNCTION: WIZ8 0x004acd50
void ReleaseSpellDatabase(void)
{
    if (g_spell_records != 0) {
        operator delete(g_spell_records);
        g_spell_records = 0;
        g_spell_database_version = 0;
    }
}

// SYNTHETIC: WIZ8 0x004abce0
// SpellObject005ECF40::`scalar deleting destructor'
