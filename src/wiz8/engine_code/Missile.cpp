/*
 * Engine Code\Missile.cpp.
 *
 * What a missile is fired from and where it comes out of. A missile holds a
 * launcher record at 0x1dc; the record carries a small table of emitters at
 * 0xd8 and an index into it at 0xa4, and the accessors below read the chosen
 * emitter, count how many the record has, and reach the emitter's own value.
 */

#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stLight.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/sr_api.h"
#include "wiz8/render_state.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "surrender/srTimer.h"

#include <stdlib.h>
#include <string.h>

extern int IncrementValue60DFAC(void);
extern void ResetCombatSlot(W8CombatSlot* slot);
extern float g_navigator_largest_extent_6081e8;

/* The copy body establishes only these fields. Padding remains explicit: the
   source leaves it uninitialized in the freshly allocated result. */
struct W8AIMissile {
    unsigned char value_00;
    unsigned char value_01;
    unsigned char unknown_02[2];
    int value_04;
    int value_08;
    unsigned char unknown_0c[4];
    int value_10;
    int value_14;
    int value_18;
    unsigned char value_1c;
    unsigned char unknown_1d[3];
};

static_assert(sizeof(W8AIMissile) == 0x20, "W8AIMissile_must_be_0x20");

// FUNCTION: WIZ8 0x004a53a0
W8AIMissile* CopyAIMissile004A53A0(const W8AIMissile* source)
{
    W8AIMissile* copy = static_cast<W8AIMissile*>(malloc(sizeof(W8AIMissile)));

    if (copy == 0) {
        srAssertFail("pAIMissile", "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp", 0x86d, 0);
    }
    copy->value_00 = source->value_00;
    copy->value_01 = source->value_01;
    copy->value_04 = source->value_04;
    copy->value_08 = source->value_08;
    copy->value_10 = source->value_10;
    copy->value_14 = source->value_14;
    copy->value_18 = source->value_18;
    copy->value_1c = source->value_1c;
    return copy;
}

#pragma pack(push, 1)
struct W8MissileTableRecord {
    unsigned char unknown_000[0x140];
    float value_140;
    unsigned char unknown_144[0xa1];
};
#pragma pack(pop)

// GLOBAL: WIZ8 0x0065bde0
W8MissileTableRecord* g_missile_table_65bde0;
// GLOBAL: WIZ8 0x0065bddc
unsigned int g_missile_table_count_65bddc;

static_assert(sizeof(W8MissileTableRecord) == 0x1e5, "W8MissileTableRecord_must_be_0x1e5");

/* Engine Code\\Missile.cpp's startup database load.  Each disk row has a
   0x101-byte editor prefix followed by the 0x1e5-byte runtime record. */
// FUNCTION: WIZ8 0x004a5600
extern "C" unsigned char LoadMissileDatabase(void)
{
    char path[] = "Data\\Databases\\MissileTables.dbs";
    int allocated_count;
    unsigned int record_count;
    unsigned int index;
    int handle;

    if (g_missile_table_65bde0) {
        ::operator delete(g_missile_table_65bde0);
        g_missile_table_65bde0 = 0;
        g_missile_table_count_65bddc = 0;
    }
    handle = FileOpen(path, 0x41, 0);
    if (!handle ||
        !ReadVirtualFile(handle, &allocated_count, 4, 0) ||
        !ReadVirtualFile(handle, &record_count, 4, 0)) {
        if (handle) {
            CloseVirtualFile(handle);
        }
        return 0;
    }
    g_missile_table_65bde0 = static_cast<W8MissileTableRecord*>(
        ::operator new(allocated_count * sizeof(W8MissileTableRecord)));
    if (!g_missile_table_65bde0) {
        CloseVirtualFile(handle);
        return 0;
    }
    for (index = 0; index < record_count; ++index) {
        if (!FileSeek(handle, 0x101, 4) ||
            !ReadVirtualFile(handle, &g_missile_table_65bde0[index],
                             sizeof(W8MissileTableRecord), 0)) {
            ::operator delete(g_missile_table_65bde0);
            g_missile_table_65bde0 = 0;
            g_missile_table_count_65bddc = 0;
            CloseVirtualFile(handle);
            return 0;
        }
    }
    g_missile_table_count_65bddc = record_count;
    CloseVirtualFile(handle);
    return 1;
}

/* Release the one allocation that owns every runtime missile-table row. */
// FUNCTION: WIZ8 0x004a5760
extern "C" void ReleaseMissileDatabase(void)
{
    if (g_missile_table_65bde0) {
        ::operator delete(g_missile_table_65bde0);
        g_missile_table_65bde0 = 0;
        g_missile_table_count_65bddc = 0;
    }
}

// VTABLE: WIZ8 0x005ecde0 W8MissileRep
// class W8MissileRep

// VTABLE: WIZ8 0x005ece08 W8Missile
// VTABLE: WIZ8 0x005ecdf4 W8Navigator
// class W8Missile

static int g_missile_iterator_0065bde4;

/* Iterate the world's missile vector. A nonzero argument restarts the shared
   cursor; a missing world or vector answers null. */
// FUNCTION: WIZ8 0x004A2760
W8Missile* Function4A2760(char restart)
{
    W8Missile* missile = 0;

    if (g_world != 0 && g_world->missiles != 0) {
        if (restart != 0) {
            g_missile_iterator_0065bde4 = 0;
        }
        if (g_missile_iterator_0065bde4 < g_world->missiles->GetCount()) {
            missile = *g_world->missiles->GetAt(g_missile_iterator_0065bde4);
            ++g_missile_iterator_0065bde4;
        }
    }
    return missile;
}

extern float Function4BE490(
    const srVector3T<float>* source, const srVector3T<float>* target);
extern float Function4BE420(
    const srVector3T<float>* source, const srVector3T<float>* target);
extern W8Missile* Function4A28D0(
    unsigned int missile_table_index,
    srVector3T<float>* source,
    float value_3,
    float value_4,
    unsigned int value_5,
    unsigned int value_6,
    unsigned int value_7,
    unsigned int value_8);

/* Derive the two launch angles from the source and target, then forward the
   remaining launch values to the missile factory. */
// FUNCTION: WIZ8 0x004A2D30
W8Missile* Function4A2D30(
    unsigned int missile_table_index, srVector3T<float>* source,
    srVector3T<float>* target, unsigned int value_4,
    unsigned int value_5, unsigned int value_6,
    unsigned int value_7)
{
    return Function4A28D0(
        missile_table_index, source,
        Function4BE420(source, target), Function4BE490(source, target),
        value_4, value_5, value_6, value_7);
}

/* The missile and spell representations use the same ordinary AnimObj
   operations for these two vtable slots.  Retail points both final tables at
   the corresponding bodies at 0x004AB290 and 0x004AB310. */
srModelInstance* W8MissileRep::SetCycleFrameLod(
    signed char emitter, signed char frame, signed char lod)
{
    return AnimObjDispatch004A14D0(emitters[emitter], lod, frame);
}

W8AniMesh* W8MissileRep::GetEmitterAniMesh(char emitter)
{
    W8AnimObj* target = emitters[emitter];

    if (target == 0) {
        return 0;
    }
    return static_cast<W8AniMesh*>(
        AnimObjEntry004A1660(target, m_bLOD, 0));
}

/* Apply the representation's current LOD to one required animation. */
// FUNCTION: WIZ8 0x004A2710
unsigned int W8MissileRep::ApplyEmitterSetting(char emitter)
{
    W8AnimObj* target = emitters[emitter];

    if (target == 0) {
        srAssertFail(
            "pao",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp",
            0x7e,
            0);
    }
    return AnimObjValue004A15D0(target, m_bLOD);
}

/* The two emitter slots start empty and at the source default playback value.
   Construction of the two light-list vectors is ordinary array-member
   construction and precedes these assignments in the retail body. */
W8MissileRep::W8MissileRep()
{
    emitters[0] = 0;
    emitters[1] = 0;
    emitter_values[0] = 15.0f;
    emitter_values[1] = 15.0f;
}

/* Copy the two proven representation values, clone each animation, and deep
   copy every optional light vector. The copied lights are ordinary scene
   objects registered with the world but detached until the missile selects
   their emitter. */
// FUNCTION: WIZ8 0x004a2db0
W8MissileRep::W8MissileRep(const W8MissileRep& other)
    : W8EmitterHost(other),
      value_0ac(other.value_0ac),
      value_0b0(other.value_0b0)
{
    int emitter;

    for (emitter = 0; emitter < 2; ++emitter) {
        if (other.emitters[emitter] == 0) {
            emitters[emitter] = 0;
            emitter_values[emitter] = 15.0f;
        }
        else {
            emitters[emitter] = CloneAnimObj004A0320(other.emitters[emitter]);
            emitter_values[emitter] = other.emitter_values[emitter];
        }
    }

    for (emitter = 0; emitter < 2; ++emitter) {
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
                        "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp",
                        0x184,
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
                            "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp",
                            0x18c,
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

// FUNCTION: WIZ8 0x004a5d40
W8AnimRepBase005EC1D8* W8MissileRep::Clone()
{
    return new W8MissileRep(*this);
}

// FUNCTION: WIZ8 0x004A3300
unsigned char W8MissileRep::ReadCycleData004A3300(
    W8GrCycleReadInfo004A6970* info,
    W8Missile* missile,
    int,
    int emitter_index)
{
    W8GrowableVector<stLight*>* lights = new W8GrowableVector<stLight*>;
    W8AnimObj* animation;
    unsigned char success;
    signed char emitter;

    if (info == 0 || info->handle_04 == 0 || missile == 0) {
        srAssertFail(
            "pInfo && pInfo->hFile && pMissile",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp",
            0x1df,
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
        missile->SetLights(lights);
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

    if (missile != 0) {
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

/* Retail constructs the +0x2d8 growable vector normally, then clears the
   complete +0x280..+0x321 tail after allocating the representation.  That
   overwrites the vector and leaks its initial five-pointer allocation; the
   ordering is preserved because it is present explicitly in the product. */
// FUNCTION: WIZ8 0x004A3C10
W8Missile::W8Missile()
    : missile_table_index_1d8(-1),
      m_pRep(0),
      flag_1e0(0),
      flag_1e1(0),
      flag_1e2(1),
      flag_1e3(0),
      flag_1e4(0),
      flag_1e5(0),
      flag_1e6(0),
      flag_1e7(1),
      value_1e8(0),
      value_1ec(0),
      lifetime_1f0(15000.0f),
      value_1f4(0),
      flag_322(0)
{
    W8GrObject::unknown_004 = 1;
    radius_084 = 1.0f;
    if (g_navigator_largest_extent_6081e8 < 1.0f) {
        g_navigator_largest_extent_6081e8 = 1.0f;
    }
    movement_0c0.value_0b0 = 1.0f;
    if (g_navigator_largest_extent_6081e8 < 1.0f) {
        g_navigator_largest_extent_6081e8 = 1.0f;
    }
    movement_0c0.alternate_radius_0b4 = 1.0f;
    if (g_navigator_largest_extent_6081e8 < 1.0f) {
        g_navigator_largest_extent_6081e8 = 1.0f;
    }
    unknown_008 = IncrementValue60DFAC();

    m_pRep = new W8MissileRep;
    if (m_pRep == 0) {
        srAssertFail(
            "m_pRep",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp",
            0x38e,
            0);
    }

    memset(values_1fc, 0, sizeof(values_1fc));
    memset(unknown_280, 0, 0xa2);
    ResetCombatSlot(&combat_slot_260);
}

/* Release the representation and every external reference before ordinary
   vector and GrCycle teardown. */
// FUNCTION: WIZ8 0x004a3fc0
W8Missile::~W8Missile()
{
    SetLights(0);
    delete m_pRep;
    m_pRep = 0;
    DetachMissileReferences005019A0(this);
    UnregisterGrCycle(this);
}

/* Remove a missile's world lights and AI allocation, then release the object. */
// FUNCTION: WIZ8 0x004a4180
void DestroyMissile(W8Missile* missile)
{
    if (missile->m_plsLights != 0) {
        int count = missile->m_plsLights->GetCount();

        while (count != 0) {
            stLight* light = missile->m_plsLights->RemoveAt(0);
            WorldRemoveLight(g_world, light);
            --count;
        }
    }
    if (missile->m_pAI != 0) {
        free(missile->m_pAI);
        missile->m_pAI = 0;
    }
    delete missile;
}

/* Delete every missile owned by one world, unlinking the vector entry before
   its ordinary object teardown. */
// FUNCTION: WIZ8 0x004a4210
void DestroyAllMissiles(W8World* world)
{
    while (world->missiles->GetCount() != 0) {
        W8Missile* missile = *world->missiles->GetAt(0);

        if (missile == 0) {
            srAssertFail(
                "pMissile",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp",
                0x4ab,
                0);
        }
        g_world->missiles->RemoveAt(g_world->missiles->IndexOf(missile));
        DestroyMissile(missile);
    }
}

/* Release the two owned animations and every light vector before the ordinary
   vector members and W8EmitterHost base tear themselves down. */
// FUNCTION: WIZ8 0x004A3230
W8MissileRep::~W8MissileRep()
{
    int emitter;

    for (emitter = 0; emitter < 2; ++emitter) {
        if (emitters[emitter] != 0) {
            DestroyAnimObj004A01E0(emitters[emitter]);
            emitters[emitter] = 0;
        }
    }

    for (emitter = 0; emitter < 2; ++emitter) {
        int light_list;

        for (light_list = 0; light_list < light_lists[emitter].GetCount();
             ++light_list) {
            DestroyLightVector(*light_lists[emitter].GetAt(light_list));
        }
        light_lists[emitter].Clear();
    }
}

/* Copy the twelve-word state block, then replace its first float from the
   selected 0x1e5-byte missile database row. */
// FUNCTION: WIZ8 0x004A5410
void W8Missile::Function4A5410(const float* values)
{
    memcpy(values_1fc, values, sizeof(values_1fc));
    values_1fc[0] = g_missile_table_65bde0[missile_table_index_1d8].value_140;
}

/* The representation a missile was fired from. */
// FUNCTION: WIZ8 0x004a45e0
W8EmitterHost* W8Missile::GetRepresentation()
{
    return m_pRep;
}

/* The animation the representation is currently firing from. */
// FUNCTION: WIZ8 0x004a4570
W8AnimObj* W8Missile::GetCurrentAnimation()
{
    return m_pRep->emitters[
        m_pRep->current_cycle];
}

/* That animation's own playback scale. */
// FUNCTION: WIZ8 0x004a45c0
float W8Missile::GetCurrentAnimationScale()
{
    return m_pRep->emitters[
        m_pRep->current_cycle]->playback_scale_08;
}

// FUNCTION: WIZ8 0x004a45f0
W8AniMesh* W8Missile::GetCurrentAniMesh()
{
    W8AnimObj* animation = m_pRep->emitters[
        m_pRep->current_cycle];

    if (animation == 0) {
        srAssertFail("pao", "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp", 0x55e, 0);
    }
    return animation->entries_18[m_pRep->m_bLOD];
}

/* How many emitters the launcher has, counted by testing each for null rather
   than read from a stored count. */
// FUNCTION: WIZ8 0x004a4590
signed char W8Missile::GetTotalAnimationCount()
{
    signed char count = 0;

    if (m_pRep->emitters[0] != 0) {
        count = 1;
    }

    if (m_pRep->emitters[1] != 0) {
        ++count;
    }
    return count;
}

/* The missile adds no work around the ordinary GrCycle update. */
// FUNCTION: WIZ8 0x004a4100
void W8Missile::UpdateRepresentation(W8World* world)
{
    W8GrCycle::UpdateRepresentation(world);
}

/* Hand the representation's LOD to the current animation. */
// FUNCTION: WIZ8 0x004a4110
signed char W8Missile::GetNumSubCycles()
{
    W8AnimObj* animation = GetCurrentAnimation();

    return static_cast<signed char>(
        AnimObjValue004A15D0(animation, m_pRep->m_bLOD));
}

// FUNCTION: WIZ8 0x004a42b0
unsigned char W8Missile::IsCycleSupported(signed char cycle)
{
    if (cycle >= 2) {
        srAssertFail(
            "bCycle<MISSILE_NUM_CYCLES",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp",
            0x4bf,
            0);
    }
    return m_pRep->emitters[cycle] != 0;
}

/* Select one of the missile representation's two emitters and rebuild its
   light and particle attachment state. */
// FUNCTION: WIZ8 0x004a4300
void W8Missile::SetCycle(signed char cycle)
{
    W8GrowableVector<stLight*>* lights;
    W8AnimObj* animation;
    int index;

    if (cycle < 0 || cycle >= 2) {
        srAssertFail(
            "bCycle >= MISSILE_CYCLE_FIRST && bCycle <= MISSILE_CYCLE_LAST",
            "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp",
            0x4d6,
            0);
    }

    lights = *m_pRep->light_lists[
        m_pRep->current_cycle].GetAt(0);
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

    m_pRep->current_cycle = cycle;
    animation = m_pRep->emitters[cycle];
    m_pRep->active = 1;
    m_pRep->flag_06e = 1;
    if (m_pRep->SetCycleFrameLod(cycle, 0, 2) != 0) {
        m_pRep->m_bLOD = 2;
    }
    else if (m_pRep->SetCycleFrameLod(cycle, 0, 1) != 0) {
        m_pRep->m_bLOD = 1;
    }
    else {
        m_pRep->m_bLOD = 0;
    }
    m_pRep->timer_068 =
        g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    m_pRep->flag_06f = animation->value_02;
    m_pRep->flag_06d = animation->unknown_00[1];
    m_pRep->flag_064 = 0;

    lights = *m_pRep->light_lists[cycle].GetAt(0);
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

/* Reset the launcher's two counters at 0x94 and 0x95. The second is one less
   than the missile's own virtual answer, and the launcher pointer is taken
   before the virtual call rather than after. */
// FUNCTION: WIZ8 0x004a4140
void W8Missile::AdvanceAnimationFrame(int value, int flags)
{
    W8MissileRep* representation_before;

    m_pRep->counter_094 = 0;
    representation_before = m_pRep;
    representation_before->counter_095 = GetNumSubCycles() - 1;
    W8GrCycle::AdvanceAnimationFrame(value, flags);
}
