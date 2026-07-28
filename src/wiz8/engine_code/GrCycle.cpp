#include "wiz8/grcycle.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/ground_shadow.h"
#include "wiz8/mesh_model.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "wiz8/vector_005ec294.h"
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

/* The pointer at W8GrCycle +0x1b0 owns this list. The construction path at
   0x004A8530 installs the growable-vector table 0x005ECED8 followed by the
   derived table 0x005ECED4, and the 44/30/17-byte destructor family proves
   the same empty-derived shape used by the other reviewed vector families.
   No source or debug witness names the element type, so it remains qualified
   by the derived vtable address. */
class W8VectorElement005ECED4;

/* Parallel registries: each name has one growable vector of cycle objects. */
extern W8GrowableVector<char*> g_grcycle_names;                       /* 0x0065BDF0 */
class W8GrCycleRegistryVector005ECEDC
    : public W8GrowableVector<W8GrCycle*> {
public:
    W8GrCycleRegistryVector005ECEDC();
    virtual ~W8GrCycleRegistryVector005ECEDC();
};

__forceinline W8GrCycleRegistryVector005ECEDC::W8GrCycleRegistryVector005ECEDC()
{
}

extern W8GrowableVector<W8GrCycleRegistryVector005ECEDC*> g_grcycles_by_name;
                                                                    /* 0x0065BE00 */

class W8Vector005ECED4
    : public W8GrowableVector<W8VectorElement005ECED4*> {
public:
    W8Vector005ECED4();
    virtual ~W8Vector005ECED4();
};                                       /* 0x10 */

__forceinline W8Vector005ECED4::W8Vector005ECED4()
{
}

// FUNCTION: WIZ8 0x004A8430
void W8GrCycle::SetSubCycle(unsigned char subcycle)
{
    signed char count = vslot5();
    W8GrCycleTarget* target = vslot9();

    if (subcycle < count) {
        target->m_subcycle = subcycle;
    }
}

// FUNCTION: WIZ8 0x004A8460
void W8GrCycle::SetBehaviour(signed char bBehaviour)
{
    W8GrCycleTarget* target = vslot9();

    if (bBehaviour < BEHAVIOUR_FIRST || bBehaviour > BEHAVIOUR_LAST) {
        srAssertFail(
            "(bBehaviour >= BEHAVIOUR_FIRST) && (bBehaviour <= BEHAVIOUR_LAST)",
            "C:\\Projects\\Wizardry 8\\Engine Code\\GrCycle.cpp",
            0x63e,
            0);
    }
    target->m_bBehaviour = bBehaviour;
}

// FUNCTION: WIZ8 0x004A84C0
void W8GrCycle::SetLights(W8Vector005EC294* lights)
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

// FUNCTION: WIZ8 0x004A8530
void W8GrCycle::AddVectorElement005ECED4(W8VectorElement005ECED4* element)
{
    if (m_vector_1b0 == 0) {
        m_vector_1b0 = new W8Vector005ECED4();
    }
    m_vector_1b0->Add(element);
}

// FUNCTION: WIZ8 0x004A8650
const char* __fastcall GetGrCycleName(W8GrCycle* cycle)
{
    int name_index;

    for (name_index = 0; name_index < g_grcycles_by_name.GetCount(); ++name_index) {
        W8GrCycleRegistryVector005ECEDC* cycles =
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

// FUNCTION: WIZ8 0x004A8700
unsigned char __fastcall IsSoleGrCycleForName(W8GrCycle* cycle)
{
    int name_index;

    for (name_index = 0; name_index < g_grcycles_by_name.GetCount(); ++name_index) {
        W8GrCycleRegistryVector005ECEDC* cycles =
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

// FUNCTION: WIZ8 0x004A87A0
W8GrCycle* FindFirstGrCycleByName(const char* name)
{
    int name_index;

    for (name_index = 0; name_index < g_grcycle_names.GetCount(); ++name_index) {
        if (_stricmp(name, *g_grcycle_names.GetAt(name_index)) == 0) {
            W8GrCycleRegistryVector005ECEDC* cycles =
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

// FUNCTION: WIZ8 0x004A8830
unsigned char UnregisterGrCycle(W8GrCycle* cycle)
{
    int name_index;

    for (name_index = 0; name_index < g_grcycles_by_name.GetCount(); ++name_index) {
        W8GrCycleRegistryVector005ECEDC* cycles =
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

// FUNCTION: WIZ8 0x004A89A0
void RegisterGrCycle(const char* name, W8GrCycle* cycle)
{
    int name_index;

    for (name_index = 0; name_index < g_grcycle_names.GetCount(); ++name_index) {
        if (_stricmp(name, *g_grcycle_names.GetAt(name_index)) == 0) {
            W8GrCycleRegistryVector005ECEDC* cycles =
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
    W8GrCycleRegistryVector005ECEDC* cycles =
        new W8GrCycleRegistryVector005ECEDC();
    cycles->Add(cycle);
    g_grcycle_names.Add(owned_name);
    g_grcycles_by_name.Add(cycles);
}

extern W8World* g_world_00659ab4;
extern void WorldRemoveLight(W8World* world, srNode* light); /* 0x0046E250 */

// FUNCTION: WIZ8 0x004A8C50
void DestroyVector005EC294(W8Vector005EC294* vector)
{
    int count;
    int index;

    if (vector != 0) {
        count = vector->GetCount();
        for (index = 0; index < count; ++index) {
            W8VectorElement005EC294* light = *vector->GetAt(index);

            if (light->world_link_234() != 0) {
                int world_index = g_world_00659ab4->lights->IndexOf(light);
                if (world_index != -1) {
                    g_world_00659ab4->lights->RemoveAt(world_index);
                }
            }
            light->setParent(0, 1);
            WorldRemoveLight(g_world_00659ab4, light);
        }
        delete vector;
    }
}

// FUNCTION: WIZ8 0x004A8D10
int FindMappedIndexInMeshChain(W8MeshModel** mesh, int key)
{
    int result;
    W8MeshModel* current;

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

// FUNCTION: WIZ8 0x004A8D50
void W8GrCycle::CreateGroundShadow(int value_140, int value_13c)
{
    m_ground_shadow = new stGroundShadow(0);
    m_ground_shadow->setName("Ground Shadow");
    m_ground_shadow->value_140 = value_140;
    m_ground_shadow->value_13c = value_13c;
}

// FUNCTION: WIZ8 0x004A8DE0
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

// FUNCTION: WIZ8 0x004A8F90
W8Vector005ECED4::~W8Vector005ECED4()
{
}
