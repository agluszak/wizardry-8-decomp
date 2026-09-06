/*
 * Engine Code\Spells.cpp.
 *
 * A spell's visual side. It hangs off the same emitter record a missile does,
 * one offset along, and the four accessors below are the missile's four
 * bodies with 0x1e0 in place of 0x1dc.
 */

#include "wiz8/float_constants.h"
#include "wiz8/magic.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/character.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stLight.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/SpellEmitterHost.h"
#include "wiz8/engine_code/SpellVisual.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/render_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "surrender/srTimer.h"
#include "soundman.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

extern int IncrementValue60DFAC(void);
extern int CountSpellsOfKind(int kind);                      /* 0x004AC8F0 */
extern unsigned char g_master_ambient_volume_6850f6;
extern const float g_monster_rotation_offset_005ec04c;
extern const double g_camera_pi_005ec2a0;
extern float Function4BE420(
    const srVector3T<float>* from, const srVector3T<float>* to);
extern float Function4BE490(
    const srVector3T<float>* from, const srVector3T<float>* to);

W8GrowableVector<stSound3D*> g_sound3d_instances_65be40;

static __inline int MinimumCasterLevel(int spell_level)
{
    switch (spell_level) {
    case 2:
        return 3;
    case 3:
        return 5;
    case 4:
        return 8;
    case 5:
        return 11;
    case 6:
        return 14;
    case 7:
        return 18;
    default:
        return 1;
    }
}

// FUNCTION: WIZ8 0x004ac9d0
int GetSpellTargetType(int spell_id, unsigned char normalize_single_target)
{
    int target_type = g_spell_records[spell_id].target_type;

    if (target_type == 1 && normalize_single_target) {
        target_type = 0;
    }
    return target_type;
}

// FUNCTION: WIZ8 0x004acb40
int MinimumCasterLevelForSpellLevel(int spell_level)
{
    return MinimumCasterLevel(spell_level);
}

// FUNCTION: WIZ8 0x004acba0
int GetMinimumCasterLevelForSpell(int spell_id)
{
    return MinimumCasterLevel(g_spell_records[spell_id].spell_level);
}

/* The emitter record a spell's visual hangs off. */
// FUNCTION: WIZ8 0x004ac890
W8EmitterHost* W8SpellVisual::GetRepresentation()
{
    return this->host;
}

/* The emitter it is currently coming out of. */
// FUNCTION: WIZ8 0x004ac820
W8AnimObj* W8SpellVisual::GetCurrentAnimation()
{
    return this->host->emitters[this->host->current_cycle];
}

/* That emitter's own value. */
// FUNCTION: WIZ8 0x004ac870
float W8SpellVisual::GetCurrentAnimationScale()
{
    return this->host->emitters[
        this->host->current_cycle]->playback_scale_08;
}

// FUNCTION: WIZ8 0x004ac8a0
W8AniMesh* W8SpellVisual::GetCurrentAniMesh()
{
    W8AnimObj* emitter = this->host->emitters[
        this->host->current_cycle];

    if (emitter == 0) {
        srAssertFail("pao", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x653, 0);
    }
    return emitter->entries_18[this->host->m_bLOD];
}

/* How many emitters the host has, counted by testing each for null. */
// FUNCTION: WIZ8 0x004ac840
signed char W8SpellVisual::GetTotalAnimationCount()
{
    char count = 0;

    if (this->host->emitters[0] != 0) {
        count = 1;
    }

    if (this->host->emitters[1] != 0) {
        ++count;
    }
    return count;
}

/* Hand the host's LOD to the current animation. */
// FUNCTION: WIZ8 0x004ac360
signed char W8SpellVisual::GetNumSubCycles()
{
    W8AnimObj* animation = GetCurrentAnimation();

    return static_cast<signed char>(
        AnimObjValue004A15D0(animation, host->m_bLOD));
}

// FUNCTION: WIZ8 0x004ac4e0
unsigned char W8SpellVisual::IsCycleSupported(signed char cycle)
{
    if (cycle >= 28) {
        srAssertFail(
            "bCycle<SPELL_NUM_CYCLES",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
            0x55a,
            0);
    }
    return host->emitters[cycle] != 0;
}

/* Reset the host's two frame counters before the ordinary GrCycle step. */
// FUNCTION: WIZ8 0x004ac390
void W8SpellVisual::AdvanceAnimationFrame(int value, int flags)
{
    W8SpellEmitterHost* representation_before;

    host->counter_094 = 0;
    representation_before = host;
    representation_before->counter_095 = GetNumSubCycles() - 1;
    W8GrCycle::AdvanceAnimationFrame(value, flags);
}

/* Select one of the spell host's 28 emitters and rebuild its light and
   particle attachment state. */
// FUNCTION: WIZ8 0x004ac580
void W8SpellVisual::SetCycle(signed char cycle)
{
    W8GrowableVector<stLight*>* lights;
    W8AnimObj* animation;
    int index;

    if (cycle < 0 || cycle >= 28) {
        srAssertFail(
            "bCycle >= SPELL_CYCLE_FIRST && bCycle <= SPELL_CYCLE_LAST",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
            0x5c1,
            0);
    }

    lights = *host->light_lists[
        host->current_cycle].GetAt(0);
    if (lights != 0) {
        for (index = 0; index < lights->GetCount(); ++index) {
            stLight* light = *lights->GetAt(index);

            light->setParent(0, 1);
            if (light->definition() != 0) {
                int world_index = g_world->lights_to_update->IndexOf(light);
                if (world_index != -1) {
                    g_world->lights_to_update->RemoveAt(world_index);
                }
            }
        }
    }

    host->current_cycle = cycle;
    animation = host->emitters[cycle];
    host->active = 1;
    host->flag_06e = 1;
    if (host->SetCycleFrameLod(cycle, 0, 2) != 0) {
        host->m_bLOD = 2;
    }
    else if (host->SetCycleFrameLod(cycle, 0, 1) != 0) {
        host->m_bLOD = 1;
    }
    else {
        host->m_bLOD = 0;
    }
    host->timer_068 =
        g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    host->flag_06f = animation->value_02;
    host->flag_06d = animation->unknown_00[1];
    host->flag_064 = 0;

    lights = *host->light_lists[cycle].GetAt(0);
    SetLights(lights);
    if (g_render_flag_60a20c != 0 && lights != 0) {
        for (index = 0; index < lights->GetCount(); ++index) {
            stLight* light = *lights->GetAt(index);

            light->setParent(g_world->dynamic_scene, 1);
            if (light->definition() != 0) {
                g_world->lights_to_update->Add(light);
            }
        }
    }

    if (m_plsParticles != 0) {
        for (index = 0; index < m_plsParticles->GetCount(); ++index) {
            W8GrCycleParticleAttachment* event = *m_plsParticles->GetAt(index);

            if (event->cycle_00 == cycle) {
                event->particle_08->SetActive(1);
                event->particle_08->value_188 = 0;
            }
            else {
                event->particle_08->SetActive(0);
            }
        }
    }
}

/* Position and orient each live spell visual according to its attachment
   mode, then let the common GrCycle update submit the resulting state. */
// FUNCTION: WIZ8 0x004abe00
void W8SpellVisual::UpdateRepresentation(W8World* world)
{
    srMatrix3T<float> rotation;
    srVector3T<float> camera_position;
    srVector3T<float> position;
    bool apply_rotation = false;

    if (value_1d8 == 0) {
        float angle;
        float pitch;

        GetCameraPosition(&position);
        SetPosition004A6DF0(&position);
        rotation.SetIdentity00467310();
        angle = GetCameraYawRadians() - g_monster_rotation_offset_005ec04c;
        if ((double)angle != g_zero_005ebb40) {
            rotation.method_00438F90(sin((double)angle), cos((double)angle));
        }
        pitch = -GetCameraPitchRadians();
        if ((double)pitch != g_zero_005ebb40) {
            rotation.method_00478EB0(sin((double)pitch), cos((double)pitch));
        }
        apply_rotation = true;
    }
    else if (value_1d8 == 2) {
        W8Monster* monster = GetMonsterByLocationID(
            target_location_id_1dc);

        if (monster != 0) {
            srModelInstance* instance = GetCurrentModelInstance004A8250();

            while (instance != 0) {
                srVector3T<double> scale(
                    value_1e8, value_1e8, value_1e8);
                instance->setScale(scale);
                instance = static_cast<srModelInstance*>(instance->firstChild());
            }

            srVector3T<float> minimum;
            srVector3T<float> maximum;
            srVector3T<float> monster_position = monster->GetPosition();

            monster->GetAnimationBounds(&minimum, &maximum);
            position.x = monster_position.x;
            position.y = monster_position.y +
                (maximum.y - minimum.y) * g_float_005ebc7c;
            position.z = monster_position.z;
            SetPosition004A6DF0(&position);
        }
    }
    else if (value_1d8 == 3) {
        GetCurrentModelInstance004A8250();
        if (flag_1e7 == 0) {
            GetCameraPosition(&camera_position);
            if (value_1ec == 0) {
                SetPosition004A6DF0(&camera_position);
                g_gd_camera_65a0f8->GetRotationMatrix(&rotation);
                apply_rotation = true;
            }
            else {
                W8Monster* monster = GetMonsterByLocationID(value_1ec);

                if (monster != 0) {
                    if (monster->Query(6) == 0x19 &&
                        monster->GetSpellPosition004C78E0(&position) != 0) {
                        SetPosition004A6DF0(&position);
                    }

                    rotation.SetIdentity00467310();
                    float angle = monster->GetYaw();
                    if ((double)angle != g_zero_005ebb40) {
                        rotation.method_00438F90(
                            sin((double)angle), cos((double)angle));
                    }
                    float pitch = Function4BE490(&position, &camera_position);
                    if ((double)pitch != g_zero_005ebb40) {
                        rotation.method_00478EB0(
                            sin((double)pitch), cos((double)pitch));
                    }
                    apply_rotation = true;
                }
            }
        }
    }

    if (apply_rotation) {
        host->SetRotation004B88D0(&rotation);
    }

    if (host->flag_378 != 0) {
        srMatrix3T<float> billboard;
        srVector3T<float> visual_position = GetPosition();
        float angle;

        billboard.SetIdentity00467310();
        camera_position = g_gd_camera_65a0f8->m_position_08c;
        angle = Function4BE420(&visual_position, &camera_position) +
            (float)g_camera_pi_005ec2a0;
        if ((double)angle != g_zero_005ebb40) {
            billboard.method_00438F90(
                sin((double)angle), cos((double)angle));
        }
        host->SetRotation004B88D0(&billboard);
    }

    srModelInstance* instance = GetCurrentModelInstance004A8250();
    if (instance != 0) {
        static_cast<stModelInstance*>(instance)->state_178 |= 0x10;
    }
    W8GrCycle::UpdateRepresentation(world);
}

W8SpellEmitterHost::W8SpellEmitterHost()
    : value_0ac(0),
      value_0b0(0),
      flag_378(0)
{
    int emitter;

    for (emitter = 0; emitter < 28; ++emitter) {
        emitters[emitter] = 0;
        emitter_values[emitter] = 15.0f;
    }
}

/* Copy the two proven host values and terminal flag, clone every populated
   animation, and deep-copy all optional per-emitter light vectors. */
// FUNCTION: WIZ8 0x004aad20
W8SpellEmitterHost::W8SpellEmitterHost(const W8SpellEmitterHost& other)
    : W8EmitterHost(other),
      value_0ac(other.value_0ac),
      value_0b0(other.value_0b0),
      flag_378(other.flag_378)
{
    int emitter;

    for (emitter = 0; emitter < 28; ++emitter) {
        if (other.emitters[emitter] == 0) {
            emitters[emitter] = 0;
            emitter_values[emitter] = 15.0f;
        }
        else {
            emitters[emitter] = CloneAnimObj004A0320(other.emitters[emitter]);
            emitter_values[emitter] = other.emitter_values[emitter];
        }
    }

    active = 1;
    current_cycle =
        other.current_cycle;

    for (emitter = 0; emitter < 28; ++emitter) {
        int list_index;

        for (list_index = 0;
             list_index < other.light_lists[emitter].GetCount();
             ++list_index) {
            W8GrowableVector<stLight*>* source_lights =
                *other.light_lists[emitter].GetAt(list_index);
            W8GrowableVector<stLight*>* copied_lights = 0;

            if (source_lights != 0) {
                int light_index;

                copied_lights = new W8GrowableVector<stLight*>;
                if (copied_lights == 0) {
                    srAssertFail(
                        "plsNewLights",
                        "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
                        0x198,
                        "Out of memory creating monster light list");
                }
                for (light_index = 0;
                     light_index < source_lights->GetCount();
                     ++light_index) {
                    stLight* source_light =
                        *source_lights->GetAt(light_index);
                    float x = source_light->positionalX();
                    float y = source_light->positionalY();
                    float z = source_light->positionalZ();
                    stLight* copied_light = new stLight;

                    if (copied_light != 0) {
                        *copied_light = *source_light;
                    }
                    if (copied_light == 0) {
                        srAssertFail(
                            "pstNewLight",
                            "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
                            0x1a0,
                            "Out of memory creating monster light");
                    }
                    copied_light->ConfigureMonsterCopy();
                    copied_light->setLocation(x, y, z);
                    copied_light->setParent(0, 0);
                    PLAdoptAppend(&g_world->m_list_0a8, copied_light);
                    copied_lights->Add(copied_light);
                }
            }
            light_lists[emitter].Add(copied_lights);
        }
    }
}

// FUNCTION: WIZ8 0x004AB340
unsigned char W8SpellEmitterHost::ReadCycleData004AB340(
    W8GrCycleReadInfo004A6970* info,
    W8SpellVisual* visual,
    int,
    int emitter_index)
{
    W8GrowableVector<stLight*>* lights = new W8GrowableVector<stLight*>;
    W8AnimObj* animation;
    unsigned char success;
    signed char emitter;

    if (info == 0 || info->handle_04 == 0 || visual == 0) {
        srAssertFail(
            "pInfo && pInfo->hFile && pSpell",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
            0x25a,
            0);
    }
    animation = CreateAnimObj004A01A0();
    success = AnimObjReadFromFile004A05C0(
        reinterpret_cast<W8ReadLevelInfo*>(info), animation, 1, lights, 1);
    emitter = static_cast<signed char>(animation->unknown_03[1]);

    if (lights->GetCount() == 0) {
        delete lights;
        lights = 0;
    }
    else {
        visual->SetLights(lights);
    }
    light_lists[emitter_index].Add(lights);

    if (emitter_index != -1) {
        emitter = static_cast<signed char>(emitter_index);
        current_cycle = emitter;
    }
    emitter_values[emitter] = animation->playback_scale_08;
    active = 1;
    flag_06e = 1;
    timer_068 = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    flag_070 = animation->unknown_03[0];
    flag_06f = animation->value_02;
    flag_06d = animation->unknown_00[1];
    emitters[emitter] = animation;

    if (visual != 0) {
        if (SetCycleFrameLod(current_cycle, 0, 2) != 0) {
            m_bLOD = 2;
        }
        else if (SetCycleFrameLod(current_cycle, 0, 1) != 0) {
            m_bLOD = 1;
        }
        else {
            m_bLOD = 0;
        }
    }
    return success;
}

// FUNCTION: WIZ8 0x004ABBB0
W8SpellVisual::W8SpellVisual()
    : value_1d8(-1),
      host(0),
      started(0),
      flag_1e5(0),
      flag_1e6(1),
      flag_1e7(0),
      value_1e8(1.0f),
      value_1ec(0)
{
    W8GrObject::unknown_004 = 1;
    unknown_008 = IncrementValue60DFAC();
    host = new W8SpellEmitterHost;
    if (host == 0) {
        srAssertFail(
            "m_pRep",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
            0x3c0,
            0);
    }
}

/* Release the emitter host, leave the world's spell collection, and unregister
   the common GrCycle identity before the base classes tear down. */
// FUNCTION: WIZ8 0x004abd00
W8SpellVisual::~W8SpellVisual()
{
    SetLights(0);
    delete host;
    host = 0;

    int index = g_world->spell_visuals->IndexOf(this);
    if (index != -1) {
        g_world->spell_visuals->RemoveAt(index);
    }
    UnregisterGrCycle(this);
}

/* Delete every spell visual owned by a world. Its light list is first handed
   back through the world light boundary, exactly as the retail teardown does. */
// FUNCTION: WIZ8 0x004ac3d0
void DestroyAllSpellVisuals(W8World* world)
{
    while (world->spell_visuals->GetCount() != 0) {
        W8SpellVisual* spell = *world->spell_visuals->GetAt(0);

        if (spell == 0) {
            srAssertFail(
                "pSpell",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
                0x51c,
                0);
        }
        g_world->spell_visuals->RemoveAt(
            g_world->spell_visuals->IndexOf(spell));
        if (spell->m_plsLights != 0) {
            int count = spell->m_plsLights->GetCount();

            while (count != 0) {
                stLight* light = spell->m_plsLights->RemoveAt(0);
                WorldRemoveLight(g_world, light);
                --count;
            }
        }
        delete spell;
    }
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
    TickAnimation(1.0f);
}

/* Send something to one named emitter. The arguments are handed on in the
   reverse of the order they arrive. */
// FUNCTION: WIZ8 0x004ab290
srModelInstance* W8SpellEmitterHost::SetCycleFrameLod(
    signed char emitter, signed char frame, signed char lod)
{
    return AnimObjDispatch004A14D0(
        this->emitters[emitter], (signed char)lod, frame);
}

/* Apply the host setting to one required emitter.  The source assertion names
   that local pointer `pao`; assertions do not replace the following call. */
// FUNCTION: WIZ8 0x004ab2c0
unsigned int W8SpellEmitterHost::ApplyEmitterSetting(char emitter)
{
    W8AnimObj* target = this->emitters[emitter];

    if (target == 0) {
        srAssertFail(
            "pao",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
            0x200,
            0);
    }
    return AnimObjValue004A15D0(target, this->m_bLOD);
}

/* One named emitter's AniMesh, looked up with the host's own setting; an
   empty slot yields none. */
// FUNCTION: WIZ8 0x004ab310
W8AniMesh* W8SpellEmitterHost::GetEmitterAniMesh(char emitter)
{
    W8AnimObj* target = this->emitters[emitter];

    if (target == 0) {
        return 0;
    }
    return (W8AniMesh*)AnimObjEntry004A1660(
        target, this->m_bLOD, 0);
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
            DestroyAnimObj004A01E0(emitters[emitter]);
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
// W8SpellVisual::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ecf40 W8SpellVisual
// VTABLE: WIZ8 0x005ecf2c W8Navigator
// class W8SpellVisual

// VTABLE: WIZ8 0x005ecf18 W8SpellEmitterHost
// class W8SpellEmitterHost

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
        SoundStop(sound_handle_13c);
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
    srVector3T<float> listener;

    if (sound_name_148 == 0) {
        return 0;
    }
    GetCameraPosition(&listener);
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
    const srVector3T<float>* listener, SOUND3DPARMS* options)
{
    float angle = -GetCameraYawRadians();
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
    offset = srVector3T<float>(
        node_position.x - listener->x,
        node_position.y - listener->y,
        node_position.z - listener->z);
    float x = Function4218E0(rotation.vectors[0], offset);
    float y = Function4218E0(rotation.vectors[1], offset);
    float z = Function4218E0(rotation.vectors[2], offset);

    memset(options, 0xff, sizeof(*options));
    srVector3T<float> listener_offset(
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
        SoundIsPlaying(sound_handle_13c) != 0) {
        return 1;
    }
    return 0;
}
