#include "wiz8/engine_code/SoundEvent.h"
#include "wiz8/grcycle.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/ground_shadow.h"
#include "wiz8/mesh_model.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "wiz8/vector_005ec294.h"
#include "surrender/srModelInstance.h"
#include "surrender/srCore.h"
#include <new>
#include <string.h>

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

/* The event object owns a timer at +0x18. Its implicit destructor is the
   eight-byte adjust-and-tail-jump body emitted at 0x004AE070. */
/* 0x004AE080 allocates 0x4c and hands it to the constructor at 0x004ADED0; the
   copy at 0x004AE000 establishes which of those fields carry across. The timer
   is an ordinary member, which is why the destructor at 0x004AE070 is nothing
   but its destruction. */
class W8VectorElement005ECED4 {
public:
    W8VectorElement005ECED4(const W8VectorElement005ECED4& other);

    unsigned int flags_00;               /* 0x00 */
    int value_04;                        /* 0x04 */
    int value_08;                        /* 0x08 */
    srVector3T<float> position_0c;       /* 0x0c */
    W8Timer005EC0A4 timer_18;            /* 0x18 */
    int value_3c;                        /* 0x3c */
    int value_40;                        /* 0x40 */
    int value_44;                        /* 0x44 */
    int value_48;                        /* 0x48 */
};

static_assert(sizeof(W8VectorElement005ECED4) == 0x4c,
              "W8VectorElement005ECED4_must_be_0x4c");

/* Everything except the timer's own pacing carries across, and bit zero of the
   flags is cleared so the copy is not treated as already started. The new timer
   is seeded from the source's speed rather than its remaining time. */
// FUNCTION: WIZ8 0x004ae000
W8VectorElement005ECED4::W8VectorElement005ECED4(
    const W8VectorElement005ECED4& other)
    : flags_00(other.flags_00),
      value_04(other.value_04),
      value_08(other.value_08),
      position_0c(other.position_0c),
      timer_18(other.timer_18.m_speed, 0),
      value_3c(other.value_3c),
      value_40(other.value_40),
      value_44(other.value_44),
      value_48(other.value_48)
{
    flags_00 &= ~1u;
}

// SYNTHETIC: WIZ8 0x004ae070
// W8VectorElement005ECED4::~W8VectorElement005ECED4

extern unsigned char UnregisterGrCycle(W8GrCycle* cycle);
extern void PrepareGrCycleEvents004AE270(
    W8GrowableVector<W8VectorElement005ECED4*>* events);
extern int UpdateSoundEvents004D5890(
    W8GrowableVector<W8VectorElement005ED094*>* events,
    const srVector3T<float>* position,
    unsigned int event_mask,
    int cycle,
    unsigned int frame,
    int subcycle);
extern void UpdateGrCycleEvents004AE170(
    W8GrowableVector<W8VectorElement005ECED4*>* events,
    int cycle,
    unsigned int frame,
    int subcycle,
    const srVector3T<float>* position);
extern float g_float_005ec128;
extern float g_float_005ebc64;

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
        m_plsLights = new W8LightVector(5);
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
            PListAdd(&g_world->m_list_0a8, copied_light);
            if (copied_light->definition() != 0) {
                g_world->lights_to_update->Add(copied_light);
            }
            m_plsLights->Add(copied_light);
        }
    }
    if (other.m_plsShakeEvents != 0 &&
        other.m_plsShakeEvents->GetCount() != 0) {
        count = other.m_plsShakeEvents->GetCount();
        m_plsShakeEvents = new W8GrowableVector<W8VectorElement005ECED4*>(5);
        if (m_plsShakeEvents == 0) {
            srAssertFail("m_plsShakeEvents", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0xe3, 0);
        }
        for (index = 0; index < count; ++index) {
            W8VectorElement005ECED4* event = new W8VectorElement005ECED4(
                **other.m_plsShakeEvents->GetAt(index));

            if (event == 0) {
                srAssertFail("pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0xe9, 0);
            }
            m_plsShakeEvents->Add(event);
        }
    }
    if (other.m_plsParticles != 0 && other.m_plsParticles->GetCount() != 0) {
        count = other.m_plsParticles->GetCount();
        m_plsParticles = new W8GrowableVector<W8GrCycleShakeEvent*>(5);
        if (m_plsParticles == 0) {
            srAssertFail("m_plsParticles", "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp", 0xf6, 0);
        }
        for (index = 0; index < count; ++index) {
            W8GrCycleShakeEvent* event = static_cast<W8GrCycleShakeEvent*>(
                ::operator new(sizeof(W8GrCycleShakeEvent)));

            if (event != 0) {
                W8GrCycleShakeEvent* source_event =
                    *other.m_plsParticles->GetAt(index);

                event->cycle_00 = source_event->cycle_00;
                event->subcycle_04 = source_event->subcycle_04;
                event->particle_08 =
                    new stParticle(*source_event->particle_08);
                event->position_0c = source_event->position_0c;
                memcpy(event->unknown_18, source_event->unknown_18,
                       sizeof(event->unknown_18));
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
        PrepareGrCycleEvents004AE270(m_plsShakeEvents);
        for (index = 0; index < count; ++index) {
            delete *m_plsShakeEvents->GetAt(index);
        }
        delete m_plsShakeEvents;
        m_plsShakeEvents = 0;
    }
    if (m_plsParticles != 0) {
        count = m_plsParticles->GetCount();
        for (index = 0; index < count; ++index) {
            W8GrCycleShakeEvent* event = *m_plsParticles->GetAt(index);
            stParticle* owner = event->particle_08;

            if (owner == 0 || owner->node_18c == 0) {
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
                            representation->selection.monster.current_cycle,
                            representation->flag_064,
                            representation->selection.monster.current_subcycle);
                    }
                    if (m_plsShakeEvents != 0) {
                        UpdateGrCycleEvents004AE170(
                            m_plsShakeEvents,
                            representation->selection.monster.current_cycle,
                            representation->flag_064,
                            representation->selection.monster.current_subcycle,
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
        representation->selection.monster.pending_cycle != -1 &&
        CanEnterCycle(representation->selection.monster.pending_cycle) != 0) {
        signed char pending_cycle =
            representation->selection.monster.pending_cycle;
        SetCycle(pending_cycle);
        representation->selection.monster.pending_cycle = -1;

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
                representation->selection.monster.current_cycle,
                representation->flag_064,
                representation->selection.monster.current_subcycle);
        }
        if (m_plsShakeEvents != 0) {
            UpdateGrCycleEvents004AE170(
                m_plsShakeEvents,
                representation->selection.monster.current_cycle,
                representation->flag_064,
                representation->selection.monster.current_subcycle,
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
    W8Position* minimum, W8Position* maximum)
{
    W8AnimObj* animation = GetCurrentAnimation();
    W8EmitterHost* representation = GetRepresentation();

    /* Five slots against the callee's six, preserved as found: see the note on
       the prototype in AnimObj.h. */
    return ((LegacyAnimObjBoundsCall)AnimObjGetBounds004A1710)(
        animation,
        representation->setting_98,
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
            animation, representation->setting_98);
        unsigned int index;

        for (index = 0; index < count; ++index) {
            W8PathAI* path = (W8PathAI*)AnimObjListEntry004A16C0(
                animation, representation->setting_98, (signed char)index);
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
            animation, representation->setting_98, 0);
    }

    return SelectCycleFrameLod004A8360(
        representation->selection.monster.current_cycle,
        representation->flag_064,
        (signed char)representation->setting_98);
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
            (W8PathAI*)m_pAI, (W8PathRepresentation*)target);
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
class W8VectorElement005ECED4;

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
// class W8GrowableVector<W8VectorElement005ECED4*>

// SYNTHETIC: WIZ8 0x004a8f70
// W8GrowableVector<W8VectorElement005ECED4*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004a8f90
// W8GrowableVector<W8VectorElement005ECED4*>::~W8GrowableVector<W8VectorElement005ECED4*>

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
void W8GrCycle::SetLights(W8LightVector* lights)
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
void W8GrCycle::AddVectorElement005ECED4(W8VectorElement005ECED4* element)
{
    if (m_plsShakeEvents == 0) {
        m_plsShakeEvents = new W8GrowableVector<W8VectorElement005ECED4*>();
    }
    m_plsShakeEvents->Add(element);
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

extern void WorldRemoveLight(W8World* world, srNode* light); /* 0x0046E250 */

// FUNCTION: WIZ8 0x004a8c50
void DestroyLightVector(W8LightVector* vector)
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
