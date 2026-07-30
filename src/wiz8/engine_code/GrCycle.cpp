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
class W8VectorElement005ECED4 {
public:
    unsigned char unknown_00[0x18];
    W8Timer005EC0A4 timer_18;
};

// SYNTHETIC: WIZ8 0x004ae070
// W8VectorElement005ECED4::~W8VectorElement005ECED4

extern unsigned char UnregisterGrCycle(W8GrCycle* cycle);
extern void PrepareGrCycleEvents004AE270(
    W8GrowableVector<W8VectorElement005ECED4*>* events);

// FUNCTION: WIZ8 0x004a5e50
W8GrCycle::W8GrCycle()
{
    current_model_instance_1a8 = 0;
    m_plsLights = 0;
    m_vector_1b0 = 0;
    m_fDeleteLights = 0;
    unknown_1b5 = 0;
    m_shake_events = 0;
    unknown_1bc = 0;
    enabled_1bd = 1;
    unknown_1be = 0;
    unknown_1bf = 0;
    m_ground_shadow = 0;
    unknown_1d4 = 0;
    scale_1cc = 1.0f;
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
    if (m_vector_1b0 != 0) {
        count = m_vector_1b0->GetCount();
        PrepareGrCycleEvents004AE270(m_vector_1b0);
        for (index = 0; index < count; ++index) {
            delete *m_vector_1b0->GetAt(index);
        }
        delete m_vector_1b0;
        m_vector_1b0 = 0;
    }
    if (m_shake_events != 0) {
        count = m_shake_events->GetCount();
        for (index = 0; index < count; ++index) {
            W8GrCycleShakeEvent* event = *m_shake_events->GetAt(index);
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
                owner->SetActive0049ACD0(1);
                owner->state_184 = 1;
                owner->active_190 = 1;
            }
        }
        delete m_shake_events;
        m_shake_events = 0;
    }
    if (m_ground_shadow != 0) {
        m_ground_shadow->release();
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

    return AnimObjGetBounds004A1710(
        animation,
        representation->setting_98,
        representation->flag_064,
        minimum,
        maximum);
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
    if (m_vector_1b0 == 0) {
        m_vector_1b0 = new W8GrowableVector<W8VectorElement005ECED4*>();
    }
    m_vector_1b0->Add(element);
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
