/*
 * Engine Code\Missile.cpp.
 *
 * What a missile is fired from and where it comes out of. A missile holds a
 * launcher record at 0x1dc; the record carries a small table of emitters at
 * 0xd8 and an index into it at 0xa4, and the accessors below read the chosen
 * emitter, count how many the record has, and reach the emitter's own value.
 */

#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GrObject.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/SpellEffect.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "surrender/srCamera.h"
#include "surrender/srTimer.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/notices.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"
#include "random.h"

#include "wiz8/startup_world.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define MISSILE_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Missile.cpp"

static_assert(sizeof(W8AIMissile) == 0x20, "W8AIMissile_must_be_0x20");

// FUNCTION: WIZ8 0x004a53a0
W8AIMissile* CopyAIMissile004A53A0(const W8AIMissile* source)
{
    W8AIMissile* copy = static_cast<W8AIMissile*>(malloc(sizeof(W8AIMissile)));

    if (copy == 0) {
        srAssertFail("pAIMissile", MISSILE_CPP, 0x86d, 0);
    }
    copy->kind_00 = source->kind_00;
    copy->flag_01 = source->flag_01;
    copy->value_04 = source->value_04;
    copy->value_08 = source->value_08;
    copy->value_10 = source->value_10;
    copy->elapsed_14 = source->elapsed_14;
    copy->limit_18 = source->limit_18;
    copy->flag_1c = source->flag_1c;
    return copy;
}

/* Advance a homing missile one AI step: run the trajectory predictor for the
   clamped half-tick delta, fold the returned advance into the elapsed clock,
   steer the representation while the path is unobstructed, and end the flight
   at expiry or an early-impact limit. */
// FUNCTION: WIZ8 0x004a4cf0
unsigned char UpdateMissileAI004A4CF0(W8AIMissile* record)
{
    W8Missile* missile;
    srVector3T<float> position;
    srVector3T<float> out;
    float advance;
    float remaining;
    float pitch;
    float yaw;
    srMatrix3T<float> rotation;
    unsigned int count;
    unsigned int delta;

    if (record == 0) {
        return 0;
    }
    missile = record->missile_0c;
    if (missile->flag_1e1 != 0) {
        if (missile->flag_1e6 == 0) {
            return 1;
        }
        position = missile->GetPosition();
        pitch = GetElevationToCamera004BE520(&position);
        position = missile->GetPosition();
        yaw = GetHeadingToCamera004BE650(&position);
        rotation.SetIdentity();
        rotation.RotateAboutY(yaw);
        rotation.RotateAboutX(pitch);
        missile->m_pRep->SetRotation004B88D0(&rotation);
        return 1;
    }
    position = missile->GetPosition();
    count = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT) >> 1;
    delta = count - record->value_10;
    if (delta > 0xfa) {
        delta = 0xfa;
    }
    record->value_10 = count;
    if (g_flag_006840bc != 0) {
        return 1;
    }
    advance = AdvanceMissileAI004A50A0(record, &out, delta);
    remaining = missile->duration_1f8 - record->elapsed_14 + 1.0f;
    if (remaining <= advance) {
        advance = remaining;
    }
    if (record->limit_18 > 0.0f) {
        remaining = record->limit_18 - record->elapsed_14 + 1.0f;
        if (remaining <= advance) {
            advance = remaining;
        }
    }
    record->elapsed_14 = advance + record->elapsed_14;
    missile->SetPosition004A6DF0(&out);
    if (missile->CheckNavigatorCollision00453540(&position, &out) == 0 && missile->flag_1e4 != 0) {
        pitch = GetElevationToCamera004BE520(&out);
        yaw = GetHeadingToCamera004BE650(&out);
        rotation.SetIdentity();
        if ((double)yaw != 0.0) {
            rotation.RotateAboutY(sin(yaw), cos(yaw));
        }
        if ((double)pitch != 0.0) {
            rotation.RotateAboutX(sin(pitch), cos(pitch));
        }
        missile->m_pRep->SetRotation004B88D0(&rotation);
    }
    if (advance + record->elapsed_14 <= missile->duration_1f8) {
        if (record->limit_18 > 0.0f && record->limit_18 < advance + record->elapsed_14 &&
            missile->CheckNavigatorCollision00453540(&position, &out) == 0) {
            missile->EnterImpactCycle();
        }
        return 1;
    }
    missile->flag_1e0 = 1;
    if (missile->missile_table_index_1d8 == 0x23 &&
        (g_combat_state == 0 || g_combat_state->unknown_8c4 != 2)) {
        missile->DetonateMissileSpell004A49E0();
    }
    if (g_missile_table_65bde0[missile->missile_table_index_1d8].flag_154 != 0) {
        AbsorbMissileDamage00500460(missile);
    }
    return 1;
}

/* Predict the homing missile's next position `steps` half-ticks ahead and
   re-aim its representation at that point. A ray through the world octree
   flags the record when a prop blocks the path and fires the prop's
   missile trigger. Returns the distance the step covered. */
// FUNCTION: WIZ8 0x004a50a0
float AdvanceMissileAI004A50A0(W8AIMissile* record, srVector3T<float>* out, unsigned int steps)
{
    W8MissileRep* representation;
    W8Missile* missile;
    W8Prop* prop;
    srVector3T<float> direction;
    srVector3T<float> position;
    srVector3T<float> basis;
    srMatrix3T<float> rotation;
    float advance;
    float pitch;
    float yaw;
    int entity;

    representation = record->missile_0c->m_pRep;
    if (steps == 0) {
        *out = representation->location_004;
        return 0.0f;
    }
    advance = (float)steps * record->value_04;
    record->missile_0c->GetVelocity(&direction);
    *out = direction;
    position = record->missile_0c->GetPosition();
    out->x = position.x + direction.x * advance;
    out->y = position.y + direction.y * advance;
    out->z = position.z + direction.z * advance;
    if (g_world->octree != 0 && (entity = TraceAgainstProps00436510(&position, out, 0, 0)) != 0) {
        record->limit_18 = 1.0f;
        prop = *g_world->collidable_props->GetAt(entity - 1);
        prop->RunMissileTrigger0044E230(record);
    }
    if (record->flag_01 != 0) {
        float dx;
        float dy;
        float dz;

        record->value_08 = record->value_08 - (float)steps * (float)g_double_005ece50;
        position = record->missile_0c->GetPosition();
        if (out->y != position.y) {
            dx = representation->location_004.x - out->x;
            dy = representation->location_004.y - out->y;
            dz = representation->location_004.z - out->z;
            if (sqrtf(dx * dx + dy * dy + dz * dz) != g_float_005ebb34) {
                pitch = GetElevationAngle(&representation->location_004, out);
                yaw = GetHeadingAngle(&representation->location_004, out);
                missile = record->missile_0c;
                rotation.vectors[0].x = 1.0f;
                rotation.vectors[0].y = 0.0f;
                rotation.vectors[0].z = 0.0f;
                basis.Set(0.0, 1.0, 0.0);
                rotation.vectors[1] = basis;
                basis.Set(0.0, 0.0, 1.0);
                rotation.vectors[2] = basis;
                if ((double)yaw != 0.0) {
                    rotation.RotateAboutY(sin(yaw), cos(yaw));
                }
                if ((double)pitch != 0.0) {
                    rotation.RotateAboutX(sin(pitch), cos(pitch));
                }
                missile->m_pRep->SetRotation004B88D0(&rotation);
                missile->SetTargetYaw(yaw);
                missile->SetTargetPitch(pitch);
            }
        }
    }
    return advance;
}

// GLOBAL: WIZ8 0x005ece50
const double g_double_005ece50 = 0.009800000000000001;

// GLOBAL: WIZ8 0x0065bde0
W8MissileTableRecord* g_missile_table_65bde0;
// GLOBAL: WIZ8 0x0065bddc
unsigned int g_missile_table_count_65bddc;

/* Engine Code\\Missile.cpp's startup database load.  Each disk row has a
   0x101-byte editor prefix followed by the 0x1e5-byte runtime record. */
// FUNCTION: WIZ8 0x004a5600
unsigned char LoadMissileDatabase(void)
{
    char path[] = "Data\\Databases\\MissileTables.dbs";
    int allocated_count;
    unsigned int record_count;
    unsigned int index;
    int handle;

    if (g_missile_table_65bde0) {
        delete[] g_missile_table_65bde0;
        g_missile_table_65bde0 = 0;
        g_missile_table_count_65bddc = 0;
    }
    handle = FileOpen(path, 0x41, 0);
    if (!handle || !FileRead(handle, &allocated_count, 4, 0) ||
        !FileRead(handle, &record_count, 4, 0)) {
        if (handle) {
            FileClose(handle);
        }
        return 0;
    }
    g_missile_table_65bde0 = new W8MissileTableRecord[allocated_count];
    if (!g_missile_table_65bde0) {
        FileClose(handle);
        return 0;
    }
    for (index = 0; index < record_count; ++index) {
        if (!FileSeek(handle, 0x101, 4) ||
            !FileRead(handle, &g_missile_table_65bde0[index], sizeof(W8MissileTableRecord), 0)) {
            delete[] g_missile_table_65bde0;
            g_missile_table_65bde0 = 0;
            g_missile_table_count_65bddc = 0;
            FileClose(handle);
            return 0;
        }
    }
    g_missile_table_count_65bddc = record_count;
    FileClose(handle);
    return 1;
}

/* Release the one allocation that owns every runtime missile-table row. */
// FUNCTION: WIZ8 0x004a5760
void ReleaseMissileDatabase(void)
{
    if (g_missile_table_65bde0) {
        delete[] g_missile_table_65bde0;
        g_missile_table_65bde0 = 0;
        g_missile_table_count_65bddc = 0;
    }
}

/* True while this missile is still an in-flight engagement that must finish
   before combat can end: either it has not been marked done, or its animation
   state for mode 6 is not the terminal value. */
// FUNCTION: WIZ8 0x004a5790
unsigned char W8Missile::BlocksEndingCombat004A5790()
{
    if (flag_1e0 == 0) {
        if (GetAnimationState004A4640(6) != 1) {
            return 1;
        }
    }
    return 0;
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
W8Missile* NextMissile004A2760(char restart)
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

/* Step every live missile and drop the finished ones.

   A missile still starting, or not yet marked for removal, detaches its
   representation, starts if needed, and updates in place; a finished one
   leaves the world collection and is destroyed. */
// FUNCTION: WIZ8 0x004a27c0
void UpdateWorldMissiles004A27C0(W8World* world)
{
    if (world == 0) {
        srAssertFail("pWorld", MISSILE_CPP, 200, 0);
    }
    srVector3T<double> camera_location = world->camera->getLocation();
    int index = 0;
    int count = world->missiles->GetCount();
    while (index < count) {
        W8Missile* missile = *world->missiles->GetAt(index);
        if (missile != 0) {
            missile->DetachRepresentation004A7A70(world);
            if (missile->flag_1e0 == 0 || missile->flag_1e2 == 0) {
                missile->StartIfHostActive();
                missile->UpdateRepresentation(world);
                missile->UpdateNavigation004553A0(0, 0);
            } else {
                world->missiles->RemoveAt(world->missiles->IndexOf(missile));
                DestroyMissile(missile);
                --index;
                --count;
            }
        }
        ++index;
    }
}

/* Derive the two launch angles from the source and target, then forward the
   remaining launch values to the missile factory. */
// FUNCTION: WIZ8 0x004A2D30
W8Missile* FireMissile004A2D30(unsigned int missile_table_index, srVector3T<float>* source,
                               srVector3T<float>* target, unsigned int value_4,
                               unsigned int value_5, unsigned int value_6, unsigned int value_7)
{
    return CreateMissile004A28D0(missile_table_index, source, GetHeadingAngle(source, target),
                                 GetElevationAngle(source, target), value_4, value_5, value_6,
                                 value_7);
}

/* The missile and spell representations use the same ordinary AnimObj
   operations for these two vtable slots.  Retail points both final tables at
   the corresponding bodies at 0x004AB290 and 0x004AB310. */
srModelInstance* W8MissileRep::SetCycleFrameLod(signed char emitter, signed char frame,
                                                signed char lod)
{
    return AnimObjDispatch004A14D0(emitters[emitter], lod, frame);
}

W8AniMesh* W8MissileRep::GetEmitterAniMesh(char emitter)
{
    /* Emitter slots are a recovered char index into a two-entry table; the
       virtual signature is ABI. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wchar-subscripts"
    W8AnimObj* target = emitters[emitter];
#pragma clang diagnostic pop

    if (target == 0) {
        return 0;
    }
    return static_cast<W8AniMesh*>(AnimObjEntry004A1660(target, m_bLOD, 0));
}

/* Apply the representation's current LOD to one required animation. */
// FUNCTION: WIZ8 0x004A2710
unsigned int W8MissileRep::ApplyEmitterSetting(char emitter)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wchar-subscripts"
    W8AnimObj* target = emitters[emitter];
#pragma clang diagnostic pop

    if (target == 0) {
        srAssertFail("pao", MISSILE_CPP, 0x7e, 0);
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
    : W8EmitterHost(other), value_0ac(other.value_0ac), value_0b0(other.value_0b0)
{
    int emitter;

    for (emitter = 0; emitter < 2; ++emitter) {
        if (other.emitters[emitter] == 0) {
            emitters[emitter] = 0;
            emitter_values[emitter] = 15.0f;
        } else {
            emitters[emitter] = CloneAnimObj004A0320(other.emitters[emitter]);
            emitter_values[emitter] = other.emitter_values[emitter];
        }
    }

    for (emitter = 0; emitter < 2; ++emitter) {
        int list_index;

        for (list_index = 0; list_index < other.light_lists[emitter].GetCount(); ++list_index) {
            W8GrowableVector<stLight*>* source_lights =
                *other.light_lists[emitter].GetAt(list_index);
            W8GrowableVector<stLight*>* copied_lights = 0;

            if (source_lights != 0) {
                int light_index;

                copied_lights = new W8GrowableVector<stLight*>;
                if (copied_lights == 0) {
                    srAssertFail("plsNewLights", MISSILE_CPP, 0x184,
                                 "Out of memory creating monster light list");
                }
                for (light_index = 0; light_index < source_lights->GetCount(); ++light_index) {
                    stLight* source_light = *source_lights->GetAt(light_index);
                    float x = source_light->positionalX();
                    float y = source_light->positionalY();
                    float z = source_light->positionalZ();
                    stLight* copied_light = new stLight;

                    if (copied_light != 0) {
                        *copied_light = *source_light;
                    }
                    if (copied_light == 0) {
                        srAssertFail("pstNewLight", MISSILE_CPP, 0x18c,
                                     "Out of memory creating monster light");
                    }
                    copied_light->ConfigureMonsterCopy();
                    copied_light->setLocation(x, y, z);
                    copied_light->setParent(0, 0);
                    PLAdoptAppend(&g_world->m_lights_0a8, copied_light);
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
unsigned char W8MissileRep::ReadCycleData004A3300(W8ReadLevelInfo* info, W8Missile* missile, int,
                                                  int emitter_index)
{
    W8GrowableVector<stLight*>* lights = new W8GrowableVector<stLight*>;
    W8AnimObj* animation;
    unsigned char success;
    signed char emitter;

    if (info == 0 || info->hFile == 0 || missile == 0) {
        srAssertFail("pInfo && pInfo->hFile && pMissile", MISSILE_CPP, 0x1df, 0);
    }
    animation = CreateAnimObj004A01A0();
    success = AnimObjReadFromFile004A05C0(info, animation, 1, lights, 1);
    emitter = static_cast<signed char>(animation->cycle);

    if (lights->GetCount() == 0) {
        delete lights;
        lights = 0;
    } else {
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
    flag_070 = animation->unknown_03;
    flag_06f = animation->value_02;
    flag_06d = animation->unknown_01;
    emitters[emitter] = animation;

    if (missile != 0) {
        if (SetCycleFrameLod(current_cycle, 0, 2) != 0) {
            m_bLOD = 2;
        } else if (SetCycleFrameLod(current_cycle, 0, 1) != 0) {
            m_bLOD = 1;
        } else {
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
    : missile_table_index_1d8(-1), m_pRep(0), flag_1e0(0), flag_1e1(0), flag_1e2(1), flag_1e3(0),
      flag_1e4(0), flag_1e5(0), flag_1e6(0), flag_1e7(1), value_1e8(0), value_1ec(0),
      lifetime_1f0(15000.0f), value_1f4(0), retargeted_322(false)
{
    W8GrObject::unknown_004 = 1;
    radius_084 = 1.0f;
    if (g_runtime_world_scale_6081e8 < 1.0f) {
        g_runtime_world_scale_6081e8 = 1.0f;
    }
    movement_0c0.value_0b0 = 1.0f;
    if (g_runtime_world_scale_6081e8 < 1.0f) {
        g_runtime_world_scale_6081e8 = 1.0f;
    }
    movement_0c0.alternate_radius_0b4 = 1.0f;
    if (g_runtime_world_scale_6081e8 < 1.0f) {
        g_runtime_world_scale_6081e8 = 1.0f;
    }
    unknown_008 = IncrementValue60DFAC();

    m_pRep = new W8MissileRep;
    if (m_pRep == 0) {
        srAssertFail("m_pRep", MISSILE_CPP, 0x38e, 0);
    }

    memset(&definition_1fc, 0, sizeof(definition_1fc));
    memset(static_cast<void*>(&result_280), 0, 0xa2);
    ResetCombatSlot(&combat_slot_260);
}

// SYNTHETIC: WIZ8 0x004a3e30
// W8Missile::`vector deleting destructor'

// SYNTHETIC: WIZ8 0x004a5da0
// W8Missile::`vector deleting destructor'`adjustor{24}'

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

/* Start or advance the missile while its launcher is live.

   When no scripted phase claims it, the ordinary path hands control to the
   missile's AI and ticks the animation. A scripted missile instead records
   the start, fires the one-shot launch for its table kind, and lets the
   combat boundary consume the animation once its table flag is set. */
// FUNCTION: WIZ8 0x004a4050
void W8Missile::StartIfHostActive()
{
    if (m_pRep->active == 0) {
        return;
    }
    if (GetAnimationState004A4640(2) == 0 || flag_1e1 == 0) {
        if (m_pAI != 0) {
            PathAIUpdate004A9260(static_cast<W8PathAI*>(m_pAI), 1);
        }
        TickAnimation(1.0f);
    } else {
        flag_1e0 = 1;
        if (missile_table_index_1d8 == 0x23 &&
            (g_combat_state == 0 || g_combat_state->unknown_8c4 != 2)) {
            DetonateMissileSpell004A49E0();
        }
        if (g_missile_table_65bde0[missile_table_index_1d8].flag_154 != 0) {
            AbsorbMissileDamage00500460(this);
        }
    }
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
            srAssertFail("pMissile", MISSILE_CPP, 0x4ab, 0);
        }
        g_world->missiles->RemoveAt(g_world->missiles->IndexOf(missile));
        DestroyMissile(missile);
    }
}

// SYNTHETIC: WIZ8 0x004a2d80
// W8MissileRep::`scalar deleting destructor'

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

        for (light_list = 0; light_list < light_lists[emitter].GetCount(); ++light_list) {
            DestroyLightVector(*light_lists[emitter].GetAt(light_list));
        }
        light_lists[emitter].Clear();
    }
}

/* Copy the effect definition, then replace its radius from the selected
   0x1e5-byte missile database row. */
// FUNCTION: WIZ8 0x004A5410
void W8Missile::SetEffectDefinition(const W8SpellEffectDefinition* definition)
{
    memcpy(&definition_1fc, definition, sizeof(definition_1fc));
    definition_1fc.radius = g_missile_table_65bde0[missile_table_index_1d8].radius_140;
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
    return m_pRep->emitters[m_pRep->current_cycle];
}

/* That animation's own playback scale. */
// FUNCTION: WIZ8 0x004a45c0
float W8Missile::GetCurrentAnimationScale()
{
    return m_pRep->emitters[m_pRep->current_cycle]->playback_scale_08;
}

// FUNCTION: WIZ8 0x004a45f0
W8AniMesh* W8Missile::GetCurrentAniMesh()
{
    W8AnimObj* animation = m_pRep->emitters[m_pRep->current_cycle];

    if (animation == 0) {
        srAssertFail("pao", MISSILE_CPP, 0x55e, 0);
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

    return static_cast<signed char>(AnimObjValue004A15D0(animation, m_pRep->m_bLOD));
}

// FUNCTION: WIZ8 0x004a42b0
bool W8Missile::IsCycleSupported(signed char cycle)
{
    if (cycle >= 2) {
        srAssertFail("bCycle<MISSILE_NUM_CYCLES", MISSILE_CPP, 0x4bf, 0);
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
        srAssertFail("bCycle >= MISSILE_CYCLE_FIRST && bCycle <= MISSILE_CYCLE_LAST", MISSILE_CPP,
                     0x4d6, 0);
    }

    lights = *m_pRep->light_lists[m_pRep->current_cycle].GetAt(0);
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
    } else if (m_pRep->SetCycleFrameLod(cycle, 0, 1) != 0) {
        m_pRep->m_bLOD = 1;
    } else {
        m_pRep->m_bLOD = 0;
    }
    m_pRep->timer_068 = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    m_pRep->flag_06f = animation->value_02;
    m_pRep->flag_06d = animation->unknown_01;
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
            } else {
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

/* Cast spell 0x83 at the missile's own position on behalf of the character
   that fired it - the detonation a stored-spell missile (table index 0x23)
   releases once its flight ends. */
// FUNCTION: WIZ8 0x004a49e0
void W8Missile::DetonateMissileSpell004A49E0()
{
    W8TargetSource source;
    W8CombatSlot target;
    srVector3T<float> position;

    if (source_22c.iType != W8_TARGET_SOURCE_CHARACTER) {
        srAssertFail("m_Source.iType == SOURCE_TYPE_CHARACTER", MISSILE_CPP, 0x6c5, 0);
    }
    position = GetPosition();
    ResetTargetSource(&source);
    source.iChar = source_22c.iChar;
    source.iType = W8_TARGET_SOURCE_CHARACTER;
    source.point = position;
    source.unknown_18[2] = 1;
    ResetCombatSlot(&target);
    target.iType = W8_TARGET_KIND_PLACE;
    target.point = position;
    CastSpellFromSource(0x83, &source, &target, 1, 0, 0, 0, 0, 0, 0, 0);
}

/* Post "<source> hits <target>" to the notice box and colour the target's
   name with the target side's colour when it differs from the source's. */
// FUNCTION: WIZ8 0x004a4ac0
void W8Missile::AnnounceCollisionTarget()
{
    wchar_t text[120];
    unsigned char target_start;
    unsigned char target_stop;
    unsigned char source_color;
    unsigned char target_color;

    if (gXStatus.fCombatMode == 0) {
        return;
    }
    swprintf(text, L"%s ", gppStringList[0x6fc / 4]);
    target_start = wcslen(text);
    if (combat_slot_260.iType == W8_TARGET_KIND_MONSTER) {
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x701, MISSILE_CPP, combat_slot_260.iMonsterID, 1);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        wcscat(text, GetMonsterName(monster_info, 0, 0));
    } else {
        wcscat(text, g_status_685170.buffers.characters[combat_slot_260.iChar].name);
    }
    target_stop = wcslen(text);
    source_color = GetSourceNoticeColor(&source_22c);
    target_color = GetTargetNoticeColor(&source_22c, &combat_slot_260);
    wcscat(text, L" ");
    wcscat(text, gppStringList[0x700 / 4]);
    ShowNotice(source_color, text, -1, -1, 0);
    if (target_color != source_color) {
        HighlightTextBoxRange(target_color, target_start, target_stop, -1);
    }
}

// FUNCTION: WIZ8 0x004a4c20
void W8Missile::EnterImpactCycle()
{
    if (IsCycleSupported(1)) {
        if (GetAnimationState004A4640(6) != 1) {
            W8MissileRep* representation = m_pRep;
            srVector3T<float> position = representation->location_004;
            representation->pending_cycle = 1;
            representation->behaviour_071 = 1;
            flag_1e1 = 1;
            if (flag_1e5 != 0) {
                representation->location_004.y = SettlePositionToGround00420BD0(&position, 0);
            }
        }
    } else {
        flag_1e0 = 1;
        if (missile_table_index_1d8 == 0x23 &&
            (g_combat_state == 0 || g_combat_state->unknown_8c4 != 2)) {
            DetonateMissileSpell004A49E0();
        }
        if (g_missile_table_65bde0[missile_table_index_1d8].flag_154 != 0) {
            AbsorbMissileDamage00500460(this);
        }
    }
}

// FUNCTION: WIZ8 0x004a4720
bool W8Missile::OnCollision(W8Navigator* other)
{
    char hit_result;
    unsigned char deflect_chance;

    if (g_startup_world_659c0c == other) {
        if (TargetSourceIsCharacter(&source_22c, 0)) {
            goto miss;
        }
        if (combat_slot_260.iType != W8_TARGET_KIND_PARTY &&
            combat_slot_260.iType != W8_TARGET_KIND_CHARACTER) {
            if (g_missile_table_65bde0[missile_table_index_1d8].flag_154 != 0) {
                goto miss;
            }
            if (combat_slot_260.iType != W8_TARGET_KIND_NONE) {
                retargeted_322 = true;
            }
            combat_slot_260.iType = W8_TARGET_KIND_CHARACTER;
            combat_slot_260.iChar = GetRandomCharacter(1, 1, -1, -1);
            combat_slot_260.iMonsterID = -1;
            if (gXStatus.fCombatMode != 0) {
                AnnounceCollisionTarget();
            }
        }
    } else {
        if (other->movement_0c0.location_id_004 <= 0) {
            goto miss;
        }
        W8Monster* monster = static_cast<W8Monster*>(other);
        int location_id = monster->propagated_value_1e4;
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x636, MISSILE_CPP, location_id, 1);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        if (TargetSourceIsMonster(&source_22c, 0) && source_22c.iMonsterID == location_id) {
            goto miss;
        }
        if (monster_info->hp_current == 0) {
            goto miss;
        }
        if (combat_slot_260.iType != W8_TARGET_KIND_MONSTER ||
            combat_slot_260.iMonsterID != location_id) {
            if (g_missile_table_65bde0[missile_table_index_1d8].flag_154 != 0) {
                goto miss;
            }
            if (combat_slot_260.iType != W8_TARGET_KIND_NONE) {
                retargeted_322 = true;
            }
            combat_slot_260.iType = W8_TARGET_KIND_MONSTER;
            combat_slot_260.iChar = -1;
            combat_slot_260.iMonsterID = location_id;
            if (gXStatus.fCombatMode != 0) {
                AnnounceCollisionTarget();
            }
        }
    }

    hit_result = 1;
    if (g_missile_table_65bde0[missile_table_index_1d8].flag_154 == 0) {
        if (combat_slot_260.iType == W8_TARGET_KIND_CHARACTER) {
            deflect_chance =
                g_status_685170.buffers.characters[combat_slot_260.iChar].bonus_1770.value_49;
        } else {
            W8MonsterInfo* monster_info =
                MonsterInfoFromID(0x676, MISSILE_CPP, combat_slot_260.iMonsterID, 1);
            deflect_chance = monster_info->modifiers_1db.value_49;
        }
        if (deflect_chance > 0 && Random(100) + 1 <= deflect_chance) {
            hit_result = 2;
        }
    }

    if (missile_table_index_1d8 == 0x23) {
        g_combat_state->unknown_8c4 = hit_result;
    } else if (g_combat_state != 0 && g_combat_state->engaged_missile != 0) {
        g_combat_state->unknown_8c4 = hit_result;
        g_combat_state->TargetHit = combat_slot_260;
    } else if (g_missile_table_65bde0[missile_table_index_1d8].flag_154 != 0) {
        ResolveSpellMissileHit(this);
    } else {
        ResolveMissileHit(this, hit_result == 2);
    }
    EnterImpactCycle();
    state_088 = 0;
    return true;

miss:
    return false;
}
