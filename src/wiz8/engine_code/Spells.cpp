/*
 * Engine Code\Spells.cpp.
 *
 * A spell's visual side. It hangs off the same emitter record a missile does,
 * one offset along, and the four accessors below are the missile's four
 * bodies with 0x1e0 in place of 0x1dc.
 */

#include "wiz8/float_constants.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/SpellEmitterHost.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

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
extern void ReleaseSoundHandle00408F70(int handle);
extern void GetPosition421070(W8Position* position);
extern unsigned char IsSoundHandleActive00408EF0(int handle);
extern unsigned char g_master_ambient_volume_6850f6;
extern const double g_zero_005ebb40;

W8GrowableVector<stSound3D*> g_sound3d_instances_65be40;

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
    return emitter->values_18[this->host->m_bLOD];
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
    AnimObjValue004A15D0(Method34(), this->host->m_bLOD);
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
    signed char emitter, signed char frame, signed char lod)
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
    return AnimObjValue004A15D0((W8AnimObj*)target, this->m_bLOD);
}

/* One named emitter's AniMesh, looked up with the host's own setting; an
   empty slot yields none. */
// FUNCTION: WIZ8 0x004ab310
W8AniMesh* W8SpellEmitterHost::GetEmitterAniMesh(char emitter)
{
    W8Emitter* target = this->emitters[emitter];

    if (target == 0) {
        return 0;
    }
    return (W8AniMesh*)AnimObjEntry004A1660(
        (W8AnimObj*)target, 0, this->m_bLOD, 0);
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

// VTABLE: WIZ8 0x005ecfb0
// class stSound3D

// VTABLE: WIZ8 0x005ecfe4
// class srClassSupport<stSound3D,srNode,0,65547>

// SYNTHETIC: WIZ8 0x004AEA70
// stSound3D::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004AF3D0
// srClassSupport<stSound3D,srNode,0,65547>::getClassID

// TEMPLATE: WIZ8 0x004AF3E0
// srClassSupport<stSound3D,srNode,0,65547>::getClassName

// TEMPLATE: WIZ8 0x004AF3F0
// srClassSupport<stSound3D,srNode,0,65547>::getClassNode

// TEMPLATE: WIZ8 0x004AF460
// srClassSupport<stSound3D,srNode,0,65547>::clone

// TEMPLATE: WIZ8 0x004AF5A0
// srClassSupport<stSound3D,srNode,0,65547>::~srClassSupport<stSound3D,srNode,0,65547>

// SYNTHETIC: WIZ8 0x004AF660
// srClassSupport<stSound3D,srNode,0,65547>::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004AE6D0
stSound3D::stSound3D(const char* name, srNode* parent)
    : srClassSupport<stSound3D, srNode, 0, 0x1000b>(
          static_cast<srNode*>(0)),
      unknown_138(0),
      sound_handle_13c(-1),
      value_140(0x7f),
      value_144(25000.0f),
      sound_name_148(0),
      flag_14c(0)
{
    if (parent != 0) {
        setParent(parent, 0);
    }
    if (name != 0) {
        sound_name_148 = static_cast<char*>(malloc(strlen(name) + 1));
        strcpy(sound_name_148, name);
    }
    g_sound3d_instances_65be40.Add(this);
}

// FUNCTION: WIZ8 0x004AEAA0
stSound3D::~stSound3D()
{
    if (sound_name_148 != 0) {
        free(sound_name_148);
    }
    if (sound_handle_13c != -1) {
        ReleaseSoundHandle00408F70(sound_handle_13c);
    }
    int index = g_sound3d_instances_65be40.IndexOf(this);
    if (index != -1) {
        g_sound3d_instances_65be40.RemoveAt(index);
    }
}

// FUNCTION: WIZ8 0x004AE8B0
srClass* stSound3D::vInstance()
{
    return new stSound3D(0, 0);
}

// FUNCTION: WIZ8 0x004AEBF0
unsigned char stSound3D::Play004AEBF0(
    unsigned char flatten, unsigned char flag)
{
    SOUND3DPARMS options;
    W8Position listener;

    if (sound_name_148 == 0) {
        return 0;
    }
    GetPosition421070(&listener);
    BuildSoundOptions004AECC0(&listener, &options);
    if (flatten != 0) {
        options.uiLoop = 0;
    }
    sound_handle_13c = Sound3DPlay(sound_name_148, &options);
    flag_14c = flag;
    return sound_handle_13c != -1;
}

// FUNCTION: WIZ8 0x004AECC0
void stSound3D::BuildSoundOptions004AECC0(
    const W8Position* listener, SOUND3DPARMS* options)
{
    float angle = -GetCameraAngleRadians420DD0();
    unsigned int volume =
        (value_140 * g_master_ambient_volume_6850f6) / 0x7f;
    srMatrix3T<float> rotation;
    srVector3T<float> node_position;
    srVector3T<float> offset;

    rotation.vectors[0].method_00421680(1.0, 0.0, 0.0);
    rotation.vectors[1].method_00421680(0.0, 1.0, 0.0);
    rotation.vectors[2].method_00421680(0.0, 0.0, 1.0);
    if ((double)angle != g_zero_005ebb40) {
        double cosine = cos(angle);
        double sine = sin(angle);
        srVector3T<float> first;
        srVector3T<float> second;
        srVector3T<float> third;
        srMatrix3T<float> camera_rotation;

        first.method_00421680(cosine, 0.0, sine);
        second.method_00421680(0.0, 1.0, 0.0);
        third.method_00421680(-sine, 0.0, cosine);
        camera_rotation.method_004219F0(first, second, third);
        rotation.method_00421A40(camera_rotation);
    }

    getLocation(node_position);
    offset.method_00421650(
        node_position.x - listener->x,
        node_position.y - listener->y,
        node_position.z - listener->z);
    float x = Function4218E0(rotation.vectors[0], offset);
    float y = Function4218E0(rotation.vectors[1], offset);
    float z = Function4218E0(rotation.vectors[2], offset);

    memset(options, 0xff, sizeof(*options));
    srVector3T<float> listener_offset;
    listener_offset.method_00421650(
        listener->x - node_position.x,
        listener->y - node_position.y,
        listener->z - node_position.z);
    options->uiVolume = static_cast<unsigned int>(
        (g_float_005ebb38 -
         listener_offset.method_00421700() / value_144) * volume);
    options->uiLoop = 1;
    options->Pos.flX = x;
    options->Pos.flY = y;
    options->Pos.flZ = z;
    options->Pos.flVelX = 0.0f;
    options->Pos.flVelY = 0.0f;
    options->Pos.flVelZ = 0.0f;
    options->Pos.flFaceX = -x;
    options->Pos.flFaceY = -y;
    options->Pos.flFaceZ = -z;
    options->Pos.flUpX = 0.0f;
    options->Pos.flUpY = g_float_005ebb38;
    options->Pos.flUpZ = 0.0f;
    options->Pos.flFalloffMin = value_144;
    options->Pos.flFalloffMax = value_144;
    options->Pos.uiVolume = options->uiVolume;
}

// FUNCTION: WIZ8 0x004AEC70
unsigned char stSound3D::IsPlaying004AEC70()
{
    if (sound_handle_13c != -1 &&
        IsSoundHandleActive00408EF0(sound_handle_13c) != 0) {
        return 1;
    }
    return 0;
}
