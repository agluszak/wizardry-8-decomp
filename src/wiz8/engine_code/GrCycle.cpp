#include "wiz8/engine_code/SoundEvent.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/float_constants.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/stLight.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/ground_shadow.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/SpellVisual.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/render_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/vector.h"
#include "wiz8/virtual_file.h"
#include "surrender/srModelInstance.h"
#include "surrender/srCore.h"
#include "FileMan.h"
#include <math.h>
#include <new>
#include <stdio.h>
#include <string.h>

template <>
srVector3T<double>* srVector3T<double>::method_004A90E0(
    const srVector3T<float>* source);

/* Engine Code\GrCycle.cpp. BEHAVIOUR_FIRST and BEHAVIOUR_LAST come from the
   canonical assertion at line 1598; the body bounds-checks against 1 and 3, so
   the enum runs 1..3. The stored-to object is whatever GrCycle's primary vtable
   slot 9 returns; only the byte it writes at +0x70 is established here.
   Slots 0..8 are declared solely to place slot 9 at vtable offset 0x24, which
   is what the canonical virtual call uses. */
#define BEHAVIOUR_FIRST 1
#define BEHAVIOUR_LAST  3

// VTABLE: WIZ8 0x005ece78 W8GrCycle
// VTABLE: WIZ8 0x005eceb8 W8Navigator
// class W8GrCycle

// SYNTHETIC: WIZ8 0x004a5f00
// W8GrCycle::`scalar deleting destructor'

// GLOBAL: WIZ8 0x005ecf98
extern float g_float_005ecf98;
// GLOBAL: WIZ8 0x0065be2c
W8GrowableVector<W8CameraShakeEffect*>* g_shake_effects_0065be2c;
// GLOBAL: WIZ8 0x0065be30
W8GameTimer* g_shake_timer_0065be30;

/* Everything except the timer's own pacing carries across, and bit zero of the
   flags is cleared so the copy is not treated as already started. The new timer
   is seeded from the source's speed rather than its remaining time. */
// FUNCTION: WIZ8 0x004ae000
W8CameraShakeEffect::W8CameraShakeEffect(const W8CameraShakeEffect& other)
    : flags_00(other.flags_00),
      intensity_04(other.intensity_04),
      value_08(other.value_08),
      position_0c(other.position_0c),
      timer_18(other.timer_18.m_duration_seconds, 0),
      cycle_3c(other.cycle_3c),
      frame_40(other.frame_40),
      subcycle_44(other.subcycle_44),
      value_48(other.value_48)
{
    flags_00 &= ~1u;
}

/* The first effect built also builds the shared live list and its timer; every
   later one finds them already there. The preset flag turns on the three bits
   that go together, and a null position leaves the effect where the caller's
   own default put it rather than at the origin. */
// FUNCTION: WIZ8 0x004aded0
W8CameraShakeEffect::W8CameraShakeEffect(
    float duration, char preset, float intensity, int value_08_,
    const srVector3T<float>* position)
    : flags_00(0),
      intensity_04(intensity),
      value_08(value_08_),
      timer_18(duration, 0),
      cycle_3c(0),
      frame_40(0),
      subcycle_44(0),
      value_48(0)
{
    if (g_shake_effects_0065be2c == 0) {
        g_shake_effects_0065be2c = new W8GrowableVector<W8CameraShakeEffect*>(5);
        g_shake_timer_0065be30 = new W8GameTimer(g_float_005ecf98, 0);
        g_shake_timer_0065be30->Restart();
    }
    if (preset != 0) {
        flags_00 |= 0x1c;
    }
    if (position != 0) {
        position_0c = *position;
    }
}

/* A shake made through the factory starts active and owned by the live list,
   which is the pair of bits set here. Trigger.cpp clears the ownership bit
   afterwards because it keeps its effect across frames and deletes it itself. */
// FUNCTION: WIZ8 0x004ae080
W8CameraShakeEffect* CreateCameraShakeEffect004AE080(
    float duration, char preset, float intensity, int value_08,
    const srVector3T<float>* position)
{
    W8CameraShakeEffect* effect = new W8CameraShakeEffect(
        duration, preset, intensity, value_08, position);

    effect->flags_00 |= 3;
    effect->timer_18.Restart();
    g_shake_effects_0065be2c->Add(effect);
    return effect;
}

/* Fire the effects one animation event names. An effect already on the live
   list is not added twice, but it is repositioned and restarted either way. */
// FUNCTION: WIZ8 0x004ae170
void TriggerShakeEffects004AE170(
    W8GrowableVector<W8CameraShakeEffect*>* effects,
    int cycle,
    unsigned int frame,
    int subcycle,
    const srVector3T<float>* position)
{
    int index;

    for (index = 0; index < effects->GetCount(); ++index) {
        W8CameraShakeEffect* effect = *effects->GetAt(index);

        if (effect->cycle_3c == cycle && effect->frame_40 == (int)frame &&
            effect->subcycle_44 == subcycle) {
            if ((effect->flags_00 & 1) == 0) {
                g_shake_effects_0065be2c->Add(effect);
            }
            effect->position_0c = *position;
            effect->flags_00 |= 1;
            effect->timer_18.Restart();
        }
    }
}

/* Stop every active effect this cycle owns. Coming off the live list and losing
   the active bit is unconditional; the delete is not, because Trigger.cpp's
   effects have already cleared the bit that says the list owns them. */
// FUNCTION: WIZ8 0x004ae270
void StopShakeEffects004AE270(W8GrowableVector<W8CameraShakeEffect*>* effects)
{
    int index;

    for (index = 0; index < effects->GetCount(); ++index) {
        W8CameraShakeEffect* effect = *effects->GetAt(index);

        if ((effect->flags_00 & 1) != 0) {
            unsigned int flags;

            g_shake_effects_0065be2c->RemoveAt(
                g_shake_effects_0065be2c->IndexOf(effect));
            flags = effect->flags_00;
            effect->flags_00 = flags & ~1u;
            if ((flags >> 1 & 1) != 0 && effect != 0) {
                delete effect;
            }
        }
    }
}

// SYNTHETIC: WIZ8 0x004ae070
// W8CameraShakeEffect::~W8CameraShakeEffect

extern int UpdateSoundEvents004D5890(
    W8GrowableVector<W8VectorElement005ED094*>* events,
    const srVector3T<float>* position,
    unsigned int event_mask,
    int cycle,
    unsigned int frame,
    int subcycle);
extern float g_float_005ec128;
extern float g_float_005ebc64;
extern int IncrementValue60DFAC(void);

/* Build the two paths used while reading a .mon resource, verify its one-byte
   version, and hand the open file plus its resource context to the typed cycle
   reader. */
// FUNCTION: WIZ8 0x004A67E0
unsigned char LoadGrCycle004A67E0(
    const W8GrCycleLoadContext* context,
    const char* mon_name,
    W8GrCycle** cycle,
    int cycle_index,
    int value,
    const char* directory,
    unsigned char object_type,
    const char* bitmap_directory)
{
    W8GrCycleReadInfo004A6970 info;
    char mon_path[128];
    char bitmap_path[1024];
    unsigned char version;
    unsigned char success;

    if (mon_name == 0 || cycle == 0) {
        srAssertFail(
            "pacName && ppCycle",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
            0x176,
            0);
    }

    sprintf(mon_path, "%s\\%s.mon", directory, mon_name);
    if (bitmap_directory == 0) {
        sprintf(bitmap_path, "%s\\Bitmaps", directory);
    }
    else {
        strcpy(bitmap_path, bitmap_directory);
    }

    int handle = FileOpen(mon_path, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
    if (handle == 0) {
        ReportError00401920(FormatString("Couldn't open %s", mon_path));
    }

    info.world_00 = context->world_00;
    info.handle_04 = handle;
    info.bitmap_directory_08 = bitmap_path;
    info.mon_path_0c = mon_path;
    success = 1;

    if (ReadVirtualFile(handle, &version, 1, 0) == 0 || version != 1 ||
        ReadGrCycleData004A6970(
            &info, cycle, cycle_index, value, object_type) == 0) {
        success = 0;
    }

    CloseVirtualFile(handle);
    if (success == 0) {
        srAssertFail(
            "fSuccess",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
            0x198,
            FormatString(
                "GrCycle::Read: ERROR - Read %s in %s failed",
                mon_path,
                bitmap_path));
    }
    return success;
}

/* Read the shared path and particle prefix, then dispatch the object-specific
   representation payload.  A particle attachment keeps the source node plus
   its local transform; the live cycle owns the attachment record and vector,
   while the particle node remains scene-owned. */
// FUNCTION: WIZ8 0x004A6970
unsigned char ReadGrCycleData004A6970(
    W8GrCycleReadInfo004A6970* info,
    W8GrCycle** cycle,
    int cycle_index,
    int value,
    unsigned char object_type)
{
    unsigned char has_path;
    unsigned char has_particles;
    unsigned char success;

    if (info == 0) {
        srAssertFail(
            "pInfo",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
            0x1ba,
            0);
    }

    if (*cycle == 0) {
        switch (object_type) {
        case 0:
            *cycle = new W8Monster;
            break;
        case 1:
            *cycle = new W8Missile;
            break;
        case 2:
            *cycle = new W8SpellVisual;
            break;
        default:
            srAssertFail(
                "FALSE",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                0x1d0,
                "Unknown object type");
            break;
        }
        if (*cycle == 0) {
            srAssertFail(
                "pCycle",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                0x1d2,
                0);
        }
        if (object_type == 0) {
            (*cycle)->unknown_008 = IncrementValue60DFAC();
        }
        (*cycle)->GetRepresentation()->current_cycle = -1;
    }

    ReadVirtualFile(info->handle_04, &has_path, 1, 0);
    if (has_path != 0 &&
        LoadPathAI004A92A0(
            reinterpret_cast<W8PathAI**>(&(*cycle)->m_pAI),
            info->handle_04) == 0) {
        srAssertFail(
            "fSuccess",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
            0x1e6,
            0);
    }

    ReadVirtualFile(info->handle_04, &has_particles, 1, 0);
    if (has_particles != 0) {
        W8GrowableVector<stParticle*> particles;

        success = ReadWorldParticles004BD0D0(
            reinterpret_cast<W8ReadLevelInfo*>(info),
            g_world->dynamic_scene,
            &particles);
        if (particles.GetCount() != 0) {
            int index;

            if ((*cycle)->m_plsParticles == 0) {
                (*cycle)->m_plsParticles =
                    new W8GrowableVector<W8GrCycleParticleAttachment*>;
            }
            for (index = 0; index < particles.GetCount(); ++index) {
                stParticle* particle = *particles.GetAt(index);
                W8GrCycleParticleAttachment* event =
                    static_cast<W8GrCycleParticleAttachment*>(
                        ::operator new(sizeof(W8GrCycleParticleAttachment)));
                srVector3T<double> location;

                if (event == 0) {
                    srAssertFail(
                        "pPartSys",
                        "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                        0x201,
                        0);
                }
                event->cycle_00 = cycle_index;
                event->subcycle_04 = -1;
                event->particle_08 = particle;
                location = particle->getLocation();
                event->position_0c.x = static_cast<float>(location.x);
                event->position_0c.y = static_cast<float>(location.y);
                event->position_0c.z = static_cast<float>(location.z);
                particle->getRotation(event->rotation_18);
                (*cycle)->m_plsParticles->Add(event);
            }
        }
    }

    switch (object_type) {
    case 0:
        success = static_cast<W8Monster*>(*cycle)->m_pRep->ReadCycleData004BF520(
            info, static_cast<W8Monster*>(*cycle), cycle_index, value);
        break;
    case 1:
        success = static_cast<W8Missile*>(*cycle)->m_pRep->ReadCycleData004A3300(
            info, static_cast<W8Missile*>(*cycle), cycle_index, value);
        break;
    case 2:
        success = static_cast<W8SpellVisual*>(*cycle)->host->ReadCycleData004AB340(
            info, static_cast<W8SpellVisual*>(*cycle), cycle_index, value);
        break;
    }
    return success;
}

// FUNCTION: WIZ8 0x004a5e50
W8GrCycle::W8GrCycle()
{
    current_model_instance_1a8 = 0;
    m_plsLights = 0;
    m_plsShakeEvents = 0;
    m_fDeleteLights = 0;
    unknown_1b5 = 0;
    m_plsParticles = 0;
    unknown_1bc = 0;
    enabled_1bd = 1;
    unknown_1be = 0;
    unknown_1bf = 0;
    m_ground_shadow = 0;
    unknown_1d4 = 0;
    scale_1cc = 1.0f;
}

/* A copy keeps none of the source's live scene state. The model instance, the
   light vector and both event vectors start empty and are rebuilt; the ground
   shadow is not carried over at all. What comes across is configuration: the
   delete-lights flag, the two bytes beside it, and the scale.

   Each of the three vectors is rebuilt the same way - a fresh growable vector at
   capacity five, then one owned element per source element. The lights are only
   rebuilt when this copy owns them, which is what m_fDeleteLights decides. */
// FUNCTION: WIZ8 0x004a5f20
W8GrCycle::W8GrCycle(const W8GrCycle& other)
    : W8GrObject(other), W8Navigator(other)
{
    int count;
    int index;

    current_model_instance_1a8 = 0;
    m_plsLights = 0;
    m_plsShakeEvents = 0;
    m_fDeleteLights = other.m_fDeleteLights;
    unknown_1b5 = other.unknown_1b5;
    m_plsParticles = 0;
    unknown_1bc = 0;
    enabled_1bd = 1;
    unknown_1be = other.unknown_1be;
    unknown_1bf = 0;
    scale_1cc = other.scale_1cc;
    m_ground_shadow = 0;
    unknown_1d4 = 0;
    if (other.m_plsLights != 0 && other.m_plsLights->GetCount() != 0 &&
        m_fDeleteLights != 0) {
        count = other.m_plsLights->GetCount();
        m_plsLights = new W8GrowableVector<stLight*>(5);
        for (index = 0; index < count; ++index) {
            stLight* source_light = *other.m_plsLights->GetAt(index);
            float x = source_light->positionalX();
            float y = source_light->positionalY();
            float z = source_light->positionalZ();
            stLight* copied_light = new stLight;

            if (copied_light != 0) {
                *copied_light = *source_light;
            }
            if (copied_light == 0) {
                srAssertFail("pstLight", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0xcb,
                             "Out of memory creating monster light");
            }
            copied_light->ConfigureMonsterCopy();
            copied_light->setLocation(x, y, z);
            copied_light->setFlag(srNode::FLAG_POSITIONAL_1);
            PLAdoptAppend(&g_world->m_list_0a8, copied_light);
            if (copied_light->definition() != 0) {
                g_world->lights_to_update->Add(copied_light);
            }
            m_plsLights->Add(copied_light);
        }
    }
    if (other.m_plsShakeEvents != 0 &&
        other.m_plsShakeEvents->GetCount() != 0) {
        count = other.m_plsShakeEvents->GetCount();
        m_plsShakeEvents = new W8GrowableVector<W8CameraShakeEffect*>(5);
        if (m_plsShakeEvents == 0) {
            srAssertFail("m_plsShakeEvents", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0xe3, 0);
        }
        for (index = 0; index < count; ++index) {
            W8CameraShakeEffect* event = new W8CameraShakeEffect(
                **other.m_plsShakeEvents->GetAt(index));

            if (event == 0) {
                srAssertFail("pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0xe9, 0);
            }
            m_plsShakeEvents->Add(event);
        }
    }
    if (other.m_plsParticles != 0 && other.m_plsParticles->GetCount() != 0) {
        count = other.m_plsParticles->GetCount();
        m_plsParticles = new W8GrowableVector<W8GrCycleParticleAttachment*>(5);
        if (m_plsParticles == 0) {
            srAssertFail("m_plsParticles", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0xf6, 0);
        }
        for (index = 0; index < count; ++index) {
            W8GrCycleParticleAttachment* event = static_cast<W8GrCycleParticleAttachment*>(
                ::operator new(sizeof(W8GrCycleParticleAttachment)));

            if (event != 0) {
                W8GrCycleParticleAttachment* source_event =
                    *other.m_plsParticles->GetAt(index);

                event->cycle_00 = source_event->cycle_00;
                event->subcycle_04 = source_event->subcycle_04;
                event->particle_08 =
                    new stParticle(*source_event->particle_08);
                event->position_0c = source_event->position_0c;
                event->rotation_18 = source_event->rotation_18;
                if (event->particle_08 == 0) {
                    srAssertFail("pstParticle", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0x66, 0);
                }
            }
            m_plsParticles->Add(event);
        }
    }
}

// FUNCTION: WIZ8 0x004a6610
W8GrCycle::~W8GrCycle()
{
    int count;
    int index;

    UnregisterGrCycle(this);
    if (m_plsLights != 0 && m_fDeleteLights != 0) {
        DestroyLightVector(m_plsLights);
        m_plsLights = 0;
    }
    if (m_plsShakeEvents != 0) {
        count = m_plsShakeEvents->GetCount();
        StopShakeEffects004AE270(m_plsShakeEvents);
        for (index = 0; index < count; ++index) {
            delete *m_plsShakeEvents->GetAt(index);
        }
        delete m_plsShakeEvents;
        m_plsShakeEvents = 0;
    }
    if (m_plsParticles != 0) {
        count = m_plsParticles->GetCount();
        for (index = 0; index < count; ++index) {
            W8GrCycleParticleAttachment* event = *m_plsParticles->GetAt(index);
            stParticle* owner = event->particle_08;

            if (owner == 0 || owner->active_particle_count_18c == 0) {
                if (event->particle_08 != 0) {
                    event->particle_08->release();
                }
                delete event;
            }
            else {
                event->particle_08 = 0;
                delete event;
                owner->SetActive(1);
                owner->state_184 = 1;
                owner->active_190 = 1;
            }
        }
        delete m_plsParticles;
        m_plsParticles = 0;
    }
    if (m_ground_shadow != 0) {
        m_ground_shadow->release();
    }
}

// FUNCTION: WIZ8 0x004a6df0
void W8GrCycle::SetPosition004A6DF0(srVector3T<float>* position)

{
  W8EmitterHost* representation = GetRepresentation();
  
  representation->SetLocation004B8850(position);
  SetPositionInternal00453590(position);
  return;
}


// FUNCTION: WIZ8 0x004a6e20
void W8GrCycle::TickAnimation(float scale)
{
    W8EmitterHost* representation = GetRepresentation();

    ApplyPendingCycle();
    if (representation->flag_06d != 0) {
        signed char subcycle_count = GetNumSubCycles();

        if (subcycle_count != 0) {
            unsigned int now = g_shared_timer_base->getMsTime(
                srTimer::TIMER_READ_DEFAULT);
            unsigned int elapsed = now - representation->timer_068;
            float rate;
            float progress;
            int frames;

            if (elapsed > 1000) {
                elapsed = 1000;
                representation->timer_068 = now - 1000;
            }

            rate = GetCurrentAnimationScale() * scale;
            progress = elapsed * rate * g_float_005ec128;
            frames = (int)progress;
            unknown_1d4 = progress - (float)frames;

            if (frames != 0) {
                srVector3T<float> position;

                representation->timer_068 +=
                    (int)((float)frames * g_float_005ebc64 / rate);
                unknown_1bc = 0;
                position = GetPosition();
                do {
                    --frames;
                    AdvanceAnimationFrame(subcycle_count, 0);
                    if (ApplyPendingCycle() != 0) {
                        frames = 0;
                    }
                    if (m_plsSoundEvents != 0) {
                        UpdateSoundEvents004D5890(
                            m_plsSoundEvents,
                            &position,
                            0x101,
                            representation->current_cycle,
                            representation->flag_064,
                            representation->current_subcycle);
                    }
                    if (m_plsShakeEvents != 0) {
                        TriggerShakeEffects004AE170(
                            m_plsShakeEvents,
                            representation->current_cycle,
                            representation->flag_064,
                            representation->current_subcycle,
                            &position);
                    }
                } while (frames != 0);
            }
        }

        if (m_plsLights != 0 && m_plsLights->GetCount() != 0) {
            UpdateLights004A7150();
        }
        unknown_1b5 = representation->flag_064;
    }
}

// FUNCTION: WIZ8 0x004a6fc0
unsigned char W8GrCycle::ApplyPendingCycle()
{
    W8EmitterHost* representation = GetRepresentation();

    if (representation != 0 &&
        representation->pending_cycle != -1 &&
        CanEnterCycle(representation->pending_cycle) != 0) {
        signed char pending_cycle =
            representation->pending_cycle;
        SetCycle(pending_cycle);
        representation->pending_cycle = -1;

        if (representation->value_066 != 0xffff) {
            unsigned char subcycle = (unsigned char)representation->value_066;
            signed char subcycle_count = GetNumSubCycles();
            W8EmitterHost* current = GetRepresentation();

            if ((int)subcycle < (int)subcycle_count) {
                current->flag_064 = subcycle;
            }
            representation->value_066 = 0xffff;
        }

        if ((signed char)representation->behaviour_071 != -1) {
            signed char behaviour = (signed char)representation->behaviour_071;
            W8EmitterHost* current = GetRepresentation();

            if (behaviour < BEHAVIOUR_FIRST || behaviour > BEHAVIOUR_LAST) {
                srAssertFail(
                    "(bBehaviour >= BEHAVIOUR_FIRST) && (bBehaviour <= BEHAVIOUR_LAST)",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                    0x63e,
                    0);
            }
            current->flag_070 = behaviour;
            representation->behaviour_071 = 0xff;
        }

        srVector3T<float> position = GetPosition();
        if (m_plsSoundEvents != 0) {
            UpdateSoundEvents004D5890(
                m_plsSoundEvents,
                &position,
                0x103,
                representation->current_cycle,
                representation->flag_064,
                representation->current_subcycle);
        }
        if (m_plsShakeEvents != 0) {
            TriggerShakeEffects004AE170(
                m_plsShakeEvents,
                representation->current_cycle,
                representation->flag_064,
                representation->current_subcycle,
                &position);
        }

        signed char subcycle_count = GetNumSubCycles();
        if ((unsigned char)representation->flag_064 >=
            (unsigned char)subcycle_count) {
            representation->flag_064 = subcycle_count - 1;
        }
        representation->flag_06d = 1;
        representation->timer_068 = g_shared_timer_base->getMsTime(
            srTimer::TIMER_READ_DEFAULT);
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004a7150
void W8GrCycle::UpdateLights004A7150()
{
    W8EmitterHost* representation = GetRepresentation();
    int count = m_plsLights->GetCount();
    int index;

    for (index = 0; index < count; ++index) {
        stLight* light = *m_plsLights->GetAt(index);
        stLightDefinition* definition = light->definition();

        if (definition == 0) {
            continue;
        }
        if (definition->type_04 == 1) {
            stLightDefinition005ECDBC* cycle_definition =
                static_cast<stLightDefinition005ECDBC*>(definition);
            if ((cycle_definition->flags_08 & 3) == 3 &&
                (cycle_definition->flags_08 & 0x40) != 0) {
                if (cycle_definition->IsEnabledForSubcycle(
                        representation->flag_064) == 0) {
                    if (light->parentNode() != 0) {
                        light->setParent(0, 0);
                    }
                } else if ((representation->flag_064 == 0 &&
                            cycle_definition->value_3c == 0) ||
                           light->parentNode() == srCore.getRootNode()) {
                    light->setParent(g_world->dynamic_scene, 0);
                    light->m_positional_248 = 0;
                    light->m_positional_250 = 1;
                    light->Reset0049D070();
                }
            }
        } else if (definition->type_04 == 2) {
            if (representation->flag_064 == 0 || unknown_1bc != 0) {
                light->Reset0049D070();
            }
            light->SetDefinitionTime0049C940(
                (float)representation->flag_064 + unknown_1d4);
            if (definition->IsEnabledForSubcycle(0) == 0) {
                if (light->parentNode() != 0) {
                    light->setParent(0, 0);
                }
            } else if (light->parentNode() == srCore.getRootNode()) {
                light->setParent(g_world->dynamic_scene, 0);
            }
        }
    }
}

extern void Function4A9720(void* path);
extern void Function4A9110(void* path);

// FUNCTION: WIZ8 0x004a7dd0
unsigned char W8GrCycle::GetAnimationBounds(
    srVector3T<float>* minimum, srVector3T<float>* maximum)
{
    W8AnimObj* animation = GetCurrentAnimation();
    W8EmitterHost* representation = GetRepresentation();

    return AnimObjGetBounds004A1710(
        animation,
        representation->m_bLOD,
        representation->flag_064,
        (srVector3T<float>*)minimum,
        (srVector3T<float>*)maximum);
}

// FUNCTION: WIZ8 0x004a7e10
unsigned char W8GrCycle::GetAnimationRadius(float* radius)
{
    W8AniMesh* mesh = GetCurrentAniMesh();

    if (mesh == 0) {
        srAssertFail(
            "0",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
            0x541,
            0);
        return 0;
    }
    return AniMeshRadius004B66E0(mesh, radius);
}

// FUNCTION: WIZ8 0x004a72f0
void W8GrCycle::AdvanceAnimationFrame(int, int)
{
    W8EmitterHost* representation = GetRepresentation();
    W8AnimObj* animation = GetCurrentAnimation();
    unsigned char frame;

    if (representation->flag_070 == 1) {
        if (representation->flag_06e == 1) {
            frame = representation->flag_064;
            if (frame < representation->counter_095) {
                representation->flag_064 = frame + 1;
            } else {
                representation->flag_06d = 0;
            }
        } else if (representation->flag_06e == 3) {
            frame = representation->flag_064;
            if (frame > representation->counter_094) {
                representation->flag_064 = frame - 1;
            } else {
                representation->flag_06d = 0;
            }
        }
    } else if (representation->flag_06e == 1) {
        frame = representation->flag_064;
        if (frame != representation->counter_095) {
            representation->flag_064 = frame + 1;
        } else if (representation->flag_06f == 2) {
            representation->flag_064 = frame - 1;
            representation->flag_06e = 3;
        } else if (representation->flag_06f == 1) {
            representation->flag_064 = representation->counter_094;
            unknown_1bc = 1;
        }
    } else if (representation->flag_06e == 3) {
        frame = representation->flag_064;
        if (frame > representation->counter_094) {
            representation->flag_064 = frame - 1;
        } else if (representation->flag_06f == 2) {
            representation->flag_064 = representation->counter_094 + 1;
            representation->flag_06e = 1;
        } else if (representation->flag_06f == 1) {
            representation->flag_064 = representation->counter_095;
        }
    }

    if (AnimationIsRunning(animation) == 1) {
        unsigned int count = AnimObjListCount004A1620(
            animation, representation->m_bLOD);
        unsigned int index;

        for (index = 0; index < count; ++index) {
            W8PathAI* path = (W8PathAI*)AnimObjListEntry004A16C0(
                animation, representation->m_bLOD, (signed char)index);
            if (path != 0) {
                PathAISetValue004A9F60(
                    path, (float)representation->flag_064);
            }
        }
    }
}

// FUNCTION: WIZ8 0x004a7420
void W8GrCycle::ResetRepresentation004A7420()
{
    W8EmitterHost* target = GetRepresentation();

    if (target == 0) {
        srAssertFail("pRep", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0x3ae, 0);
    }
    Function4A9720(m_pAI);
    target->flag_06e = 1;
    target->flag_064 = 0;
    target->timer_068 =
        g_shared_timer_base->getUTime(srTimer::TIMER_READ_DEFAULT);
}

/* Push the cycle's animation state into the live scene.

   A running animation places every model instance its current list holds; a
   stopped one places the single instance the cycle/frame/LOD selector returns,
   and then walks that instance's children when it has any. Either way the
   representation's four-dword render block and its optional scale ride along,
   and the last instance placed becomes the cached one.

   The ground shadow and the owned lights follow. The shadow deliberately
   reparents to the global world rather than to the pWorld argument, which is
   why both appear in one body. */
// FUNCTION: WIZ8 0x004a7470
void W8GrCycle::UpdateRepresentation(W8World* pWorld)
{
    W8EmitterHost* pRep = GetRepresentation();
    W8AnimObj* animation;
    srMatrix3T<float> rotation;
    srVector3T<double> location;
    srVector3T<float> vecPos;
    int index;

    if (pWorld == 0) {
        srAssertFail("pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0x3d1, 0);
    }
    if (pRep->active == 0) {
        return;
    }
    animation = GetCurrentAnimation();
    if (AnimationIsRunning(animation) == 1) {
        int count = (int)AnimObjListCount004A1620(animation, pRep->m_bLOD);

        for (index = 0; index < count; ++index) {
            stModelInstance005EC7D0* psrMesh =
                (stModelInstance005EC7D0*)AnimObjDispatchList004A1560(
                    animation, pRep->m_bLOD, (signed char)index);
            W8PathAI* path;
            srMatrix3T<float> current;

            if (psrMesh == 0) {
                srAssertFail(
                    "psrMesh", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0x3e4, 0);
            }
            *(W8AnimRepValue4*)&psrMesh->render_depth_164 = pRep->value_04c;
            if (pRep->flag_061 != 0) {
                if (pRep->value_05c == g_float_005ebb38) {
                    psrMesh->flag_1a0 = 0;
                } else {
                    psrMesh->flag_1a0 = 1;
                    psrMesh->scale_1a4 = pRep->value_05c;
                }
            }
            psrMesh->clearFlag(srNode::FLAG_POSITIONAL_0);
            psrMesh->setParent(pWorld->dynamic_scene, 0);
            path = (W8PathAI*)AnimObjListEntry004A16C0(
                animation, pRep->m_bLOD, (signed char)index);
            if (path != 0) {
                PathAIApply004AA520(path, psrMesh);
            }
            vecPos = movement_0c0.position_040;
            location.x = vecPos.x;
            location.y = vecPos.y + movement_0c0.vertical_offset_0c0;
            location.z = vecPos.z;
            psrMesh->setLocation(location);
            pRep->GetRotation004B88F0(&rotation);
            psrMesh->getRotation(current);
            rotation.method_00421A40(current);
            psrMesh->setRotation(rotation);
            current_model_instance_1a8 = psrMesh;
        }
    } else {
        stModelInstance005EC7D0* psrMesh =
            (stModelInstance005EC7D0*)SelectCycleFrameLod004A8360(
                pRep->current_cycle,
                pRep->flag_064,
                (signed char)pRep->m_bLOD);
        srNode* child;

        if (psrMesh == 0) {
            srAssertFail(
                "psrMesh", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0x40f, 0);
        }
        AniMeshSetFlag10004B6860(
            pRep->GetEmitterAniMesh(pRep->current_cycle), 1);
        *(W8AnimRepValue4*)&psrMesh->render_depth_164 = pRep->value_04c;
        if (pRep->flag_061 != 0) {
            if (pRep->value_05c == g_float_005ebb38) {
                psrMesh->flag_1a0 = 0;
            } else {
                psrMesh->flag_1a0 = 1;
                psrMesh->scale_1a4 = pRep->value_05c;
            }
        }
        vecPos = movement_0c0.position_040;
        location.x = vecPos.x;
        location.y = vecPos.y + movement_0c0.vertical_offset_0c0;
        location.z = vecPos.z;
        pRep->GetRotation004B88F0(&rotation);
        child = psrMesh->firstChild();
        if (child == 0) {
            psrMesh->setLocation(location);
            psrMesh->setRotation(rotation);
            psrMesh->clearFlag(srNode::FLAG_POSITIONAL_0);
        } else {
            do {
                child->setLocation(location);
                child->setRotation(rotation);
                child = child->nextSibling();
            } while (child != 0);
        }
        current_model_instance_1a8 = psrMesh;
        psrMesh->clearFlag(srNode::FLAG_POSITIONAL_1);
        psrMesh->setParent(pWorld->dynamic_scene, 0);
    }
    UpdateParticleAttachments004A7E50();
    if (m_ground_shadow != 0) {
        srVector3T<float> position = GetPosition();

        location.x = position.x;
        location.y = position.y;
        location.z = position.z;
        m_ground_shadow->setLocation(location);
        m_ground_shadow->angle_138 = GetYaw();
        m_ground_shadow->clearFlag(srNode::FLAG_POSITIONAL_1);
        m_ground_shadow->setParent(g_world->dynamic_scene, 0);
    }
    if (m_plsLights != 0) {
        int count = m_plsLights->GetCount();
        srVector3T<float> origin;

        if (AnimationIsRunning(animation) == 1) {
            pRep->GetLocation004B8890(&origin);
        } else {
            vecPos = movement_0c0.position_040;
            origin.x = vecPos.x;
            origin.y = vecPos.y + movement_0c0.vertical_offset_0c0;
            origin.z = vecPos.z;
        }
        pRep->GetRotation004B88F0(&rotation);
        for (index = 0; index < count; ++index) {
            stLight* light = *m_plsLights->GetAt(index);
            srVector3T<float> offset = light->m_positional_228;

            location.x = rotation.vectors[0].x * offset.x +
                rotation.vectors[0].y * offset.y +
                rotation.vectors[0].z * offset.z + origin.x;
            location.y = rotation.vectors[1].x * offset.x +
                rotation.vectors[1].y * offset.y +
                rotation.vectors[1].z * offset.z + origin.y;
            location.z = rotation.vectors[2].x * offset.x +
                rotation.vectors[2].y * offset.y +
                rotation.vectors[2].z * offset.z + origin.z;
            light->setLocation(location);
        }
    }
}

// FUNCTION: WIZ8 0x004a7a70
void W8GrCycle::DetachRepresentation004A7A70(W8World* world)
{
    W8EmitterHost* representation = GetRepresentation();
    W8AnimObj* animation = GetCurrentAnimation();

    if (AnimationIsRunning(animation) == 1) {
        int count = (int)AnimObjListCount004A1620(
            animation, representation->m_bLOD);
        for (int index = 0; index < count; ++index) {
            srModelInstance* mesh = AnimObjDispatchList004A1560(
                animation, representation->m_bLOD,
                static_cast<signed char>(index));
            if (mesh == 0) {
                srAssertFail(
                    "psrMesh",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                    0x476, 0);
            }
            W8AniMesh* ani_mesh = representation->GetEmitterAniMesh(
                representation->current_cycle);
            if (ani_mesh != 0) {
                AniMeshSetFlag10004B6860(ani_mesh, 0);
            }
            mesh->setFlag(srNode::FLAG_POSITIONAL_0);
            mesh->setParent(0, 1);
        }
    }
    else if (current_model_instance_1a8 != 0) {
        if (world == 0) {
            srAssertFail(
                "pWorld",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                0x484, 0);
        }
        W8AniMesh* ani_mesh = representation->GetEmitterAniMesh(
            representation->current_cycle);
        AniMeshSetFlag10004B6860(ani_mesh, 0);
        current_model_instance_1a8->setFlag(srNode::FLAG_POSITIONAL_0);
        current_model_instance_1a8->setFlag(srNode::FLAG_POSITIONAL_1);
        current_model_instance_1a8->setParent(0, 0);
        current_model_instance_1a8 = 0;
    }

    if (m_ground_shadow != 0) {
        m_ground_shadow->setFlag(srNode::FLAG_POSITIONAL_1);
        m_ground_shadow->setParent(0, 1);
    }
}

/* Place every particle this cycle has attached to its model.

   An attachment normally rides a named vertex of the model's mesh: the
   particle's own key is looked up through the mesh-model chain, and the vertex
   it maps to gives the offset. An attachment whose key resolves to nothing
   falls back to the offset it carries itself. Either way the offset is scaled
   and rotated into the model instance's frame and added to its location.

   The particle's own mode then decides its orientation: mode three aims it
   along the cycle's stored axis, mode four leaves the orientation alone, and
   anything else composes the attachment's own rotation with the model's. */
// FUNCTION: WIZ8 0x004a7e50
void W8GrCycle::UpdateParticleAttachments004A7E50()
{
    int count;
    int index;
    srModelInstance* psrMesh;
    W8EmitterHost* pRep;
    stMeshModel* pMeshModel;
    srMatrix3T<float> rotation;
    srVector3T<double> scale;
    srVector3T<double> location;
    srVector3T<double> target;
    srVector3T<double> axis;
    srMatrix3T<float> combined;
    srMatrix3T<double> world;
    srVector3T<float> offset;
    srVector3T<float> placed;
    int vertex;
    float scale_x;
    float scale_y;
    float scale_z;

    if (m_plsParticles == 0) {
        return;
    }
    count = m_plsParticles->GetCount();
    if (count == 0) {
        return;
    }
    psrMesh = current_model_instance_1a8;
    if (psrMesh == 0) {
        W8AnimObj* animation = GetCurrentAnimation();
        W8EmitterHost* rep = GetRepresentation();

        if (AnimationIsRunning(animation) == 1) {
            psrMesh = AnimObjDispatchList004A1560(animation, rep->m_bLOD, 0);
        } else {
            psrMesh = SelectCycleFrameLod004A8360(
                rep->current_cycle,
                rep->flag_064,
                (signed char)rep->m_bLOD);
        }
    }
    pRep = GetRepresentation();
    /* When there is no instance the mesh model is never read, and the original
       leaves it holding whatever the count's slot did. */
    pMeshModel = psrMesh != 0 ? (stMeshModel*)psrMesh->model() : (stMeshModel*)count;

    current_model_instance_1a8->getRotation(rotation);
    scale = current_model_instance_1a8->getScale();
    scale_x = (float)scale.x;
    scale_y = (float)scale.y;
    scale_z = (float)scale.z;

    for (index = 0; index < count; ++index) {
        W8GrCycleParticleAttachment* attachment = *m_plsParticles->GetAt(index);
        stParticle* particle = attachment->particle_08;
        if (particle->active_1a0 == 0) {
            continue;
        }
        if (pMeshModel != 0) {
            short key = particle->value_260;

            if (key < 0) {
                vertex = -1;
            } else {
                stMeshModel* scan = pMeshModel;

                do {
                    vertex = scan->FindMappedIndex(key);
                    if (vertex != -1) {
                        break;
                    }
                    scan = scan->next;
                } while (scan != 0);
                pMeshModel = scan;
            }
        } else {
            vertex = -1;
        }
        if (pMeshModel == 0 || vertex == -1) {
            offset = attachment->position_0c;
        } else {
            srVector3T<float>* locations;

            if ((pMeshModel->flags_3a0 >> 2 & 1) == 0) {
                locations = pMeshModel->getVertexLoc();
            } else {
                locations = pMeshModel->GetVertexLocations00471AD0(
                    pRep->flag_064, 1, 0);
            }
            if (vertex >= pMeshModel->vertex_location_count_22c) {
                vertex = 0;
            }
            offset = locations[vertex];
        }
        offset.x = offset.x * scale_x;
        offset.y = offset.y * scale_y;
        offset.z = offset.z * scale_z;
        placed.x = rotation.vectors[0].x * offset.x +
            rotation.vectors[0].y * offset.y +
            rotation.vectors[0].z * offset.z;
        placed.y = Function4218E0(rotation.vectors[1], offset);
        placed.z = Function4218E0(rotation.vectors[2], offset);
        location = current_model_instance_1a8->getLocation();
        placed.x = placed.x + (float)location.x;
        placed.y = (float)location.y + placed.y;
        placed.z = (float)location.z + placed.z;

        if (unknown_1bf != 0 && particle->value_1b8 == 3) {
            target.x = placed.x;
            target.y = placed.y;
            target.z = placed.z;
            axis.x = m_axis_1c0.x;
            axis.y = m_axis_1c0.y;
            axis.z = m_axis_1c0.z;
            particle->setRotation(axis, target, 0.0);
        } else if (particle->value_1b8 != 4) {
            combined = rotation;
            combined.method_00421A40(attachment->rotation_18);
            world.vectors[0].x = combined.vectors[0].x;
            world.vectors[0].y = combined.vectors[0].y;
            world.vectors[0].z = combined.vectors[0].z;
            world.vectors[1].method_004A90E0(&combined.vectors[1]);
            world.vectors[2].method_004A90E0(&combined.vectors[2]);
            particle->setWorldSpaceRotation(world);
        }
        location.x = placed.x;
        location.y = placed.y;
        location.z = placed.z;
        particle->setLocation(location);
    }
}

/* Choose the level of detail for the current cycle from the distance to the
   listener and from which LODs the cycle actually has.

   Availability is probed by asking the representation for each LOD's frame
   zero; a cycle with none of the three is a content error and says so. The
   distance thresholds are the representation's own pair scaled by the detail
   slider, and LOD 2 is the near one. The tail then lets the fog/detail setting
   override the distance choice in either direction, but only towards a LOD the
   cycle has. */
// FUNCTION: WIZ8 0x004a7be0
void W8GrCycle::SelectLOD004A7BE0(const float* position)
{
    W8EmitterHost* pRep = GetRepresentation();
    srVector3T<float> vecLoc = pRep->location_004;
    float dx = vecLoc.x - position[0];
    float dy = vecLoc.y - position[1];
    float dz = vecLoc.z - position[2];
    float distance = (float)sqrt(dx * dx + dy * dy + dz * dz);
    unsigned char has_lod_2 =
        pRep->SetCycleFrameLod(pRep->current_cycle, 0, 2) != 0;
    unsigned char has_lod_1 =
        pRep->SetCycleFrameLod(pRep->current_cycle, 0, 1) != 0;
    unsigned char has_lod_0 =
        pRep->SetCycleFrameLod(pRep->current_cycle, 0, 0) != 0;

    if (has_lod_2 == 0 && has_lod_1 == 0 && has_lod_0 == 0) {
        ReportError00401920("Monster has no valid LODs!");
    }
    if (g_render_brightness_60a210 * pRep->lod_range_09c > distance) {
        if (has_lod_2 != 0) {
            pRep->m_bLOD = 2;
        } else if (has_lod_1 != 0) {
            pRep->m_bLOD = 1;
        } else {
            pRep->m_bLOD = 0;
        }
    } else if (g_render_brightness_60a210 * pRep->lod_range_0a0 > distance) {
        if (has_lod_1 != 0) {
            pRep->m_bLOD = 1;
        } else if (has_lod_2 != 0) {
            pRep->m_bLOD = 2;
        } else {
            pRep->m_bLOD = 0;
        }
    } else {
        if (has_lod_0 != 0) {
            pRep->m_bLOD = 0;
        } else if (has_lod_1 != 0) {
            pRep->m_bLOD = 1;
        } else {
            pRep->m_bLOD = 2;
        }
    }
    if (g_render_fog_distance_60e610 <= g_float_005ebc3c && pRep->m_bLOD != 2) {
        if (has_lod_2 != 0) {
            pRep->m_bLOD = 2;
        }
        return;
    }
    if ((g_render_fog_distance_60e610 <= g_float_005ec5c0 && pRep->m_bLOD == 0) ||
        (g_render_fog_distance_60e610 >= g_float_005ec5c4 && pRep->m_bLOD == 2)) {
        if (has_lod_1 != 0) {
            pRep->m_bLOD = 1;
        }
        return;
    }
    if (g_render_fog_distance_60e610 >= g_float_005ec390 && pRep->m_bLOD != 0 &&
        has_lod_0 != 0) {
        pRep->m_bLOD = 0;
    }
}

/* Select the model instance for the representation's current cycle, frame,
   and LOD. AnimObj's frame lookup is the source of the returned instance; the
   callers immediately use it as an srNode/srModelInstance. */
// FUNCTION: WIZ8 0x004a8250
srModelInstance* W8GrCycle::GetCurrentModelInstance004A8250()
{
    W8AnimObj* animation;
    W8EmitterHost* representation;

    if (current_model_instance_1a8 != 0) {
        return current_model_instance_1a8;
    }

    animation = GetCurrentAnimation();
    representation = GetRepresentation();
    if (AnimationIsRunning(animation) == 1) {
        return AnimObjDispatchList004A1560(
            animation, representation->m_bLOD, 0);
    }

    return SelectCycleFrameLod004A8360(
        representation->current_cycle,
        representation->flag_064,
        (signed char)representation->m_bLOD);
}

// FUNCTION: WIZ8 0x004a8360
srModelInstance* W8GrCycle::SelectCycleFrameLod004A8360(
    signed char cycle, signed char frame, signed char lod)
{
    W8EmitterHost* target;

    if (lod < 0 || lod >= 3) {
        srAssertFail(
            "bLOD >= 0 && bLOD < NUM_LODS",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
            0x5f1,
            0);
    }
    if (frame < 0) {
        srAssertFail("bFrame >= 0", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0x5f2, 0);
    }
    if (cycle < 0) {
        srAssertFail("bCycle >= 0", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0x5f3, 0);
    }
    if (m_pAI != 0) {
        target = GetRepresentation();
        PathAIApplyToRep004A91F0(
            (W8PathAI*)m_pAI, target);
    }
    target = GetRepresentation();
    return target->SetCycleFrameLod(cycle, frame, lod);
}

// FUNCTION: WIZ8 0x004a8400
unsigned char W8GrCycle::ReplacePath004A8400(void* path)
{
    if (m_pAI != 0) {
        Function4A9110(m_pAI);
    }
    m_pAI = path;
    return 1;
}

// FUNCTION: WIZ8 0x004a84a0
void W8GrCycle::SubmitTargetValue004A84A0()
{
    W8EmitterHost* target = GetRepresentation();

    GetAnimationRadius(&target->value_0a8);
}

/* The pointer at W8GrCycle +0x1b0 owns this specialization. No source or debug
   witness names the element type, so it remains address-qualified. */

/* Parallel registries: each name has one growable vector of cycle objects. */
extern W8GrowableVector<char*> g_grcycle_names;                       /* 0x0065BDF0 */
// VTABLE: WIZ8 0x005ecedc
// class W8GrowableVector<W8GrCycle*>

// SYNTHETIC: WIZ8 0x004a9050
// W8GrowableVector<W8GrCycle*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004a9070
// W8GrowableVector<W8GrCycle*>::~W8GrowableVector<W8GrCycle*>

extern W8GrowableVector<W8GrowableVector<W8GrCycle*>*> g_grcycles_by_name;
                                                                    /* 0x0065BE00 */

// VTABLE: WIZ8 0x005eced4
// class W8GrowableVector<W8CameraShakeEffect*>

// SYNTHETIC: WIZ8 0x004a8f70
// W8GrowableVector<W8CameraShakeEffect*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004a8f90
// W8GrowableVector<W8CameraShakeEffect*>::~W8GrowableVector<W8CameraShakeEffect*>

// VTABLE: WIZ8 0x005ececc
// class W8GrowableVector<W8GrCycleParticleAttachment*>

// SYNTHETIC: WIZ8 0x004a8fe0
// W8GrowableVector<W8GrCycleParticleAttachment*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004a8fb0
// W8GrowableVector<W8GrCycleParticleAttachment*>::~W8GrowableVector<W8GrCycleParticleAttachment*>

// FUNCTION: WIZ8 0x004a8430
void W8GrCycle::SetSubCycle(unsigned char subcycle)
{
    signed char count = GetNumSubCycles();
    W8EmitterHost* target = GetRepresentation();

    if (subcycle < count) {
        target->flag_064 = subcycle;
    }
}

// FUNCTION: WIZ8 0x004a8460
void W8GrCycle::SetBehaviour(signed char bBehaviour)
{
    W8EmitterHost* target = GetRepresentation();

    if (bBehaviour < BEHAVIOUR_FIRST || bBehaviour > BEHAVIOUR_LAST) {
        srAssertFail(
            "(bBehaviour >= BEHAVIOUR_FIRST) && (bBehaviour <= BEHAVIOUR_LAST)",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
            0x63e,
            0);
    }
    target->flag_070 = bBehaviour;
}

// FUNCTION: WIZ8 0x004a84c0
void W8GrCycle::SetLights(W8GrowableVector<stLight*>* lights)
{
    if (m_fDeleteLights && m_plsLights != 0) {
        srAssertFail(
            "!m_fDeleteLights || (m_fDeleteLights && !m_plsLights)",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
            0x678,
            0);
    }
    m_fDeleteLights = 0;
    m_plsLights = lights;
}

// FUNCTION: WIZ8 0x004a8530
void W8GrCycle::AddShakeEffect004A8530(W8CameraShakeEffect* effect)
{
    if (m_plsShakeEvents == 0) {
        m_plsShakeEvents = new W8GrowableVector<W8CameraShakeEffect*>();
    }
    m_plsShakeEvents->Add(effect);
}

// FUNCTION: WIZ8 0x004a8650
const char* __fastcall GetGrCycleName(W8GrCycle* cycle)
{
    int name_index;

    for (name_index = 0; name_index < g_grcycles_by_name.GetCount(); ++name_index) {
        W8GrowableVector<W8GrCycle*>* cycles =
            *g_grcycles_by_name.GetAt(name_index);
        if (cycles->GetCount() == 0) {
            srAssertFail(
                "plsCyclesOfAName->Length()",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                0x6c4,
                0);
        }
        if (cycles->IndexOf(cycle) != -1) {
            return *g_grcycle_names.GetAt(name_index);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004a8700
unsigned char __fastcall IsSoleGrCycleForName(W8GrCycle* cycle)
{
    int name_index;

    for (name_index = 0; name_index < g_grcycles_by_name.GetCount(); ++name_index) {
        W8GrowableVector<W8GrCycle*>* cycles =
            *g_grcycles_by_name.GetAt(name_index);
        if (cycles->GetCount() == 0) {
            srAssertFail(
                "plsCyclesOfAName->Length()",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                0x6e6,
                0);
        }
        if (cycles->IndexOf(cycle) != -1) {
            return cycles->GetCount() == 1;
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x004a87a0
W8GrCycle* FindFirstGrCycleByName(const char* name)
{
    int name_index;

    for (name_index = 0; name_index < g_grcycle_names.GetCount(); ++name_index) {
        if (_stricmp(name, *g_grcycle_names.GetAt(name_index)) == 0) {
            W8GrowableVector<W8GrCycle*>* cycles =
                *g_grcycles_by_name.GetAt(name_index);
            if (cycles->GetCount() == 0) {
                srAssertFail(
                    "plsCyclesOfThisName->Length()",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                    0x709,
                    0);
            }
            return *cycles->GetAt(0);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004a8830
unsigned char UnregisterGrCycle(W8GrCycle* cycle)
{
    int name_index;

    for (name_index = 0; name_index < g_grcycles_by_name.GetCount(); ++name_index) {
        W8GrowableVector<W8GrCycle*>* cycles =
            *g_grcycles_by_name.GetAt(name_index);
        if (cycles->GetCount() == 0) {
            srAssertFail(
                "plsCyclesOfAName->Length()",
                "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                0x726,
                0);
        }
        int cycle_index = cycles->IndexOf(cycle);
        if (cycle_index != -1) {
            cycles->RemoveAt(cycle_index);
            if (cycles->GetCount() != 0) {
                return 0;
            }
            g_grcycles_by_name.RemoveAt(name_index);
            delete cycles;
            char* name = g_grcycle_names.RemoveAt(name_index);
            delete[] name;
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004a89a0
void RegisterGrCycle(const char* name, W8GrCycle* cycle)
{
    int name_index;

    for (name_index = 0; name_index < g_grcycle_names.GetCount(); ++name_index) {
        if (_stricmp(name, *g_grcycle_names.GetAt(name_index)) == 0) {
            W8GrowableVector<W8GrCycle*>* cycles =
                *g_grcycles_by_name.GetAt(name_index);
            if (cycles->GetCount() == 0) {
                srAssertFail(
                    "plsCyclesOfThisName->Length()",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
                    0x763,
                    0);
            }
            cycles->Add(cycle);
            return;
        }
    }

    char* owned_name = new char[strlen(name) + 1];
    strcpy(owned_name, name);
    W8GrowableVector<W8GrCycle*>* cycles =
        new W8GrowableVector<W8GrCycle*>();
    cycles->Add(cycle);
    g_grcycle_names.Add(owned_name);
    g_grcycles_by_name.Add(cycles);
}

// FUNCTION: WIZ8 0x004a8c50
void DestroyLightVector(W8GrowableVector<stLight*>* vector)
{
    int count;
    int index;

    if (vector != 0) {
        count = vector->GetCount();
        for (index = 0; index < count; ++index) {
            stLight* light = *vector->GetAt(index);

            if (light->definition() != 0) {
                int world_index = g_world->lights_to_update->IndexOf(light);
                if (world_index != -1) {
                    g_world->lights_to_update->RemoveAt(world_index);
                }
            }
            light->setParent(0, 1);
            WorldRemoveLight(g_world, light);
        }
        delete vector;
    }
}

// FUNCTION: WIZ8 0x004a8d10
int FindMappedIndexInMeshChain(stMeshModel** mesh, int key)
{
    int result;
    stMeshModel* current;

    if (key < 0 || mesh == 0 || *mesh == 0) {
        return -1;
    }

    current = *mesh;
    while (current != 0) {
        result = current->FindMappedIndex((short)key);
        if (result != -1) {
            break;
        }
        current = current->next;
    }

    *mesh = current;
    return result;
}

// FUNCTION: WIZ8 0x004a8d50
void W8GrCycle::CreateGroundShadow(int value_140, int value_13c)
{
    m_ground_shadow = new stGroundShadow(0);
    m_ground_shadow->setName("Ground Shadow");
    m_ground_shadow->value_140 = value_140;
    m_ground_shadow->value_13c = value_13c;
}

// FUNCTION: WIZ8 0x004a8de0
void W8GrCycle::SetGroundShadowVisible(char visible)
{
    if (m_ground_shadow != 0) {
        if (visible) {
            m_ground_shadow->clearFlag(srNode::FLAG_POSITIONAL_0);
        }
        else {
            m_ground_shadow->setFlag(srNode::FLAG_POSITIONAL_0);
        }
    }
}

template <>
srVector3T<double>* srVector3T<double>::method_004A90E0(
    const srVector3T<float>* source)
{
    x = (double)source->x;
    y = (double)source->y;
    z = (double)source->z;
    return this;
}

// TEMPLATE: WIZ8 0x004a90e0
// srVector3T<double>::method_004A90E0
