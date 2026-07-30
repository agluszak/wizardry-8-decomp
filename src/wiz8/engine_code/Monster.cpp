#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/combat_state.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/AniMesh.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/grcycle.h"
#include "wiz8/magic.h"
#include "wiz8/mesh_model.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/vector_005ec294.h"
#include "surrender/srTimer.h"
#include "surrender/srModelInstance.h"

#include <string.h>

extern srTimer* g_shared_timer_base;
extern unsigned char GetFlag68F105(void);
extern unsigned char FindEntityByName(
    const char* name,
    W8Position* position,
    int* value,
    W8Position* direction);
extern void SetTriggerVariableByName00444030(const char* name, int value);
extern void Function401920(const char* message);
extern char* FormatString(const char* format, ...);

#define MONSTER_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp"

// VTABLE: WIZ8 0x005ecdac
// class W8GrowableVector<float>

// VTABLE: WIZ8 0x005ed200
// class W8MonsterRep

// SYNTHETIC: WIZ8 0x004beba0
// W8MonsterRep::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004bea20
W8MonsterRep::W8MonsterRep()
    : flag_5bc(0),
      name_5c0(0),
      linked_objects_5e8(0),
      value_5ec(0),
      scale_5f0(1.0f),
      minimum_scale_5f4(0.0f),
      maximum_scale_5f8(0.0f),
      value_5fc(1.0f),
      flag_600(0),
      flag_601(0),
      value_604(10.0f),
      value_608(0),
      value_60c(0),
      value_610(0),
      monster_light_624(0)
{
    int index;

    value_5c4 = 0;
    for (index = 0; index < 8; ++index) {
        objects_5c8[index] = 0;
    }
}

// FUNCTION: WIZ8 0x004bebd0
W8MonsterRep::W8MonsterRep(const W8MonsterRep& other)
    : W8EmitterHost(other),
      flag_5bc(other.flag_5bc),
      linked_objects_5e8(0),
      value_5ec(other.value_5ec),
      scale_5f0(other.scale_5f0),
      minimum_scale_5f4(other.minimum_scale_5f4),
      maximum_scale_5f8(other.maximum_scale_5f8),
      value_5fc(other.value_5fc),
      flag_600(other.flag_600),
      flag_601(other.flag_601),
      value_604(other.value_604),
      value_608(other.value_608),
      value_60c(other.value_60c),
      value_610(other.value_610),
      monster_light_624(0)
{
    signed char cycle;
    int index;

    value_5c4 = 0;
    for (index = 0; index < 8; ++index) {
        objects_5c8[index] = 0;
    }
    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        Method004BF0F0(cycle, &other, cycle);
    }
    if (other.monster_light_624 != 0) {
        monster_light_624 = new MonsterLight(*other.monster_light_624);
    }
    name_5c0 = 0;
    if (other.name_5c0 != 0) {
        name_5c0 = new char[strlen(other.name_5c0) + 1];
        if (name_5c0 != 0) {
            strcpy(name_5c0, other.name_5c0);
        }
    }
}

extern void DestroyAnimObj004A01E0(W8AnimObj* animation);
extern W8AnimObj* CloneAnimObj004A0320(const W8AnimObj* animation);

// FUNCTION: WIZ8 0x004bee50
W8MonsterRep::~W8MonsterRep()
{
    int cycle;
    int index;

    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int count = animations[cycle].GetCount();
        for (index = 0; index < count; ++index) {
            W8AnimObj* animation = *animations[cycle].GetAt(index);
            if (animation != 0) {
                DestroyAnimObj004A01E0(animation);
            }
        }
    }
    for (index = 0; index < 8; ++index) {
        delete objects_5c8[index];
    }
    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int count = light_lists[cycle].GetCount();
        for (index = 0; index < count; ++index) {
            DestroyLightVector(*light_lists[cycle].GetAt(index));
        }
        light_lists[cycle].Clear();
    }
    while (linked_runtime_objects_614.GetCount() != 0) {
        delete linked_runtime_objects_614.RemoveAt(0);
    }
    delete monster_light_624;
    delete[] name_5c0;
}

/* Deep-copy one cycle's animation objects and render lights while retaining
   its per-subcycle scalar values.  The light copies are new scene objects:
   they are registered with the world's light list and detached until the
   owning GrCycle selects this cycle. */
// FUNCTION: WIZ8 0x004bf0f0
void W8MonsterRep::Method004BF0F0(
    signed char cycle,
    const W8MonsterRep* other,
    signed char other_cycle)
{
    int index;

    for (index = 0; index < other->animations[other_cycle].GetCount(); ++index) {
        animations[cycle].Add(
            CloneAnimObj004A0320(*other->animations[other_cycle].GetAt(index)));
        animation_scales[cycle].Add(
            *other->animation_scales[other_cycle].GetAt(index));
    }

    for (index = 0; index < other->light_lists[other_cycle].GetCount(); ++index) {
        W8LightVector* source_lights =
            *other->light_lists[other_cycle].GetAt(index);
        W8LightVector* copied_lights = 0;

        if (source_lights != 0) {
            int light_index;

            copied_lights = new W8LightVector;
            if (copied_lights == 0) {
                srAssertFail(
                    "plsNewLights",
                    MONSTER_CPP,
                    0x1e5,
                    "Out of memory creating monster light list");
            }
            for (light_index = 0;
                 light_index < source_lights->GetCount();
                 ++light_index) {
                stLight* source_light = *source_lights->GetAt(light_index);
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
                        MONSTER_CPP,
                        0x1ed,
                        "Out of memory creating monster light");
                }
                copied_light->ConfigureMonsterCopy();
                copied_light->setLocation(x, y, z);
                copied_light->setParent(0, 0);
                PListAdd(&g_world->m_list_0a8, copied_light);
                copied_lights->Add(copied_light);
            }
        }
        light_lists[cycle].Add(copied_lights);
    }
}

/* The representation clone slot is an ordinary virtual copy operation.  The
   allocation size and call to the copy constructor are both visible in the
   emitted body; there is no separate representation wrapper involved. */
// FUNCTION: WIZ8 0x004ca9e0
W8AnimRepBase005EC1D8* W8MonsterRep::Clone()
{
    return new W8MonsterRep(*this);
}

struct W8Forwarded {
    void Method4C5290();
};

extern "C" {
extern void Function4C4EF0(void);
extern void Function4A7A70(int value);
extern unsigned char g_flag_6081e4;
extern int g_value_659c14;
}

// VTABLE: WIZ8 0x005ed22c W8Monster
// VTABLE: WIZ8 0x005ed218 W8Navigator
// class W8Monster

// SYNTHETIC: WIZ8 0x004bfde0
// W8Monster::`scalar deleting destructor'

/* cvdump preserves a terminal space in this generated thunk's demangled name;
   the explicit name reference must preserve it too. */
// SYNTHETIC: WIZ8 0x004cae30
// W8Monster::`vector deleting destructor'`adjustor{24}' 

extern int g_monster_cycle_registry_weight_0065ba4c;
extern void PrepareMonsterCycleForDestruction004ACF90(
    W8Monster* cycle);
extern void __fastcall RefreshMonsterCycleRegistry004C6B10(
    W8Monster* cycle);
extern unsigned char Function420E10(void);
extern unsigned char g_flag_00689b32;

// FUNCTION: WIZ8 0x004bfb00
W8Monster::W8Monster()
{
    memset(&value_1e0, 0, 0x58);
    memset(&state_28c, 0, sizeof(state_28c));
    memset(&state_2ac, 0, sizeof(state_2ac));
    memset(&state_2fc, 0, sizeof(state_2fc));
    memset(&flags_330, 0, sizeof(flags_330));

    flags_1dc = 0;
    value_1e0 = -1;
    propagated_value_1e4 = -1;
    value_1e8 = 1.0f;
    value_1ec = 1.0f;
    value_1f0 = 1.0f;
    value_210 = -1;
    object_238 = 0;
    value_23c = 0;
    value_240 = -1;
    value_278 = 0;
    registry_weight_27c = 0;
    formation.value_00 = 0;
    formation.value_04 = 0;
    formation.value_08 = 0;
    object_334 = 0;

    m_pRep = new W8MonsterRep;
    m_pRep->linked_objects_5e8 = PListCreate();
}

// FUNCTION: WIZ8 0x004bfe00
W8Monster::W8Monster(const W8Monster& rhs)
    : W8GrCycle(rhs),
      flags_1dc(rhs.flags_1dc),
      value_1e0(rhs.value_1e0),
      propagated_value_1e4(rhs.propagated_value_1e4),
      value_1e8(1.0f),
      value_1ec(1.0f),
      value_1f0(1.0f),
      value_1f4(rhs.value_1f4),
      value_1f8(rhs.value_1f8),
      flag_1fc(0),
      flag_1fd(0),
      value_200(0),
      value_204(0),
      value_208(0),
      value_210(-1),
      flag_215(rhs.flag_215),
      flag_216(1),
      flag_217(0),
      flag_218(0),
      value_21c(rhs.value_21c),
      value_220(rhs.value_220),
      value_224(rhs.value_224),
      value_228(rhs.value_228),
      flag_22c(0),
      flag_22d(0),
      object_238(0),
      value_240(-1),
      value_278(0),
      registry_weight_27c(rhs.registry_weight_27c),
      object_334(0)
{
    formation.value_00 = 0;
    formation.value_04 = 0;
    formation.value_08 = 0;
    flags_330.flag_00 = 0;
    flags_330.flag_01 = 0;
    flags_330.copied_flag_02 = rhs.flags_330.copied_flag_02;

    if (rhs.m_pRep == 0) {
        srAssertFail(
            "rhs.m_pRep",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            1099,
            0);
    }
    m_pRep = static_cast<W8MonsterRep*>(rhs.m_pRep->Clone());
    m_pRep->linked_objects_5e8 = PListCreate();

    state_28c.flag_00 = 0;
    state_28c.flag_01 = 0;
    state_28c.value_02 = -1;
    state_28c.flag_03 = 0;
    state_28c.flag_04 = 0;
    state_28c.flag_05 = 0;
    state_2ac.flag_00 = 0;
    state_2ac.value_04 = 0;
    state_2ac.value_08 = 0;
    state_2ac.value_0c = 0;
    state_2ac.value_1c = 0;
    state_2ac.value_20 = 0;
    state_2ac.value_24 = -1;
    state_2ac.flag_28 = 1;
    state_2fc.scale_00 = 1.0f;
    state_2fc.scale_04 = 1.0f;
    flags_1dc &= 0xfffffcb6;
    state_22e = 0;
    unknown_230[0] = 0;
    if (flags_330.copied_flag_02 != 0) {
        fields.state_088 = 0;
    }
}

// FUNCTION: WIZ8 0x004c0170
W8Monster::~W8Monster()
{
    SetLights(0);
    if (m_pRep->linked_objects_5e8 != 0) {
        PrepareMonsterCycleForDestruction004ACF90(this);
        PListDestroy(m_pRep->linked_objects_5e8);
    }
    if (IsSoleGrCycleForName(this)) {
        g_monster_cycle_registry_weight_0065ba4c -= registry_weight_27c;
        RefreshMonsterCycleRegistry004C6B10(this);
    }
    UnregisterGrCycle(this);
    delete m_pRep;
    if (object_238 != 0) {
        object_238->release();
        object_238 = 0;
    }
    flags_1dc &= ~0x20;
    value_23c = 0;
    value_240 = -1;
    if (object_334 != 0) {
        object_334->release();
        object_334 = 0;
    }
}

/* Navigator is W8Monster's second base at +0x18. VC6 places this override in
   that secondary table and emits the adjusted entry form at 0x004CA840. */
// FUNCTION: WIZ8 0x004ca840
void W8Monster::SetPosition(const W8Position* position)
{
    GetRepresentation()->SetLocation004B8850(position);
    m_pRep->SetLocation004B8850(position);
    SetPositionInternal00453590(position);
    fields.position_dirty_09c = 1;
}

/* Decide whether a requested cycle can replace the current one. Monster adds
   combat, motionless, and death rules to W8GrCycle's primary-table operation;
   W8Navigator's distinct slot 3 remains inherited in the secondary table. */
// FUNCTION: WIZ8 0x004c2bf0
unsigned char W8Monster::CanEnterCycle(signed char cycle)
{
    unsigned int monster_index = MonsterGetIndexByLocationID(
        0x969, MONSTER_CPP, propagated_value_1e4, 1);
    W8MonsterInfo* monster_info =
        MonsterGetScriptPartByLocationIndex(monster_index);

    if (g_in_combat_00683f94 != 0 && Function420E10() != 0) {
        return 0;
    }
    if (m_pRep->flag_06d == 0) {
        if (cycle != 0x14 && cycle != 0x15 && cycle != 0 &&
            monster_info->motionless != 0) {
            if (g_flag_00689b32 == 0) {
                return 0;
            }
            srAssertFail("FALSE", MONSTER_CPP, 0x97f, 0);
            return 0;
        }
    } else {
        if (IsCycleInterruptable((signed char)Query(6)) == 0 && Query(7) == 0) {
            return 0;
        }
        if (cycle == 0x15 && monster_info->monster_species == 0x199 &&
            Query(2) == 0) {
            return 0;
        }
    }
    return 1;
}

/* Report whether a cycle may be interrupted. The original diagnostic names
   this operation `Monster::CycleInterruptable`; its spelling is retained here
   because it is the only source-level name available. */
// FUNCTION: WIZ8 0x004c2cf0
unsigned char W8Monster::IsCycleInterruptable(signed char cycle)
{
    signed char current_cycle;
    signed char pending_cycle;
    const char* pending_name;
    const char* current_name;
    const char* requested_name;

    if (GetFlag68F105() != 0) {
        return 1;
    }
    if (m_pRep->flag_06d == 0) {
        current_cycle = (signed char)Query(6);
        pending_cycle = m_pRep->selection.monster.pending_cycle;
        if (pending_cycle != -1 && CanEnterCycle(pending_cycle) != 0) {
            pending_name = g_cycle_names[pending_cycle].name;
            if (current_cycle != -1) {
                current_name = g_cycle_names[current_cycle].name;
            } else {
                current_name = "";
            }
            if (cycle != -1) {
                requested_name = g_cycle_names[cycle].name;
            } else {
                requested_name = "";
            }
            FormatDebugMessage(
                1,
                "Monster::CycleInterruptable (ID %d) - WARNING: Monster is "
                "not animating - Cycle %d(%s), current %d(%s), pending "
                "%d(%s)",
                propagated_value_1e4,
                (int)cycle,
                requested_name,
                (int)current_cycle,
                current_name,
                (int)pending_cycle,
                pending_name);
        }
        return 1;
    }

    switch (cycle) {
    case -1:
    case 1:
    case 2:
    case 3:
    case 4:
    case 0x16:
        return 1;
    }
    return 0;
}

/* Apply the two script-specific side effects selected before an ungrouped
   monster is removed: state two returns it to Balbrak's home marker, while
   state three clears the ScregActive trigger variable. */
// FUNCTION: WIZ8 0x004c50f0
void W8Monster::ApplyRemovalStateEffects()
{
    W8Position position;

    switch (state_22e) {
    case 2:
        if (FindEntityByName("NP_Balbrakhome", &position, 0, 0) != 0) {
            SetPosition(&position);
        }
        break;
    case 3:
        SetTriggerVariableByName00444030("ScregActive", 0);
        break;
    }
}

/* Engine Code\Monster.cpp. CYCLE_NUM_UNIQUE and the method name both come from
   the canonical assertion at line 960, whose message reads
   "GetNumSubsPerCycle() -> Invalid cycle num.". The element count and stride
   agree with the reviewed constructor: 27 entries of 0x10 bytes at 0xAC ends at
   0x25C, exactly where Monster's second vector array begins. */
// FUNCTION: WIZ8 0x004bfab0
unsigned char W8MonsterRep::GetNumSubsPerCycle(signed char bCycle)
{
    if (bCycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle < CYCLE_NUM_UNIQUE",
            MONSTER_CPP,
            0x3c0,
            "GetNumSubsPerCycle() -> Invalid cycle num.");
    }
    if (bCycle == -1) {
        bCycle = selection.monster.current_cycle;
    }
    return (unsigned char)animations[bCycle].GetCount();
}

/* Select the active AnimObj for a cycle and dispatch the requested LOD/frame.
   This is the concrete implementation behind AnimRep's third vtable slot. */
// FUNCTION: WIZ8 0x004bf8c0
srModelInstance* W8MonsterRep::SetCycleFrameLod(
    signed char cycle, int frame, int lod)
{
    int subcycle = selection.monster.current_subcycle;
    W8MonsterAnimationVector* selected_cycle = &animations[cycle];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (subcycle < selected_cycle->GetCount()) {
        animation_slot = selected_cycle->data + subcycle;
    }
    else {
        animation_slot = selected_cycle->data;
    }
    animation = *animation_slot;
    if (animation->flag_05 == 0) {
        return AnimObjDispatch004A14D0(animation, (signed char)lod, frame);
    }
    return AnimObjDispatchList004A1560(animation, (signed char)lod, 0);
}

/* Stop the selected subcycle for one animation cycle.  AnimObj's canonical
   body takes the three stack arguments emitted here; keep that call-site ABI
   local until the older four-parameter declaration is corrected as its own
   bundle. */
// FUNCTION: WIZ8 0x004bf920
void W8MonsterRep::StopEmitter(char cycle)
{
    typedef void* (__cdecl *LegacyAnimObjEntryCall)(
        W8AnimObj*, signed char, unsigned int);

    W8AnimObj* animation =
        *animations[cycle].GetAt(selection.monster.current_subcycle);

    if (animation != 0) {
        ((LegacyAnimObjEntryCall)AnimObjEntry004A1660)(
            animation, setting_98, 0);
    }
}

/* The Monster vtable's slot-three method selects the active subcycle's
   AnimObj (falling back to entry zero) and submits the Monster's current
   animation index.  The assertion's `pao` spelling establishes the pointee's
   AnimObj identity without supplying a name for this Monster method. */
// FUNCTION: WIZ8 0x004bf970
unsigned int W8MonsterRep::ApplyEmitterSetting(char cycle)
{
    W8MonsterAnimationVector* selected_cycle = &animations[cycle];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (selection.monster.current_subcycle < selected_cycle->GetCount()) {
        animation_slot = selected_cycle->data +
            selection.monster.current_subcycle;
    } else {
        animation_slot = selected_cycle->data;
    }
    animation = *animation_slot;
    if (animation == 0) {
        srAssertFail(
            "pao",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0x2de,
            0);
    }
    return AnimObjValue004A15D0(
        animation, setting_98);
}

// FUNCTION: WIZ8 0x004caa40
signed char W8Monster::GetNumSubCycles()
{
    W8MonsterRep* representation = m_pRep;
    W8MonsterAnimationVector* cycle = &representation->animations[
        representation->selection.monster.current_cycle];
    W8AnimObj** slot = cycle->data;
    int subcycle = representation->selection.monster.current_subcycle;

    if (subcycle < cycle->count) {
        slot += subcycle;
    }

    return (signed char)AnimObjValue004A15D0(
        *slot, representation->setting_98);
}

/* W8Monster stores its animation object immediately after the shared
   0x1d8-byte GrCycle base. */
// FUNCTION: WIZ8 0x004c3740
unsigned char W8Monster::IsCycleSupported(signed char cycle)
{
    if (cycle >= W8_MONSTER_CYCLE_COUNT) {
        srAssertFail(
            "bCycle < CYCLE_NUM_UNIQUE",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0xafc,
            "IsCycleSupported() -> Invalid cycle num.");
    }
    return m_pRep->animations[cycle].GetCount() != 0;
}

// FUNCTION: WIZ8 0x004c3dd0
signed char W8Monster::GetTotalAnimationCount()
{
    signed char total = 0;
    int cycle;

    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        total += (signed char)m_pRep->animations[cycle].GetCount();
    }
    return total;
}

// FUNCTION: WIZ8 0x004caa90
float W8Monster::GetCurrentAnimationScale()
{
    W8MonsterRep* representation = m_pRep;

    return *representation->animation_scales[
        representation->selection.monster.current_cycle].GetAt(
            representation->selection.monster.current_subcycle);
}

// FUNCTION: WIZ8 0x004cab00
W8EmitterHost* W8Monster::GetRepresentation()
{
    return m_pRep;
}

// FUNCTION: WIZ8 0x004c3df0
unsigned char W8Monster::GetAnimationBounds(
    W8Position* minimum, W8Position* maximum)
{
    unsigned char result;
    float scale;

    result = W8GrCycle::GetAnimationBounds(minimum, maximum);
    scale = m_pRep->scale_5f0;
    minimum->x *= scale;
    minimum->y *= scale;
    minimum->z *= scale;
    scale = m_pRep->scale_5f0;
    maximum->x *= scale;
    maximum->y *= scale;
    maximum->z *= scale;
    return result;
}

// FUNCTION: WIZ8 0x004c3ed0
unsigned char W8Monster::GetAnimationRadius(float* radius)
{
    unsigned char result = W8GrCycle::GetAnimationRadius(radius);

    *radius *= m_pRep->scale_5f0;
    return result;
}

static const float g_monster_bounds_vertical_factor_005ecd88 = 0.66f;

// FUNCTION: WIZ8 0x004c3e60
unsigned char W8Monster::GetAnimationCenter(W8Position* center)
{
    W8Position minimum;
    W8Position maximum;

    if (GetAnimationBounds(&minimum, &maximum) != 0) {
        srVector3T<float> position = GetPosition();

        center->x = position.x;
        center->y = position.y;
        center->z = position.z;
        center->y += (maximum.y - minimum.y) *
            g_monster_bounds_vertical_factor_005ecd88;
        return 1;
    }
    return 0;
}

/* Query the current animation state. The selector is an internal ten-entry
   interface used by MonsterManager and the animation driver; selector eight is
   intentionally unsupported and returns -1 with out-of-range selectors. */
// FUNCTION: WIZ8 0x004c4660
int W8Monster::Query(int query)
{
    int result = -1;
    unsigned int animation_value;

    switch (query) {
    case 0:
        result = m_pRep->ApplyEmitterSetting(
            m_pRep->selection.monster.current_cycle);
        break;
    case 1:
        result = GetTotalAnimationCount();
        break;
    case 2:
        if (m_pRep->flag_06e != 1 && m_pRep->flag_06e != 2) {
            result = m_pRep->flag_064 == 0;
            break;
        }
        animation_value = m_pRep->ApplyEmitterSetting(
            m_pRep->selection.monster.current_cycle);
        result = m_pRep->flag_064 == animation_value - 1;
        break;
    case 3:
        if (m_pRep->flag_06e == 1 || m_pRep->flag_06e == 2) {
            result = m_pRep->flag_064 == 0;
            break;
        }
        animation_value = m_pRep->ApplyEmitterSetting(
            m_pRep->selection.monster.current_cycle);
        result = m_pRep->flag_064 == animation_value - 1;
        break;
    case 4:
        result = m_pRep->flag_064;
        break;
    case 5:
        result = m_pRep->ApplyEmitterSetting(
            m_pRep->selection.monster.current_cycle) != (unsigned int)-1;
        break;
    case 6:
        result = m_pRep->selection.monster.current_cycle;
        break;
    case 7:
        if (m_pRep->flag_070 == 3) {
            if (m_pRep->flag_06d == 0) {
                result = 1;
            }
            break;
        }

        animation_value = m_pRep->ApplyEmitterSetting(
            m_pRep->selection.monster.current_cycle);
        if ((m_pRep->flag_06e == 1 &&
             m_pRep->flag_064 == animation_value - 1) ||
            (m_pRep->flag_06e == 3 && m_pRep->flag_064 == 0) ||
            m_pRep->flag_06e == 4 || m_pRep->flag_06e == 2) {
            result = 1;
        }
        break;
    case 9:
        result = m_pRep->selection.monster.current_subcycle;
        break;
    }
    return result;
}

// FUNCTION: WIZ8 0x004c32e0
void W8Monster::AdvanceAnimationFrame(int value, int)
{
    unsigned char previous_frame = m_pRep->flag_064;

    W8GrCycle::AdvanceAnimationFrame(value, 0);
    if ((m_pRep->selection.monster.current_cycle == 7 ||
         m_pRep->selection.monster.current_cycle == 13 ||
         m_pRep->selection.monster.current_cycle == 17) &&
        ((value_1f4 > 0 && previous_frame < value_1f4 &&
          value_1f4 <= m_pRep->flag_064) ||
         (value_1f4 == 0 && m_pRep->flag_064 == 1))) {
        HandleAnimationThreshold004C75C0();
    }
    HandleAnimationFrame004C74D0(previous_frame);
    if (m_shake_events != 0 && m_shake_events->GetCount() != 0) {
        UpdateShakeEvents004C3380(previous_frame);
    }
}

extern unsigned int MonsterGetIndexByLocationID(
    int caller_line, const char* caller_file, int location_id,
    unsigned char assert_on_failure);
extern W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int index);
extern unsigned int MonsterCastsSpell(
    W8MonsterInfo* monster_info, int spell_id, unsigned int power_level);
extern void FatigueMonster(
    W8MonsterInfo* monster_info, unsigned int amount, int report_to);
extern int g_spell_effect_frame_0064c158;
extern int g_spell_index_0069b7dc;
extern void* CreateSpellEffect004AD8A0(
    const char* mls_name, int frame, W8Monster* parent, int value, int flags);
extern void SetTargetSourceToMonster(
    const W8MonsterInfo* monster_info, W8TargetSource* source);
extern void ClearAttackBlock(void* block);
extern unsigned int ChooseAttackMode(unsigned int attack_modes);
extern int CalculateMonsterMissileAccuracy(
    W8MonsterInfo* monster_info, const W8MonsterAttack* attack,
    int attack_mode, int flags);
extern void CombatLog(const char* format, ...);
extern void FireMissileSourceToTarget(
    int missile_type, W8TargetSource* source, W8CombatSlot* target,
    void* attack_block, unsigned char use_default_accuracy,
    unsigned int range_category, int accuracy);
extern unsigned int g_missile_table_count_65bddc;

// VTABLE: WIZ8 0x005ed288
// class W8MonsterShakeCallback

// VTABLE: WIZ8 0x005ed290
// class W8MonsterShakeCallbackBase

// SYNTHETIC: WIZ8 0x004c3710
// W8MonsterShakeCallback::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x004c3730
// W8MonsterShakeCallback::~W8MonsterShakeCallback

// SYNTHETIC: WIZ8 0x004cab40
// W8MonsterShakeCallbackBase::`scalar deleting destructor'

/* Cycle 25 launches either the queued spell visual or the monster's pending
   spell action when its animation crosses the configured frame. The cast
   returns the stamina charge; passing that value straight to FatigueMonster
   is why MonsterCastsSpell cannot have the void return type previously used
   by Magic.cpp. */
// FUNCTION: WIZ8 0x004c74d0
void W8Monster::HandleAnimationFrame004C74D0(unsigned char previous_frame)
{
    W8MonsterInfo* monster_info;
    int action_kind;
    int action_detail;
    unsigned int power_level;
    unsigned int fatigue;

    if (m_pRep->selection.monster.current_cycle == 25 &&
        ((value_1f8 > 0 && previous_frame < value_1f8 &&
          value_1f8 <= m_pRep->flag_064) ||
         (value_1f8 == 0 && m_pRep->flag_064 == 1))) {
        if (state_2fc.unknown_08[0] != 0) {
            state_2fc.unknown_08[0] = 0;
            CreateSpellEffect004AD8A0(
                g_spell_records[g_spell_index_0069b7dc].resource_name,
                g_spell_effect_frame_0064c158, this, 0, 0);
            return;
        }

        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(
                0x1804, MONSTER_CPP, propagated_value_1e4, 1));
        action_kind = monster_info->action_kind;
        action_detail = monster_info->action_detail;
        power_level = *(unsigned int*)monster_info->unknown_2e9;
        if (action_kind == 2 && action_detail != 0 && power_level != 0) {
            fatigue = MonsterCastsSpell(
                monster_info, action_detail, power_level);
            FatigueMonster(monster_info, fatigue, 0);
            monster_info->unknown_2df[1] = 1;
        }
    }
}

struct W8MonsterMissileAttackBlock {
    int unknown_00;
    int missile_value_04;
    unsigned char missile_values_08[0x10];
    int monster_value_18;
    int missile_value_1c;
    unsigned char unknown_20[0x10];
};

static_assert(
    sizeof(W8MonsterMissileAttackBlock) == 0x30,
    "W8MonsterMissileAttackBlock_size_must_be_0x30");

/* Launch the missile at the frame shared by attack cycles 7, 13 and 17. In
   combat an already-selected attack is reused; otherwise the monster picks a
   live character and the first database attack that permits a missile mode. */
// FUNCTION: WIZ8 0x004c75c0
void W8Monster::HandleAnimationThreshold004C75C0()
{
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;
    W8TargetSource source;
    W8MonsterMissileAttackBlock attack_block;
    const W8MonsterAttack* attack;
    unsigned int attack_index;
    unsigned int range_category;
    int missile_type;
    int accuracy;
    unsigned char selected_attack;
    unsigned char monster_value;

    selected_attack = 0;
    monster_info = MonsterGetScriptPartByLocationIndex(
        MonsterGetIndexByLocationID(
            0x1834, MONSTER_CPP, propagated_value_1e4, 1));
    record = GetMonsterDataForInfo(monster_info);
    SetTargetSourceToMonster(monster_info, &source);

    if (g_in_combat_00683f94 != 0 &&
        g_combat_state->selected_slot == 2 &&
        g_combat_state->selected_monster == monster_info) {
        attack_index = monster_info->pCombat->attack_index_11;
        selected_attack = 1;
        goto prepare_attack;
    }

    monster_info->combat_slot_2ba.iType = W8_TARGET_KIND_CHARACTER;
    monster_info->combat_slot_2ba.iChar = GetRandomCharacter(1, 1, -1, -1);
    monster_info->combat_slot_2ba.iMonsterID = -1;
    if (monster_info->combat_slot_2ba.iChar == -1) {
        return;
    }

    for (attack_index = 0; attack_index < W8_MAX_MONSTER_ATTACKS;
         ++attack_index) {
        attack = &record->attacks[attack_index];
        if (attack->fHasAttack != 0 &&
            (attack->attack_modes & 0x110) != 0) {
            monster_info->action_detail = ChooseAttackMode(attack->attack_modes);
            goto prepare_attack;
        }
    }

    missile_type = 0;
    range_category = 3;
    ClearAttackBlock(&attack_block);
    accuracy = 50;
    goto fire_missile;

prepare_attack:
    if (attack_index >= W8_MAX_MONSTER_ATTACKS) {
        srAssertFail(
            "uiAttack < MAX_MONSTER_ATTACKS", MONSTER_CPP, 0x1862, 0);
    }
    attack = &record->attacks[attack_index];
    missile_type = attack->missile_type;
    if ((unsigned int)missile_type >= g_missile_table_count_65bddc) {
        FormatDebugMessage(
            0, "WARNING: %ls has invalid missile type %d for attack %d",
            record, missile_type, attack_index);
        missile_type = 0;
    }

    range_category = attack->range_category;
    ClearAttackBlock(&attack_block);
    attack_block.missile_value_04 = attack->missile_value_17;
    memcpy(attack_block.missile_values_08, attack->missile_values_05, 0x10);
    monster_value = record->missile_value_24f;
    attack_block.monster_value_18 =
        monster_value + (monster_value < 15 ? monster_value : 15);
    attack_block.missile_value_1c = attack->missile_value_1b;

    if (selected_attack != 0) {
        accuracy = CalculateMonsterMissileAccuracy(
            monster_info, attack, monster_info->action_detail, 0);
        CombatLog("TO HIT: MISSILE ACCURACY = %d%%\n", accuracy);
    } else {
        accuracy = 50;
    }

fire_missile:
    monster_info->unknown_2df[0] = 1;
    FireMissileSourceToTarget(
        missile_type, &source, &monster_info->combat_slot_2ba, &attack_block,
        selected_attack == 0, range_category, accuracy);
}

// FUNCTION: WIZ8 0x004c3620
void W8MonsterShakeCallback::RestoreAnimation()
{
    W8MonsterRep* representation;

    if (m_pMonster == 0) {
        srAssertFail("m_pMonster", MONSTER_CPP, 0x102, 0);
    }
    if (m_pParticles == 0) {
        srAssertFail("m_pParticles", MONSTER_CPP, 0x103, 0);
    }

    m_pParticles->callback_26c = 0;
    m_pParticles->SetActive0049ACD0(0);
    representation = m_pMonster->m_pRep;
    if (saved_behaviour < 1 || saved_behaviour > 3) {
        srAssertFail(
            "bBehaviour >= BEHAVIOUR_FIRST && bBehaviour <= BEHAVIOUR_LAST",
            "..\\Engine Code\\Include\\AnimRep.hpp", 0x87, 0);
    }
    representation->behaviour_071 = saved_behaviour;
    representation->SetFrameMethod004B55C0(saved_frame_method);
    representation->flag_06e = 1;
    representation->counter_094 = 0;
    representation->counter_095 = m_pMonster->GetNumSubCycles() - 1;
    delete this;
}

/* Drive the particles attached to the active cycle/subcycle. A particle with
   no distinct frame range fires when the animation crosses its own start
   frame; a ranged particle is switched on and off at its explicit bounds. */
// FUNCTION: WIZ8 0x004c3380
void W8Monster::UpdateShakeEvents004C3380(unsigned char previous_frame)
{
    W8AnimObj* animation;
    W8GrCycleShakeEvent* event;
    stParticle* particle;
    W8MonsterShakeCallback* callback;
    int count;
    int index;
    unsigned char animation_has_range;

    count = m_shake_events->GetCount();
    animation = *m_pRep->animations[
        m_pRep->selection.monster.current_cycle].GetAt(
            m_pRep->selection.monster.current_subcycle);
    animation_has_range =
        animation != 0 && animation->start_frame_14 != 0 &&
        animation->end_frame_15 >= animation->start_frame_14;

    for (index = 0; index < count; ++index) {
        event = *m_shake_events->GetAt(index);
        if (event->cycle_00 != m_pRep->selection.monster.current_cycle ||
            event->subcycle_04 !=
                m_pRep->selection.monster.current_subcycle) {
            continue;
        }

        particle = event->particle_08;
        if (animation_has_range != 0 &&
            (particle->start_frame_264 == -1 ||
             particle->end_frame_268 == -1 ||
             particle->start_frame_264 == particle->end_frame_268) &&
            previous_frame < animation->start_frame_14 &&
            animation->start_frame_14 <= m_pRep->flag_064) {
            if (enabled_1bd != 0) {
                particle->SetActive0049ACD0(1);
                particle->value_188 = 0;
                if (index == 0) {
                    callback = new W8MonsterShakeCallback;
                    callback->m_pMonster = this;
                    callback->m_pParticles = particle;
                    callback->saved_behaviour = m_pRep->flag_070;
                    callback->saved_frame_method = m_pRep->flag_06f;
                    particle->callback_26c = callback;

                    m_pRep->behaviour_071 = 3;
                    if (animation->start_frame_14 == animation->end_frame_15) {
                        m_pRep->SetFrameMethod004B55C0(4);
                        m_pRep->flag_06e = 1;
                    } else {
                        m_pRep->SetFrameMethod004B55C0(2);
                        m_pRep->counter_094 = animation->start_frame_14;
                        if (animation->end_frame_15 < GetNumSubCycles()) {
                            m_pRep->counter_095 = animation->end_frame_15;
                        } else {
                            m_pRep->counter_095 = GetNumSubCycles() - 1;
                        }
                    }
                }
            }
            continue;
        }

        if (particle->start_frame_264 != -1 &&
            particle->end_frame_268 != -1 &&
            particle->start_frame_264 != particle->end_frame_268) {
            if ((unsigned int)previous_frame ==
                (unsigned int)particle->start_frame_264) {
                if (enabled_1bd != 0) {
                    particle->SetActive0049ACD0(1);
                    particle->value_188 = 0;
                }
            } else if ((unsigned int)m_pRep->flag_064 ==
                       (unsigned int)particle->end_frame_268) {
                particle->SetActive0049ACD0(0);
            }
        }
    }
}

// FUNCTION: WIZ8 0x004cab10
W8AnimObj* W8Monster::GetCurrentAnimation()
{
    W8MonsterRep* representation = m_pRep;

    return *representation->animations[
        representation->selection.monster.current_cycle].GetAt(
            representation->selection.monster.current_subcycle);
}

// FUNCTION: WIZ8 0x004caac0
void W8Monster::SetCurrentAnimationScale(float scale)
{
    W8MonsterRep* representation = m_pRep;

    *representation->animation_scales[
        representation->selection.monster.current_cycle].GetAt(
            representation->selection.monster.current_subcycle) = scale;
}

/* Resolve the active cycle/subcycle AnimObj and submit entry zero using the
   Monster's animation index. */
// FUNCTION: WIZ8 0x004c3f00
W8AniMesh* W8Monster::GetCurrentAniMesh()
{
    typedef void* (__cdecl *LegacyAnimObjEntryCall)(
        W8AnimObj*, signed char, unsigned int);

    int cycle_index =
        m_pRep->selection.monster.current_cycle;
    int subcycle_index =
        m_pRep->selection.monster.current_subcycle;
    W8MonsterAnimationVector* cycle = &m_pRep->animations[cycle_index];
    W8AnimObj** animation_slot;
    W8AnimObj* animation;

    if (subcycle_index < cycle->GetCount()) {
        animation_slot = cycle->data + subcycle_index;
    } else {
        animation_slot = cycle->data;
    }
    animation = *animation_slot;
    if (animation == 0) {
        srAssertFail(
            "pao",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0xc4e,
            0);
    }
    /* This canonical caller passes the legacy three-argument call shape to
       0x004A1660 even though that callee's own body has a four-slot prototype.
       Preserve the observed caller ABI locally rather than weakening the
       callee's reviewed declaration. */
    return (W8AniMesh*)((LegacyAnimObjEntryCall)AnimObjEntry004A1660)(
        animation, fields.animation_index_080, 0);
}

/* Store one value in the two cycle records used as its compact mirrors, then
   propagate it to every attached object's +0x28 field.  The body consumes two
   cdecl arguments; callers that reserve another stack slot clean it themselves. */
// FUNCTION: WIZ8 0x004c5870
void MonsterPropagateValue004C5870(W8Monster* monster, int value)
{
    int index;
    int count;

    if (monster == 0) {
        srAssertFail(
            "pMonster",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Monster.cpp",
            0x125f,
            0);
    }
    monster->propagated_value_1e4 = value;
    monster->fields.location_id_0c4 = (unsigned short)value;
    if (monster->m_plsSoundEvents != 0) {
        count = monster->m_plsSoundEvents->GetCount();
        index = 0;
        if (count > 0) {
            do {
                int propagated_value = monster->propagated_value_1e4;
                W8VectorElement005ED094* object =
                    *monster->m_plsSoundEvents->GetAt(index);
                ++index;
                object->value_028 = propagated_value;
            } while (index < count);
        }
    }
}

// FUNCTION: WIZ8 0x004c5710
bool MonsterHasPendingCycle(W8Monster* monster)
{
    return monster->m_pRep->selection.monster.pending_cycle != -1;
}

/* Cycle 17's third state byte is preserved by ActivateMonster while the live
   engine object is rebuilt, then restored into the replacement. */
// FUNCTION: WIZ8 0x004c57f0
unsigned char MonsterGetCycle17State(W8Monster* monster)
{
    return monster->unknown_1be;
}

// FUNCTION: WIZ8 0x004c5800
void MonsterSetCycle17State(W8Monster* monster, unsigned char state)
{
    monster->unknown_1be = state;
}

// FUNCTION: WIZ8 0x004c5820
unsigned char MonsterGetRuntimeFlag5BC(W8Monster* monster)
{
    return monster->m_pRep->flag_5bc;
}

// FUNCTION: WIZ8 0x004c5840
void MonsterSetRuntimeFlag5BC(W8Monster* monster, unsigned char flag)
{
    monster->m_pRep->flag_5bc = flag;
}

/* Cycle 18's pointee carries the scale at +0x5f0. Both accessors reach it the
   same way - through the pointer at the cycle's +0x0c, which 0x004E60B0 also
   reads a byte from - so the pointee is a shared engine object rather than
   anything the cycle owns. It is not modelled: only this one field is known. */
// FUNCTION: WIZ8 0x004c5780
float MonsterGetScale(W8Monster* monster)
{
    return monster->m_pRep->scale_5f0;
}

// FUNCTION: WIZ8 0x004c57a0
void MonsterSetScale(W8Monster* monster, float scale)
{
    monster->m_pRep->scale_5f0 = scale;
}

// FUNCTION: WIZ8 0x004c57c0
void MonsterGetScaleRange(W8Monster* monster, float* minimum, float* maximum)
{
    W8MonsterRep* runtime = monster->m_pRep;

    *minimum = runtime->minimum_scale_5f4;
    *maximum = runtime->maximum_scale_5f8;
}

/* Returns the previous animation state and timestamps every update through the
   recovered shared SurRender timer. */
// FUNCTION: WIZ8 0x004c5a00
unsigned char MonsterSetAnimating(W8Monster* monster, unsigned char animating)
{
    if (monster != 0) {
        W8MonsterRep* runtime = monster->m_pRep;
        unsigned char previous = runtime->flag_06d;

        runtime->flag_06d = animating;
        runtime->timer_068 =
            g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        return previous;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c59e0
unsigned char MonsterIsAnimating(W8Monster* monster)
{
    if (monster != 0) {
        return monster->m_pRep->flag_06d;
    }
    return 0;
}

/* Cycle 19 bit 5 blocks pending-cycle changes. Otherwise the request is stored
   as the signed low byte in cycle 18's runtime record. */
// FUNCTION: WIZ8 0x004c5aa0
void MonsterSetPendingCycle(W8Monster* monster, int cycle)
{
    if (monster != 0 && ((monster->flags_1dc >> 5) & 1) == 0) {
        monster->m_pRep->selection.monster.pending_cycle = (signed char)cycle;
    }
}

// FUNCTION: WIZ8 0x004c5e40
void MonsterSetRuntimeBehaviour(W8Monster* monster, signed char behaviour)
{
    if (monster != 0) {
        if (behaviour < 1 || behaviour > 3) {
            srAssertFail(
                "bBehaviour >= BEHAVIOUR_FIRST && bBehaviour <= BEHAVIOUR_LAST",
                "..\\Engine Code\\Include\\AnimRep.hpp",
                0x87,
                0);
        }
        monster->m_pRep->behaviour_071 = behaviour;
    }
}

// FUNCTION: WIZ8 0x004c5ee0
unsigned char MonsterHasCycle19Flag3(W8Monster* monster)
{
    if (monster != 0) {
        return (monster->flags_1dc >> 3) & 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c6160
void MonsterSetStateA0(W8Monster* monster, unsigned char state)
{
    if (monster != 0) {
        monster->fields.state_088 = state;
    }
}

/* Named by the MonsterManager assertions. A null monster answers -1 rather than
   forwarding, which is how the callers tell "no monster" from a real result. */
// FUNCTION: WIZ8 0x004c5b40
int MonsterQuery(W8Monster* monster, int query)
{
    if (monster != NULL) {
        return monster->Query(query);
    }
    return -1;
}

// FUNCTION: WIZ8 0x004ca4c0
unsigned char W8Monster::IsDying()
{
    unsigned char dying = Query(6) == 0x15 ||
                          m_pRep->selection.monster.pending_cycle == 0x15;

    return dying;
}

/* Resolve mapped vertex zero on the current model and transform it into world
   space. Models without that mapping use the Navigator position plus the
   Monster's vertical offset. */
// FUNCTION: WIZ8 0x004c72a0
void W8Monster::GetMappedPosition004C72A0(W8Position* position)
{
    srModelInstance* instance = GetCurrentModelInstance004A8250();

    if (instance != 0) {
        stMeshModel* mesh = static_cast<stMeshModel*>(instance->model());
        while (mesh != 0) {
            int index = mesh->FindMappedIndex(0);

            if (index >= 0) {
                srVector3T<float>* vertices;
                if ((mesh->flags_3a0 & 4) == 0) {
                    vertices = mesh->getVertexLoc();
                }
                else {
                    vertices = mesh->GetVertexLocations00471AD0(0, 1, 0.0f);
                }
                if (vertices != 0) {
                    srMatrix4T<float> matrix;
                    float x;
                    float y;
                    float z;

                    position->x = vertices[index].x;
                    position->y = vertices[index].y;
                    position->z = vertices[index].z;
                    instance->getWorldSpaceMatrix(matrix);
                    x = position->x;
                    y = position->y;
                    z = position->z;
                    position->y = matrix.vectors[1].x * x +
                                  matrix.vectors[1].y * y +
                                  matrix.vectors[1].z * z +
                                  matrix.vectors[1].w;
                    position->x = matrix.vectors[0].x * x +
                                  matrix.vectors[0].y * y +
                                  matrix.vectors[0].z * z +
                                  matrix.vectors[0].w;
                    position->z = matrix.vectors[2].x * x +
                                  matrix.vectors[2].y * y +
                                  matrix.vectors[2].z * z +
                                  matrix.vectors[2].w;
                    return;
                }
            }
            mesh = mesh->next;
        }
    }

    position->x = fields.position_100.x;
    position->y = fields.position_100.y;
    position->z = fields.position_100.z;
    position->y += fields.value_178;
}

/* Six thin bodies over the live animation object. Each is a null check and a
   forward, or a single member read; nothing here says what the members and
   slots are for, so each is named for what it reaches. */

extern void Function4C4DE0(int arg_1, int arg_2, int arg_3);
extern void Function4C0300(int arg_1, int arg_2, int arg_3, int arg_4, int arg_5);
/* Neither takes an argument nor reads ECX: both work entirely over the pair of
   globals at 0x00659B34 and 0x00659B3C, which is what makes them free
   functions rather than the Navigator methods their neighbours in the same
   address range are. */
extern void Function453160(void);
extern void Function4531A0(void);
/* Cleans its own argument - the caller at 0x004C5A40 pushes and never adjusts
   afterwards - so it is __stdcall and not the cdecl the decompiler assumes. */
extern void __stdcall Function4A7BE0(const float* position);

/* The location-id lookup pair, spelled as Magic Effects.cpp already declares
   it: the index comes first and the script part is fetched from it. */
extern unsigned int MonsterGetIndexByLocationID(
    int caller_line, const char* caller_file, int location_id, unsigned char assert_on_failure);
extern W8MonsterInfo* MonsterGetScriptPartByLocationIndex(unsigned int index);
extern unsigned int MonsterCastsSpell(
    W8MonsterInfo* monster_info, int spell_id, unsigned int power_level);
extern void FatigueMonster(
    W8MonsterInfo* monster_info, unsigned int amount, int report_to);

/* The caller proves only the roles below: the first global selects a frame in
   the spell animation, and the second indexes g_spell_records. Their original
   descriptive names have not been recovered. */
extern int g_spell_effect_frame_0064c158;
extern int g_spell_index_0069b7dc;
extern void* CreateSpellEffect004AD8A0(
    const char* mls_name, int frame, W8Monster* parent, int value, int flags);

/* 0x00421070, owned by the 0041F261-0042403F quarantine: the shared reference
   position every consumer of the object at 0x0065A0F8 reads. */
extern void GetPosition421070(W8Position* position);
extern void MonsterNavigatorUpdate(W8Navigator* navigator); /* 0x00453970 */
extern void Function4A84A0(W8GrCycle* monster);
/* Spelled the way MonsterManager.cpp already declares it: the callee takes its
   receiver in ECX, which __fastcall is how a no-argument member call is
   reachable from a free declaration. The receiver is the monster's Navigator
   base at +0x18. */
extern void __fastcall Function4537E0(W8Navigator* navigator);

/* Copies a position into a local and hands the local on. The monster argument
   is dead beyond its own null check - the callee never receives it - which is
   the same shape the other guarded forwarders here take, except that what
   survives the guard is the copy rather than the object.
   The copy goes through the FPU one component at a time - `fld dword` then
   `fstp dword` per component - rather than as the three integer moves VC6
   emits for a plain three-float assignment, which is what this body still gets
   and the whole of its remaining difference. That shape is the signature of
   srVector3T<float>::method_00421680 expanded inline: its parameters are
   doubles, so each float round-trips through the FPU instead of being copied
   as bits. The image carries both an out-of-line COMDAT copy of that setter at
   0x00421680 and this inlined expansion, which is the multiple-translation-unit
   visibility the inlining policy asks for before a body moves into a header.

   That was measured rather than argued. Defining the setter in srMath.h and
   calling it here reproduces the copy exactly - the three fld/fstp pairs land
   instruction for instruction, leaving only a register choice and one
   scheduling swap - and takes this body from 0.375 to 0.8125. It also stops
   VC6 emitting the out-of-line copy at all, because this is the only call site
   in the tree and it inlines: 0x00421680 goes from exact to missing. The
   inlining policy requires the bundle to improve without regressing an exact
   boundary, so the trade is refused and the out-of-line definition stays.
   Hand-spelling the conversion does not work either - `(float)(double)f` is
   value-preserving, so VC6 folds it straight back to the integer copy.
   Reproducing both emissions needs a second call site that does not inline,
   which is not decidable from this one; the filed bead tracks it. */
// FUNCTION: WIZ8 0x004c5a40
void MonsterForward4A7BE0(W8Monster* monster, const W8Position* position)
{
    srVector3T<float> local;

    if (monster != 0) {
        local.x = position->x;
        local.y = position->y;
        local.z = position->z;
        Function4A7BE0(&local.x);
    }
}

/* Records a value on the cycle runtime and, when nothing is pending, seeds the
   pending cycle from the runtime's own fallback at 0x0a4 rather than leaving it
   at -1. The runtime pointer is fetched twice rather than held in a local -
   the second `mov` reloads it from the cycle - which is what says the original
   spelled the two reaches out separately instead of naming the record once. */
// FUNCTION: WIZ8 0x004c6c00
void W8Monster::SetRuntimeValueA6(unsigned char value)
{
    m_pRep->selection.monster.runtime_value_a6 = value;
    if (m_pRep->selection.monster.pending_cycle == -1) {
        m_pRep->selection.monster.pending_cycle =
            m_pRep->selection.monster.current_cycle;
    }
}

/* Writes the sixteen-byte block the cycle runtime carries at 0x4c, but only for
   a monster that is neither absent nor already answering the dying cycle to
   query six - the same 0x15 the death test compares against, reached the same
   way. Both guards leave through one shared exit, which is why the body has a
   single epilogue despite testing two things. The block arrives by value and is
   stored as one assignment. */
// FUNCTION: WIZ8 0x004c5ad0
void MonsterSetRuntimeBlock4C(W8Monster* monster, W8MonsterRuntimeBlock4C block)
{
    if (monster != 0 && monster->Query(6) != 0x15) {
        monster->m_pRep->value_04c = block;
    }
}


/* The engine object a monster holds at 0x0c, or nothing when there is no
   monster to ask. */
// FUNCTION: WIZ8 0x004c5b30
void* MonsterGetObject0C(W8Monster* monster)
{
    if (monster != 0) {
        return monster->m_pAI;
    }
    return 0;
}

/* Run the member update with no monster of its own to name. */
// FUNCTION: WIZ8 0x004c5770
void UpdateMonsterNavigator(void)
{
    MonsterNavigatorUpdate(0);
}

/* Two null-checked forwards that share one shape: a monster that is not there
   is simply not acted on. */
// FUNCTION: WIZ8 0x004c5ea0
void MonsterForward4A84A0(W8Monster* monster)
{
    if (monster != 0) {
        Function4A84A0(monster);
    }
}

// FUNCTION: WIZ8 0x004c6140
void MonsterForward4537E0(W8Monster* monster)
{
    if (monster != 0) {
        Function4537E0(monster);
    }
}

/* Two more of the same null-checked shape, except that what they forward to is
   already recovered: both callees are W8GrCycle setters GrCycle.cpp owns, and
   both are reached as methods rather than as free functions - the receiver
   stays in ECX across the guard and only the value is pushed. That is what
   types the parameter as the cycle rather than as the opaque pointer the
   neighbouring forwarders take. */
// FUNCTION: WIZ8 0x004c61a0
void MonsterSetCycleBehaviour(W8GrCycle* cycle, signed char bBehaviour)
{
    if (cycle != 0) {
        cycle->SetBehaviour(bBehaviour);
    }
}

// FUNCTION: WIZ8 0x004c61c0
void MonsterSetCycleSubCycle(W8GrCycle* cycle, unsigned char subcycle)
{
    if (cycle != 0) {
        cycle->SetSubCycle(subcycle);
    }
}

/* An unguarded three-argument forward. The arguments are pushed back to front
   and handed straight on, so nothing here says what any of them mean. */
// FUNCTION: WIZ8 0x004c5eb0
void MonsterForward4C4DE0(int arg_1, int arg_2, int arg_3)
{
    Function4C4DE0(arg_1, arg_2, arg_3);
}

/* The same unguarded cdecl pass-through with five arguments. Like its
   three-argument sibling it re-pushes its own stack slots and makes a real
   call rather than jumping: caller-cleanup cdecl cannot tail-jump when there
   are stack arguments to account for. */
// FUNCTION: WIZ8 0x004c58e0
void MonsterForward4C0300(int arg_1, int arg_2, int arg_3, int arg_4, int arg_5)
{
    Function4C0300(arg_1, arg_2, arg_3, arg_4, arg_5);
}

/* Two whole-body tail calls. Neither wrapper takes an argument and neither
   callee touches ECX - both read only the pair of globals at 0x00659B34 and
   0x00659B3C - so the wrappers pass nothing on and VC6 lowers each to a bare
   jump. That is the whole difference from the cdecl pass-throughs above: with
   no stack arguments there is nothing left to clean up. */
// FUNCTION: WIZ8 0x004c61e0
void MonsterForward453160(void)
{
    Function453160();
}

// FUNCTION: WIZ8 0x004c61f0
void MonsterForward4531A0(void)
{
    Function4531A0();
}

/*
 * Six more null-guarded forwarders onto the Navigator base at +0x18, the same shape
 * MonsterForward4537E0 has: the guard tests the monster, the receiver is
 * derived from it with a `lea`, and a monster that is not there is simply not
 * acted on. What each one answers on the null path is the evidence for its
 * return type - a cleared AL for the byte-sized ones, a loaded 0.0f for the
 * float, and a bare return for the four that hand nothing back.
 */
// FUNCTION: WIZ8 0x004c5f50
void MonsterSetNavigatorValue120(W8Monster* monster, float value)
{
    if (monster != 0) {
        monster->SetValue120(value);
    }
}

// FUNCTION: WIZ8 0x004c5f70
float MonsterGetNavigatorValue120(W8Monster* monster)
{
    if (monster != 0) {
        float value = monster->GetValue120();
        return value;
    }
    return 0.0f;
}

// FUNCTION: WIZ8 0x004c5f90
unsigned char MonsterForward452630(W8Monster* monster, const W8Position* position)
{
    if (monster != 0) {
        return monster->Function452630(position);
    }
    return 0;
}

// FUNCTION: WIZ8 0x004c5fb0
void MonsterForward453690(W8Monster* monster, void* argument)
{
    if (monster != 0) {
        monster->Function453690(argument);
    }
}

// FUNCTION: WIZ8 0x004c5fd0
void MonsterSetNavigatorObjectFlag38(W8Monster* monster, char value)
{
    if (monster != 0) {
        monster->SetObject68Flag38(value);
    }
}

// FUNCTION: WIZ8 0x004c6200
void MonsterSetNavigatorFlag25(W8Monster* monster, char value)
{
    if (monster != 0) {
        monster->SetFlag25(value);
    }
}

/* Hands the shared reference position to one of Navigator's two position
   sinks. The monster's script part is found the long way round - the location
   id lives in the cycle array at 0x1e4, and the lookup pair turns it into the
   W8MonsterInfo whose control state gates the whole body - which is what puts
   this in Monster.cpp rather than in the engine: the assertion path the lookup
   carries names this file and line 5299.
   Control state one is the only value that suppresses the update; every other
   value falls through. The position is fetched before the flag is read, so it
   is read even on the path that turns out not to need one sink over the other,
   and the flag decides only which sink receives it. */
// FUNCTION: WIZ8 0x004c6240
void MonsterForwardReferencePosition(W8Monster* monster, char alternate)
{
    W8MonsterInfo* monster_info;
    W8Position position;

    if (monster != 0) {
        monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
            0x14b3, MONSTER_CPP, monster->propagated_value_1e4, 1));
        if (monster_info->control_state != 1) {
            GetPosition421070(&position);
            if (alternate != 0) {
                monster->Function454040(&position);
            } else {
                monster->Function453F30(&position);
            }
        }
    }
}

/* Flatten every model instance reachable from every cycle and subcycle. The
   temporary vectors used by the damage-appearance accessors below prove the
   element type: AniMesh's frame lookup returns stModelInstance objects and the
   consumers read their first-party fields beyond the srModelInstance base. */
// FUNCTION: WIZ8 0x004c6350
void W8Monster::CollectModelInstances004C6350(
    W8GrowableVector<stModelInstance*>* instances)
{
    int cycle;

    GetTotalAnimationCount();
    for (cycle = 0; cycle < W8_MONSTER_CYCLE_COUNT; ++cycle) {
        int subcycle;

        for (subcycle = 0;
             subcycle < m_pRep->GetNumSubsPerCycle((signed char)cycle);
             ++subcycle) {
            W8MonsterAnimationVector* cycle_animations =
                &m_pRep->animations[cycle];
            W8AnimObj* animation;

            if (subcycle >= cycle_animations->GetCount()) {
                Function401920(FormatString(
                    "Monster %s: Missing CYCLE %s subcycle %d",
                    m_pRep->name_5c0,
                    g_cycle_names[cycle].name,
                    subcycle));
            }
            animation = *cycle_animations->GetAt(subcycle);
            if (animation == 0) {
                continue;
            }

            if (AnimationIsRunning(animation) == 0) {
                int list_index;

                for (list_index = 0; list_index < 3; ++list_index) {
                    W8AniMesh* mesh =
                        static_cast<W8AniMesh*>(animation->entries_18[list_index]);
                    if (mesh != 0) {
                        int frame_count = AniMeshValue004B64F0(mesh);

                        if ((mesh->flags_00 & 0x20) != 0) {
                            instances->Add(GetAniMeshFrame004B6550(mesh, 0));
                        }
                        else {
                            int frame;

                            for (frame = 0; frame < frame_count; ++frame) {
                                instances->Add(
                                    GetAniMeshFrame004B6550(mesh, frame));
                            }
                        }
                    }
                }
            }
            else if (AnimationIsRunning(animation) == 1) {
                int list_index;

                for (list_index = 0; list_index < 3; ++list_index) {
                    W8PList* list = animation->lists_28[list_index];
                    if (list != 0) {
                        int mesh_index;
                        int mesh_count = PListGetCount(list);

                        for (mesh_index = 0; mesh_index < mesh_count; ++mesh_index) {
                            W8AniMesh* mesh = static_cast<W8AniMesh*>(
                                PListGetAt(list, mesh_index));
                            if (mesh != 0) {
                                int frame;
                                int frame_count = AniMeshValue004B64F0(mesh);

                                for (frame = 0; frame < frame_count; ++frame) {
                                    instances->Add(
                                        GetAniMeshFrame004B6550(mesh, frame));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Select the damage-stage model on every frame instance owned by this
   Monster. UpdateMonsterDamageAppearance supplies the HP-derived stage. */
// FUNCTION: WIZ8 0x004c6990
void W8Monster::SetDamageStage004C6990(int stage)
{
    W8GrowableVector<stModelInstance*> instances;
    int index;

    CollectModelInstances004C6350(&instances);
    for (index = 0; index < instances.GetCount(); ++index) {
        (*instances.GetAt(index))->damage_stage_184 = stage;
    }
}

/* Every frame instance in one Monster carries the same number of available
   damage stages, so the first instance supplies the count. */
// FUNCTION: WIZ8 0x004c6a50
int W8Monster::GetDamageStageCount004C6A50()
{
    W8GrowableVector<stModelInstance*> instances;

    CollectModelInstances004C6350(&instances);
    if (instances.GetCount() != 0) {
        return (*instances.GetAt(0))->damage_stage_count_18c;
    }
    return 0;
}

/* Resolve the database-controlled render gate after the Monster's transient
   runtime overrides. The alternate argument selects the secondary live-info
   flag used by the world-update path. */
// FUNCTION: WIZ8 0x004c7c00
unsigned char W8Monster::IsRenderable004C7C00(char alternate)
{
    unsigned char disabled = flag_217;
    int location_id = propagated_value_1e4;
    W8MonsterInfo* monster_info;
    W8MonsterRecord* record;

    if (disabled != 0) {
        return 0;
    }
    if (flag_215 != 0) {
        return 1;
    }
    if (location_id == -1) {
        return 1;
    }
    if (((flags_1dc >> 8) & 1) != 0) {
        return 1;
    }
    if (flags_330.flag_00 != 0) {
        return 1;
    }

    monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
        0x1977, MONSTER_CPP, location_id, 1));
    record = GetMonsterDataForInfo(monster_info);
    if (record->flag_248 > 0) {
        return monster_info->flag_28d;
    }
    if (alternate != 0) {
        return monster_info->flag_2ab;
    }
    return monster_info->flag_24d;
}

/* Forward to the object's own vtable slot four. */
// FUNCTION: WIZ8 0x004c59b0
void MonsterCallSlot10(void* object, int argument)
{
    (*(void(**)(void*, int))(*(void***)object + 4))(object, argument);
}

extern "C" {
// FUNCTION: WIZ8 0x004C5810
void Function4C5810(W8Forwarded* target)
{
    target->Method4C5290();
}
}
extern "C" {
// FUNCTION: WIZ8 0x004C5860
void Function4C5860(W8MonsterReleasable005C8* object)
{
    if (object != NULL) {
        delete object;
    }
}
}
extern "C" {
// FUNCTION: WIZ8 0x004C59C0
void Function4C59C0(int enabled, int value)
{
    if (enabled != 0 && value != 0) {
        Function4A7A70(value);
    }
}
}
extern "C" {
// FUNCTION: WIZ8 0x004C5ED0
void Function4C5ED0(int enabled)
{
    if (enabled != 0) {
        Function4C4EF0();
    }
}
}
// FUNCTION: WIZ8 0x004C6220
void SetFlag6081E4(unsigned char value)
{
    g_flag_6081e4 = value;
    g_value_659c14 = 0;
}
