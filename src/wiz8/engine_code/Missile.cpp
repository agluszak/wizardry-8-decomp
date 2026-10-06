#include "wiz8/engine_code/AnimRep.hpp"
#include "wiz8/spell_ids.h"
/*
 * Engine Code\Missile.cpp.
 *
 * What a missile is fired from and where it comes out of. A missile holds a
 * launcher record at 0x1dc; the record carries a small table of emitters at
 * 0xd8 and an index into it at 0xa4, and the accessors below read the chosen
 * emitter, count how many the record has, and reach the emitter's own value.
 */

#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/GameData.h"
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
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/engine_code/SoundEvent.h"
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
#include "wiz8/engine_code/PolyPick.h"
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
#include <stdio.h>
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
W8AIMissile* CopyAIMissile(const W8AIMissile* source)
{
    W8AIMissile* copy = static_cast<W8AIMissile*>(malloc(sizeof(W8AIMissile)));

    if (copy == 0) {
        srAssertFail("pAIMissile", MISSILE_CPP, 0x86d, 0);
    }
    copy->kind = source->kind;
    copy->gravity = source->gravity;
    copy->speed_per_step = source->speed_per_step;
    copy->fall_speed = source->fall_speed;
    copy->last_half_tick = source->last_half_tick;
    copy->elapsed = source->elapsed;
    copy->limit = source->limit;
    copy->unknown_1c = source->unknown_1c;
    return copy;
}

/* Advance a homing missile one AI step: run the trajectory predictor for the
   clamped half-tick delta, fold the returned advance into the elapsed clock,
   steer the representation while the path is unobstructed, and end the flight
   at expiry or an early-impact limit. */
// FUNCTION: WIZ8 0x004a4cf0
unsigned char UpdateMissileAI(W8AIMissile* record)
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
    missile = record->missile;
    if (missile->impacting) {
        if (!missile->align_explosion) {
            return 1;
        }
        position = missile->GetPosition();
        pitch = ElevationToTargetCPP(&position);
        position = missile->GetPosition();
        yaw = HeadingToTargetCPP(&position);
        rotation.SetIdentity();
        rotation.RotateAboutY(yaw);
        rotation.RotateAboutX(pitch);
        missile->m_pRep->SetRotation(&rotation);
        return 1;
    }
    position = missile->GetPosition();
    count = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT) >> 1;
    delta = count - record->last_half_tick;
    if (delta > 0xfa) {
        delta = 0xfa;
    }
    record->last_half_tick = count;
    if (gXStatus.world_update_blocked) {
        return 1;
    }
    advance = AdvanceMissileAI(record, &out, delta);
    remaining = missile->duration - record->elapsed + 1.0f;
    if (remaining <= advance) {
        advance = remaining;
    }
    if (record->limit > 0.0f) {
        remaining = record->limit - record->elapsed + 1.0f;
        if (remaining <= advance) {
            advance = remaining;
        }
    }
    record->elapsed = advance + record->elapsed;
    missile->SetCyclePosition(&out);
    if (!missile->CheckNavigatorCollision(&position, &out) && missile->align_camera) {
        pitch = ElevationToTargetCPP(&out);
        yaw = HeadingToTargetCPP(&out);
        rotation.SetIdentity();
        if (yaw != 0.0) {
            rotation.RotateAboutY(sin(yaw), cos(yaw));
        }
        if (pitch != 0.0) {
            rotation.RotateAboutX(sin(pitch), cos(pitch));
        }
        missile->m_pRep->SetRotation(&rotation);
    }
    if (advance + record->elapsed <= missile->duration) {
        if (record->limit > 0.0f && record->limit < advance + record->elapsed &&
            !missile->CheckNavigatorCollision(&position, &out)) {
            missile->EnterImpactCycle();
        }
        return 1;
    }
    missile->flight_done = true;
    if (missile->missile_table_index == 0x23 &&
        (g_combat_state == 0 || g_combat_state->missile_hit_result != W8_MISSILE_HIT_DEFLECTED)) {
        missile->DetonateMissileSpell();
    }
    if (g_missile_table[missile->missile_table_index].spell_missile != 0) {
        AbsorbMissileDamage(missile);
    }
    return 1;
}

/* Predict the homing missile's next position `steps` half-ticks ahead and
   re-aim its representation at that point. A ray through the world octree
   flags the record when a prop blocks the path and fires the prop's
   missile trigger. Returns the distance the step covered. */
// FUNCTION: WIZ8 0x004a50a0
float AdvanceMissileAI(W8AIMissile* record, srVector3T<float>* out, unsigned int steps)
{
    W8MissileRep* representation;
    W8Missile* missile;
    W8Prop* prop;
    srVector3T<float> direction;
    srVector3T<float> position;
    srMatrix3T<float> rotation;
    float advance;
    float pitch;
    float yaw;
    int entity;

    representation = record->missile->m_pRep;
    if (steps == 0) {
        *out = representation->location;
        return 0.0f;
    }
    advance = steps * record->speed_per_step;
    record->missile->GetVelocity(&direction);
    *out = direction;
    position = record->missile->GetPosition();
    out->Set(position.x + direction.x * advance, position.y + direction.y * advance,
             position.z + direction.z * advance);
    if (g_world->octree != 0 &&
        (entity = g_world->octree->TraceAgainstProps(&position, out, 0, 0)) != 0) {
        record->limit = 1.0f;
        prop = *g_world->collidable_props->GetAt(entity - 1);
        prop->RunMissileTrigger(record);
    }
    if (record->gravity) {
        float dx;
        float dy;
        float dz;

        record->fall_speed =
            record->fall_speed - steps * static_cast<float>(g_missile_gravity_acceleration);
        position = record->missile->GetPosition();
        if (out->y != position.y) {
            dx = representation->location.x - out->x;
            dy = representation->location.y - out->y;
            dz = representation->location.z - out->z;
            if (sqrtf(dx * dx + dy * dy + dz * dz) != g_float_zero) {
                pitch = GetElevationAngle(&representation->location, out);
                yaw = GetHeadingAngle(&representation->location, out);
                missile = record->missile;
                rotation.SetIdentity();
                if (yaw != 0.0) {
                    rotation.RotateAboutY(sin(yaw), cos(yaw));
                }
                if (pitch != 0.0) {
                    rotation.RotateAboutX(sin(pitch), cos(pitch));
                }
                missile->m_pRep->SetRotation(&rotation);
                missile->SetTargetYaw(yaw);
                missile->SetTargetPitch(pitch);
            }
        }
    }
    return advance;
}

// GLOBAL: WIZ8 0x005ece50
const double g_missile_gravity_acceleration = 0.009800000000000001;
// GLOBAL: WIZ8 0x005ece58
const float g_character_projectile_height = 81.25f;
// GLOBAL: WIZ8 0x005ece5c
const float g_character_projectile_height_step = 32.5f;

// GLOBAL: WIZ8 0x0065bde0
W8MissileTableRecord* g_missile_table;

// GLOBAL: WIZ8 0x0060c9c8
const char* g_missile_cycle_names[] = {"FLY", "EXPLODE"};
// GLOBAL: WIZ8 0x0065bddc
unsigned int g_missile_table_count;

/* Engine Code\\Missile.cpp's startup database load.  Each disk row has a
   0x101-byte editor prefix followed by the 0x1e5-byte runtime record. */
// FUNCTION: WIZ8 0x004a5600
unsigned char LoadMissileDatabase(void)
{
    int allocated_count;
    unsigned int database_version;
    unsigned int index;
    int handle;
    bool success;

    ReleaseMissileDatabase();
    handle = FileOpen("Data\\Databases\\MissileTables.dbs", 0x41, 0);
    if (!handle) {
        return 0;
    }
    success = FileRead(handle, &allocated_count, 4, 0) && FileRead(handle, &database_version, 4, 0);
    g_missile_table = new W8MissileTableRecord[allocated_count];
    if (!g_missile_table) {
        srAssertFail("s_pMissileTable", MISSILE_CPP, 0x8d6, 0);
    }
    for (index = 0; index < static_cast<unsigned int>(allocated_count); ++index) {
        if (!success) {
            break;
        }
        success = FileSeek(handle, 0x101, 4) &&
                  FileRead(handle, &g_missile_table[index], sizeof(W8MissileTableRecord), 0);
    }
    if (success) {
        g_missile_table_count = allocated_count;
    } else {
        delete[] g_missile_table;
        g_missile_table = 0;
        g_missile_table_count = 0;
    }
    FileClose(handle);
    return success;
}

/* Release the one allocation that owns every runtime missile-table row. */
// FUNCTION: WIZ8 0x004a5760
void ReleaseMissileDatabase(void)
{
    if (g_missile_table) {
        delete[] g_missile_table;
        g_missile_table = 0;
        g_missile_table_count = 0;
    }
}

/* True while this missile is still an in-flight engagement that must finish
   before combat can end: either it has not been marked done, or its animation
   state for mode 6 is not the terminal value. */
// FUNCTION: WIZ8 0x004a5790
bool W8Missile::BlocksEndingCombat()
{
    if (!flight_done) {
        if (GetAnimationState(6) != 1) {
            return true;
        }
    }
    return false;
}

// FUNCTION: WIZ8 0x004A57B0
void GetCharacterProjectilePosition(unsigned int character_index, srVector3T<float>* position)
{
    srVector3T<float> camera;
    srMatrix3T<float> rotation;
    srVector3T<float> first;
    srVector3T<float> second;
    srVector3T<float> third;
    srMatrix3T<float> step;
    float angle;
    double cosine;
    double sine;

    GetCameraPosition(&camera);
    position->SetZero();
    if ((character_index & 1) == 0) {
        position->x = 75.0f;
    } else {
        position->x = -75.0f;
    }
    rotation.SetIdentity();
    position->y = g_character_projectile_height -
                  character_index * g_float_half * g_character_projectile_height_step;
    angle = GetCameraYawRadians() - g_monster_rotation_offset;
    if (angle != 0.0) {
        cosine = cos(angle);
        sine = sin(angle);
        third.Set(static_cast<float>(-sine), 0.0f, static_cast<float>(cosine));
        second.Set(0.0, 1.0, 0.0);
        first.Set(cosine, 0.0, sine);
        step.SetRows(first, second, third);
        rotation.MultiplyBy(step);
    }
    angle = -GetCameraPitchRadians();
    if (angle != 0.0) {
        cosine = cos(angle);
        sine = sin(angle);
        third.Set(0.0f, static_cast<float>(sine), static_cast<float>(cosine));
        second.Set(0.0, cosine, -sine);
        first.Set(1.0, 0.0, 0.0);
        step.SetRows(first, second, third);
        rotation.MultiplyBy(step);
    }
    position->x = DotProduct(rotation.vectors[0], *position);
    position->y = DotProduct(rotation.vectors[1], *position);
    position->z = DotProduct(rotation.vectors[2], *position);
    *position += camera;
}

// VTABLE: WIZ8 0x005ecde0 W8MissileRep
// class W8MissileRep

// VTABLE: WIZ8 0x005ece08 W8GrObject
// VTABLE: WIZ8 0x005ecdf4 W8Navigator
// class W8Missile

// GLOBAL: WIZ8 0x0065BDE4
static int g_missile_iterator;

/* Iterate the world's missile vector. A nonzero argument restarts the shared
   cursor; a missing world or vector answers null. */
// FUNCTION: WIZ8 0x004A2760
W8Missile* NextMissile(bool restart)
{
    W8Missile* missile = 0;

    if (g_world != 0 && g_world->missiles != 0) {
        if (restart) {
            g_missile_iterator = 0;
        }
        if (g_missile_iterator < g_world->missiles->GetCount()) {
            missile = *g_world->missiles->GetAt(g_missile_iterator);
            ++g_missile_iterator;
        }
    }
    return missile;
}

/* Step every live missile and drop the finished ones.

   A missile still starting, or not yet marked for removal, detaches its
   representation, starts if needed, and updates in place; a finished one
   leaves the world collection and is destroyed. */
// FUNCTION: WIZ8 0x004a27c0
void UpdateWorldMissiles(W8World* world)
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
            missile->DetachRepresentation(world);
            if (!missile->flight_done || missile->block_released == 0) {
                missile->StartIfHostActive();
                missile->UpdateRepresentation(world);
                missile->UpdateNavigation(0, false);
            } else {
                world->missiles->Remove(missile);
                missile->DestroyMissile();
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
W8Missile* FireMissile(unsigned int missile_table_index, srVector3T<float>* source,
                       srVector3T<float>* target, float flight_speed, unsigned int trace_mask,
                       unsigned int block_released, float duration)
{
    return CreateMissile(missile_table_index, source, GetHeadingAngle(source, target),
                         GetElevationAngle(source, target), flight_speed, trace_mask,
                         block_released, duration);
}

/* Instantiate the missile for `missile_table_index`: load or clone the cycle
   the table row names, then stamp the index, register the result on the world
   missile list and give it the default navigation bounds. */
// FUNCTION: WIZ8 0x004A5450
W8Missile* AllocateMissile(int missile_table_index)
{
    W8GrCycleLoadContext context;
    W8Missile* missile;
    srVector3T<float> minimum;
    srVector3T<float> maximum;

    context.world = g_world;
    context.directory = "Data\\Spells\\Bitmaps";
    missile = 0;
    LoadMissileCycle(&context, g_missile_table[missile_table_index].cycle_name, &missile, 1);
    if (missile == 0) {
        return 0;
    }
    missile->missile_table_index = missile_table_index;
    missile->m_pRep->pending_cycle = 0;
    missile->flight_done = false;
    if (missile->flight_done) {
        if (missile->missile_table_index == 0x23 &&
            (g_combat_state == 0 ||
             g_combat_state->missile_hit_result != W8_MISSILE_HIT_DEFLECTED)) {
            missile->DetonateMissileSpell();
        }
        if (g_missile_table[missile->missile_table_index].spell_missile != 0) {
            AbsorbMissileDamage(missile);
        }
    }
    missile->impacting = false;
    g_world->missiles->Add(missile);
    minimum = -125.0;
    maximum = 125.0;
    missile->SetBounds(&minimum, &maximum);
    missile->active = true;
    return missile;
}

/* Load the cycle a missile record names. A name already on the GrCycle
   registry clones the registered template into a fresh W8Missile; otherwise
   the "data\\Missiles\\<name>.mls" script is parsed - its align/explode/
   gravity/velocity keywords set the missile's tail flags - and the cycle
   lines load through the ordinary GrCycle path. */
// FUNCTION: WIZ8 0x004a3550
unsigned char LoadMissileCycle(W8GrCycleLoadContext* context, const char* name,
                               W8Missile** ppMissile, int)
{
    W8GrCycle* found;
    W8GrCycle* loaded_cycle;
    W8Missile* missile;
    W8AIMissile* ai;
    bool loaded;
    unsigned char more;
    bool gravity;
    bool align_camera;
    bool explode_ground;
    bool align_explosion;
    float velocity;
    int handle;
    W8SoundEventKind sound_kind;
    int frame;
    int cycle;
    int index;
    float duration;
    float seconds;
    float intensity;
    W8CameraShakeEffect* effect;
    W8SoundEvent* sound;
    char path[100];
    char line[100];
    char pacName[52];
    char pacFileName[52];
    char pacToken[52];
    char wave_path[256];
    char pacLoop[128];

    found = FindFirstGrCycleByName(name);
    if (found != 0) {
        missile = new W8Missile(*static_cast<W8Missile*>(found));
        if (missile == 0) {
            srAssertFail("pMissile", MISSILE_CPP, 0x3eb, 0);
        }
        ai = static_cast<W8AIMissile*>(missile->m_pAI);
        if (ai->kind != W8_AI_RECORD_MISSILE) {
            srAssertFail("pAI->ubAIType == AI_TYPE_MISSILE", MISSILE_CPP, 0x3f1, 0);
        }
        ai->missile = missile;
        *ppMissile = missile;
        if (missile == 0) {
            srAssertFail("*ppMissile", MISSILE_CPP, 0x285, 0);
        }
        RegisterGrCycle(name, *ppMissile);
        return 1;
    }
    PauseSharedGameTimers();
    more = 1;
    loaded = true;
    gravity = false;
    align_camera = false;
    explode_ground = false;
    align_explosion = false;
    velocity = 15000.0f;
    sprintf(path, "data\\Missiles\\%s.mls", name);
    handle = FileOpen(path, 0x41, 0);
    *ppMissile = 0;
    if (handle != 0) {
        for (;;) {
            do {
                if (more == 0 || !loaded) {
                    goto close_file;
                }
                ReadTextLine(handle, line, 100, &more);
            } while (line[0] == '#');
            pacName[0] = 0;
            pacFileName[0] = '\0';
            sscanf(line, "%s %s", pacName, pacFileName);
            if (_stricmp("align_camera", pacName) != 0) {
                if (_stricmp("explode_ground", pacName) == 0) {
                    explode_ground = true;
                } else if (_stricmp("align_explosion", pacName) == 0) {
                    align_explosion = true;
                } else if (_stricmp("gravity", pacName) == 0) {
                    gravity = true;
                } else if (strcmp("velocity", pacName) == 0) {
                    sscanf(line, "%s %f", pacName, &velocity);
                } else {
                    if (strlen(line) > 2) {
                        /* Retail asserts the array address; always true. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
                        if (pacName == 0) {
#pragma clang diagnostic pop
                            srAssertFail("pacName", MISSILE_CPP, 0x32f, 0);
                        }
                        for (index = 0; index < 2; ++index) {
                            if (_strnicmp(pacName, g_missile_cycle_names[index],
                                          strlen(g_missile_cycle_names[index])) == 0) {
                                loaded_cycle = *ppMissile;
                                loaded = LoadGrCycle(context, pacFileName, &loaded_cycle, index, 1,
                                                     "Data\\Missiles", 1, "Data\\Spells\\Bitmaps");
                                if (loaded) {
                                    *ppMissile = static_cast<W8Missile*>(loaded_cycle);
                                }
                                goto next_line;
                            }
                        }
                    }
                    if (_stricmp(pacName, "SOUND_FRAME") == 0) {
                        sound_kind = W8_SOUND_EVENT_FRAME;
                    } else {
                        if (_stricmp(pacName, "SOUND_CYCLE") != 0) {
                            if (_stricmp(pacName, "SHAKE_FRAME") == 0) {
                                duration = 1.0f;
                                seconds = 10.0f;
                                intensity = 1.0f;
                                sscanf(line, "%s %s %d %f %f %f", pacToken, pacName, &frame,
                                       &intensity, &duration, &seconds);
                                /* Retail asserts the array address; always
                                   true. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
                                if (pacName == 0) {
#pragma clang diagnostic pop
                                    srAssertFail("pacName", MISSILE_CPP, 0x32f, 0);
                                }
                                cycle = -1;
                                for (index = 0; index < 2; ++index) {
                                    if (_strnicmp(pacName, g_missile_cycle_names[index],
                                                  strlen(g_missile_cycle_names[index])) == 0) {
                                        cycle = index;
                                        break;
                                    }
                                }
                                effect = new W8CameraShakeEffect(duration, true, intensity,
                                                                 seconds * g_world_scale, 0);
                                if (effect != 0) {
                                    effect->cycle = cycle;
                                    effect->frame = frame;
                                    effect->subcycle = 0;
                                    (*ppMissile)->AddShakeEffect(effect);
                                }
                            }
                            goto next_line;
                        }
                        sound_kind = W8_SOUND_EVENT_CYCLE;
                    }
                    memcpy(pacLoop, &g_empty_ambient_name, 2);
                    memset(pacLoop + 2, 0, sizeof(pacLoop) - 2);
                    sscanf(line, "%s %s %d %s %s", pacToken, pacName, &frame, pacFileName, pacLoop);
                    /* Retail asserts the array address; always true. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
                    if (pacName == 0) {
#pragma clang diagnostic pop
                        srAssertFail("pacName", MISSILE_CPP, 0x32f, 0);
                    }
                    cycle = -1;
                    for (index = 0; index < 2; ++index) {
                        if (_strnicmp(pacName, g_missile_cycle_names[index],
                                      strlen(g_missile_cycle_names[index])) == 0) {
                            cycle = index;
                            break;
                        }
                    }
                    sprintf(wave_path, "Data\\Missiles\\Sounds\\%s.WAV", pacFileName);
                    sound = CreateSoundEvent(sound_kind, cycle, frame, 0, wave_path,
                                             _stricmp(pacLoop, "LOOP") == 0);
                    if (sound != 0) {
                        (*ppMissile)->AddSoundEvent(sound);
                    }
                }
            } else {
                align_camera = true;
            }
        next_line:;
        }
    }
    loaded_cycle = *ppMissile;
    loaded = LoadGrCycle(context, name, &loaded_cycle, 0, 1, "Data\\Missiles", 1,
                         "Data\\Spells\\Bitmaps");
    if (loaded) {
        *ppMissile = static_cast<W8Missile*>(loaded_cycle);
        RegisterGrCycle(name, *ppMissile);
    }
    ResumeSharedGameTimers();
    return loaded;

close_file:
    FileClose(handle);
    if (*ppMissile != 0) {
        (*ppMissile)->gravity = gravity;
        (*ppMissile)->align_camera = align_camera;
        (*ppMissile)->explode_ground = explode_ground;
        (*ppMissile)->align_explosion = align_explosion;
        (*ppMissile)->lifetime = velocity * g_world_scale;
    }
    if (loaded) {
        RegisterGrCycle(name, *ppMissile);
    }
    ResumeSharedGameTimers();
    return loaded;
}

/* Build the launch state for a fresh missile: rotate the forward direction by
   the heading and pitch, position and aim the navigator, probe the world
   octree for an early-impact limit, and attach the kind-3 AI record that
   drives the flight. */
// FUNCTION: WIZ8 0x004A28D0
W8Missile* CreateMissile(unsigned int missile_table_index, srVector3T<float>* source, float heading,
                         float pitch, float flight_speed, unsigned int trace_mask,
                         unsigned char block_released, float duration)
{
    W8Missile* missile;
    W8Octree* octree;
    W8AIMissile* ai;
    srVector3T<float> direction(0.0f, 0.0f, 1.0f);
    srVector3T<float> end;
    srMatrix3T<float> rotation;
    srMatrix3T<float> aim;
    float limit = -1.0f;

    missile = AllocateMissile(missile_table_index);
    if (missile != 0) {
        missile->m_pRep->pending_cycle = 0;
        missile->SetCycle(0);
        missile->m_pRep->pending_behaviour = W8_ANIMATION_NEVER_STOP;
        missile->m_pRep->pending_subcycle = 0;
        missile->m_pRep->subcycle = 0;
        rotation.SetIdentity();
        if (heading != g_double_zero) {
            rotation.RotateAboutY(sin(heading), cos(heading));
        }
        if (pitch != g_double_zero) {
            rotation.RotateAboutX(sin(pitch), cos(pitch));
        }
        direction.Transform(rotation);
        missile->SetVelocity(&direction);
        missile->SetCyclePosition(source);
        aim.SetIdentity();
        aim.RotateAboutY(heading);
        aim.RotateAboutX(pitch);
        missile->m_pRep->SetRotation(&aim);
        missile->SetAngles(heading);
        missile->SetPitch(pitch);
        octree = g_world->octree;
        if (octree != 0) {
            end.Set(direction.x * duration, direction.y * duration, direction.z * duration);
            end = end + *source;
            if (octree->TraceLineOfSight(source, &end, false, -3, -3, true, 0) != 0) {
                end -= *source;
                limit = end.Length();
                if (limit < g_float_one) {
                    limit = 1.0f;
                }
            }
        }
        if (flight_speed == g_float_zero) {
            flight_speed = missile->lifetime;
        }
        if (missile->m_pAI != 0) {
            free(missile->m_pAI);
        }
        ai = static_cast<W8AIMissile*>(malloc(sizeof(W8AIMissile)));
        if (ai != 0) {
            float scale = flight_speed * g_float_one_thousandth;
            memset(ai, 0, sizeof(W8AIMissile));
            ai->speed_per_step = scale;
            ai->kind = W8_AI_RECORD_MISSILE;
            ai->gravity = missile->gravity;
            ai->limit = limit;
            ai->last_half_tick = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT) >> 1;
            ai->missile = missile;
        }
        missile->m_pAI = ai;
        if (ai == 0) {
            srAssertFail("pMissile->GrObject::GetAI()", MISSILE_CPP, 0x120, 0);
        }
        ai->fall_speed = static_cast<float>(sin(-pitch) * flight_speed * g_float_one_thousandth);
        ai->limit = limit;
        missile->trace_mask = trace_mask;
        missile->duration = duration;
        missile->block_released = block_released;
    }
    return missile;
}

/* The missile and spell representations use the same ordinary AnimObj
   operations for these two vtable slots.  Retail points both final tables at
   the corresponding bodies at 0x004AB290 and 0x004AB310. */
srModelInstance* W8MissileRep::SetCycleFrameLod(signed char emitter, signed char frame,
                                                signed char lod)
{
    return AnimObjDispatch(emitters[emitter], lod, frame);
}

W8AniMesh* W8MissileRep::GetEmitterAniMesh(signed char emitter)
{
    W8AnimObj* target = emitters[emitter];

    if (target == 0) {
        return 0;
    }
    return AnimObjEntry(target, m_bLOD, 0);
}

/* Apply the representation's current LOD to one required animation. */
// FUNCTION: WIZ8 0x004A2710
unsigned int W8MissileRep::ApplyEmitterSetting(signed char emitter)
{
    W8AnimObj* target = emitters[emitter];

    if (target == 0) {
        srAssertFail("pao", MISSILE_CPP, 0x7e, 0);
    }
    return AnimObjValue(target, m_bLOD);
}

/* The two emitter slots start empty and at the source default playback value.
   Construction of the two light-list vectors is ordinary array-member
   construction and precedes these assignments in the retail body. */
W8MissileRep::W8MissileRep()
{
    emitters[0] = 0;
    emitters[1] = 0;
    emitter_playback_scales[0] = 15.0f;
    emitter_playback_scales[1] = 15.0f;
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
            emitter_playback_scales[emitter] = 15.0f;
        } else {
            emitters[emitter] = CloneAnimObj(other.emitters[emitter]);
            emitter_playback_scales[emitter] = other.emitter_playback_scales[emitter];
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

                copied_lights = new W8Vector<stLight*>;
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
                    PLAdoptAppend(&g_world->transient_lights, copied_light);
                    copied_lights->Add(copied_light);
                }
            }
            light_lists[emitter].Add(copied_lights);
        }
    }
}

// FUNCTION: WIZ8 0x004a5d40
W8AnimRepBase* W8MissileRep::Clone()
{
    return new W8MissileRep(*this);
}

// FUNCTION: WIZ8 0x004A3300
unsigned char W8MissileRep::ReadCycleData(W8ReadLevelInfo* info, W8Missile* missile,
                                          int cycle_index, int)
{
    W8GrowableVector<stLight*>* lights = new W8Vector<stLight*>;
    W8AnimObj* animation;
    unsigned char success;
    signed char emitter;

    if (info == 0 || info->hFile == 0 || missile == 0) {
        srAssertFail("pInfo && pInfo->hFile && pMissile", MISSILE_CPP, 0x1df, 0);
    }
    animation = CreateAnimObj();
    success = AnimObjReadFromFile(info, animation, 1, lights, 1);
    emitter = static_cast<signed char>(animation->cycle);

    if (lights->GetCount() == 0) {
        delete lights;
        lights = 0;
    } else {
        missile->SetLights(lights);
    }
    light_lists[cycle_index].Add(lights);

    if (cycle_index != -1) {
        emitter = static_cast<signed char>(cycle_index);
        current_cycle = emitter;
    }
    emitter_playback_scales[emitter] = animation->playback_scale;
    active = 1;
    frame_direction = W8_ANIMATION_FORWARD;
    timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    animation_behaviour = animation->behaviour;
    frame_method = animation->frame_method;
    animation_playing = animation->animation_playing;
    emitters[emitter] = animation;

    if (missile->m_pRep != 0) {
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
    : missile_table_index(-1), flight_done(0), impacting(0), block_released(1), gravity(0),
      align_camera(0), explode_ground(0), align_explosion(0), flag(1), value_1e8(0), value_1ec(0),
      lifetime(15000.0f), flags0(0), retargeted(false)
{
    W8GrObject::kind = 1;
    radius = 1.0f;
    if (g_runtime_world_scale < 1.0f) {
        g_runtime_world_scale = 1.0f;
    }
    movement.collision_radius = 1.0f;
    if (g_runtime_world_scale < 1.0f) {
        g_runtime_world_scale = 1.0f;
    }
    movement.alternate_radius = 1.0f;
    if (g_runtime_world_scale < 1.0f) {
        g_runtime_world_scale = 1.0f;
    }
    id = AllocateGrObjectId();

    m_pRep = new W8MissileRep;
    if (m_pRep == 0) {
        srAssertFail("m_pRep", MISSILE_CPP, 0x38e, 0);
    }

    memset(&definition, 0, sizeof(definition));
    memset(&result, 0, sizeof(result));
    ResetCombatSlot(&combat_slot);
}

/* The clone path reuses the registered template's flight configuration but
   drops its in-flight state: the detonate/hit flags clear, duration
   resets, and the AI record is not copied. Retail still clears the complete
   +0x280..+0x321 tail after result's member construction, leaking the
   growable vector's initial allocation exactly as the default constructor
   does. */
// FUNCTION: WIZ8 0x004A3E50
W8Missile::W8Missile(const W8Missile& other)
    : W8GrCycle(other), missile_table_index(other.missile_table_index), flight_done(0),
      impacting(0), block_released(other.block_released), gravity(other.gravity),
      align_camera(other.align_camera), explode_ground(other.explode_ground),
      align_explosion(other.align_explosion), flag(other.flag), value_1e8(other.value_1e8),
      value_1ec(other.value_1ec), lifetime(other.lifetime), flags0(other.flags0), duration(0.0f),
      retargeted(false)
{
    W8GrObject::kind = 1;
    id = AllocateGrObjectId();
    m_pRep = static_cast<W8MissileRep*>(other.m_pRep->Clone());

    memset(&definition, 0, sizeof(definition));
    memset(&result, 0, sizeof(result));
    ResetCombatSlot(&combat_slot);
}

/* Mode-keyed query over the representation's emitter/cycle state; used by the
   missile script handlers. */
// FUNCTION: WIZ8 0x004A4640
unsigned long W8Missile::GetAnimationState(int mode)
{
    switch (mode) {
    case 0:
        return m_pRep->ApplyEmitterSetting(m_pRep->current_cycle);
    case 1:
        return GetNumSubCycles();
    case 2:
        return m_pRep->subcycle == m_pRep->ApplyEmitterSetting(m_pRep->current_cycle) - 1;
    case 3:
        return m_pRep->subcycle == 0;
    case 4:
        return m_pRep->subcycle;
    case 5:
        return m_pRep->ApplyEmitterSetting(m_pRep->current_cycle) != 0xffffffff;
    case 6:
        return m_pRep->current_cycle;
    case 7:
        return m_pRep->animation_playing == 0;
    default:
        return 0xffffffff;
    }
}

/* Release the representation and every external reference before ordinary
   vector and GrCycle teardown. */
// FUNCTION: WIZ8 0x004a3fc0
W8Missile::~W8Missile()
{
    SetLights(0);
    delete m_pRep;
    DetachMissileReferences(this);
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
    if (GetAnimationState(2) == 0 || !impacting) {
        if (m_pAI != 0) {
            PathAIUpdate(m_pAI, 1);
        }
        W8GrCycle::TickAnimation(1.0f);
    } else {
        flight_done = true;
        if (missile_table_index == 0x23 &&
            (g_combat_state == 0 ||
             g_combat_state->missile_hit_result != W8_MISSILE_HIT_DEFLECTED)) {
            DetonateMissileSpell();
        }
        if (g_missile_table[missile_table_index].spell_missile) {
            AbsorbMissileDamage(this);
        }
    }
}

/* Remove a missile's world lights and AI allocation, then release the object. */
// FUNCTION: WIZ8 0x004a4180
void W8Missile::DestroyMissile()
{
    if (m_plsLights != 0) {
        int count = m_plsLights->GetCount();

        while (count != 0) {
            stLight* light = m_plsLights->RemoveAt(0);
            WorldRemoveLight(g_world, light);
            --count;
        }
    }
    if (m_pAI != 0) {
        free(m_pAI);
        m_pAI = 0;
    }
    delete this;
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
        g_world->missiles->Remove(missile);
        missile->DestroyMissile();
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
            DestroyAnimObj(emitters[emitter]);
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
    memcpy(&this->definition, definition, sizeof(this->definition));
    this->definition.radius = g_missile_table[missile_table_index].radius;
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
    return m_pRep->emitters[m_pRep->current_cycle]->playback_scale;
}

// FUNCTION: WIZ8 0x004a45f0
W8AniMesh* W8Missile::GetCurrentAniMesh()
{
    W8AnimObj* animation = m_pRep->emitters[m_pRep->current_cycle];

    if (animation == 0) {
        srAssertFail("pao", MISSILE_CPP, 0x55e, 0);
    }
    return animation->entries[m_pRep->m_bLOD];
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

    return static_cast<signed char>(AnimObjValue(animation, m_pRep->m_bLOD));
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
/* 0x004A4440 is a split address inside this body: it resumes at the
   animation-record field copy after the timestamp read, not a separate
   authored function. */
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
    DetachCycleLights(lights);

    m_pRep->current_cycle = cycle;
    animation = m_pRep->emitters[cycle];
    m_pRep->active = 1;
    m_pRep->frame_direction = W8_ANIMATION_FORWARD;
    if (m_pRep->SetCycleFrameLod(cycle, 0, 2) != 0) {
        m_pRep->m_bLOD = 2;
    } else if (m_pRep->SetCycleFrameLod(cycle, 0, 1) != 0) {
        m_pRep->m_bLOD = 1;
    } else {
        m_pRep->m_bLOD = 0;
    }
    m_pRep->timer = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    m_pRep->frame_method = animation->frame_method;
    m_pRep->animation_playing = animation->animation_playing;
    m_pRep->subcycle = 0;

    lights = *m_pRep->light_lists[cycle].GetAt(0);
    SetLights(lights);
    AttachCycleLights(lights);

    if (m_plsParticles != 0) {
        for (index = 0; index < m_plsParticles->GetCount(); ++index) {
            W8GrCycleParticleAttachment* event = *m_plsParticles->GetAt(index);

            if (event->cycle == cycle) {
                event->m_pstParticles->SetActive(1);
                event->m_pstParticles->emission_count = 0;
            } else {
                event->m_pstParticles->SetActive(0);
            }
        }
    }
}

/* Reset the launcher's two counters at 0x94 and 0x95. The second is one less
   than the missile's own virtual answer, and the launcher pointer is taken
   before the virtual call rather than after. */
// FUNCTION: WIZ8 0x004a4140
void W8Missile::AdvanceAnimationFrame(int value, int arg_flags)
{
    W8MissileRep* representation_before;

    m_pRep->first_frame = 0;
    representation_before = m_pRep;
    representation_before->last_frame = GetNumSubCycles() - 1;
    W8GrCycle::AdvanceAnimationFrame(value, arg_flags);
}

/* Cast spell 0x83 at the missile's own position on behalf of the character
   that fired it - the detonation a stored-spell missile (table index 0x23)
   releases once its flight ends. */
// FUNCTION: WIZ8 0x004a49e0
void W8Missile::DetonateMissileSpell()
{
    W8TargetSource source;
    W8CombatSlot target;
    srVector3T<float> position;

    if (m_Source.iType != W8_TARGET_SOURCE_CHARACTER) {
        srAssertFail("m_Source.iType == SOURCE_TYPE_CHAR", MISSILE_CPP, 0x6c5, 0);
    }
    position = GetPosition();
    ResetTargetSource(&source);
    source.iChar = m_Source.iChar;
    source.iType = W8_TARGET_SOURCE_CHARACTER;
    source.point = position;
    source.aim_resolved = true;
    ResetCombatSlot(&target);
    target.iType = W8_TARGET_KIND_PLACE;
    target.point = position;
    CastSpellFromSource(W8_SPELL_ROCKET_BLAST, &source, &target, 1, 0, 0, false, 0, 0, 0, 0);
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

    if (!gXStatus.fCombatMode) {
        return;
    }
    swprintf(text, L"%s ", gppStringList[0x1bf]);
    target_start = wcslen(text);
    if (combat_slot.iType == W8_TARGET_KIND_MONSTER) {
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x701, MISSILE_CPP, combat_slot.iMonsterID, true);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        wcscat(text, GetMonsterName(monster_info, 0, 0));
    } else {
        wcscat(text, g_status.buffers.Char[combat_slot.iChar].name);
    }
    target_stop = wcslen(text);
    source_color = GetSourceNoticeColor(&m_Source);
    target_color = GetTargetNoticeColor(&m_Source, &combat_slot);
    wcscat(text, L" ");
    wcscat(text, gppStringList[0x1c0]);
    ShowNotice(source_color, text);
    if (target_color != source_color) {
        HighlightTextBoxRange(target_color, target_start, target_stop, -1);
    }
}

// FUNCTION: WIZ8 0x004a4c20
void W8Missile::EnterImpactCycle()
{
    if (IsCycleSupported(1)) {
        if (GetAnimationState(6) != 1) {
            W8MissileRep* representation = m_pRep;
            srVector3T<float> position = representation->location;
            representation->pending_cycle = 1;
            representation->pending_behaviour = W8_ANIMATION_PLAY_ONCE;
            impacting = true;
            if (explode_ground) {
                representation->location.y = SettlePositionToGround(&position, 0);
            }
        }
    } else {
        flight_done = true;
        if (missile_table_index == 0x23 &&
            (g_combat_state == 0 ||
             g_combat_state->missile_hit_result != W8_MISSILE_HIT_DEFLECTED)) {
            DetonateMissileSpell();
        }
        if (g_missile_table[missile_table_index].spell_missile) {
            AbsorbMissileDamage(this);
        }
    }
}

// FUNCTION: WIZ8 0x004a4720
bool W8Missile::OnCollision(W8Navigator* other)
{
    char hit_result;
    unsigned char deflect_chance;

    if (g_startup_world == other) {
        if (TargetSourceIsCharacter(&m_Source, 0)) {
            goto miss;
        }
        if (combat_slot.iType != W8_TARGET_KIND_PARTY &&
            combat_slot.iType != W8_TARGET_KIND_CHARACTER) {
            if (g_missile_table[missile_table_index].spell_missile) {
                goto miss;
            }
            if (combat_slot.iType != W8_TARGET_KIND_NONE) {
                retargeted = true;
            }
            combat_slot.iType = W8_TARGET_KIND_CHARACTER;
            combat_slot.iChar = GetRandomCharacter(1, 1, -1, -1);
            combat_slot.iMonsterID = -1;
            if (gXStatus.fCombatMode) {
                AnnounceCollisionTarget();
            }
        }
    } else {
        if (other->movement.location_id <= 0) {
            goto miss;
        }
        W8Monster* monster = static_cast<W8Monster*>(other);
        int location_id = monster->location_id;
        unsigned int monster_list_index =
            MonsterGetIndexByLocationID(0x636, MISSILE_CPP, location_id, true);
        W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
        if (TargetSourceIsMonster(&m_Source, 0) && m_Source.iMonsterID == location_id) {
            goto miss;
        }
        if (monster_info->hp_current == 0) {
            goto miss;
        }
        if (combat_slot.iType != W8_TARGET_KIND_MONSTER || combat_slot.iMonsterID != location_id) {
            if (g_missile_table[missile_table_index].spell_missile) {
                goto miss;
            }
            if (combat_slot.iType != W8_TARGET_KIND_NONE) {
                retargeted = true;
            }
            combat_slot.iType = W8_TARGET_KIND_MONSTER;
            combat_slot.iChar = -1;
            combat_slot.iMonsterID = location_id;
            if (gXStatus.fCombatMode) {
                AnnounceCollisionTarget();
            }
        }
    }

    hit_result = W8_MISSILE_HIT;
    if (!g_missile_table[missile_table_index].spell_missile) {
        if (combat_slot.iType == W8_TARGET_KIND_CHARACTER) {
            deflect_chance = g_status.buffers.Char[combat_slot.iChar].bonus.missile_deflect_chance;
        } else {
            W8MonsterInfo* monster_info =
                MonsterInfoFromID(0x676, MISSILE_CPP, combat_slot.iMonsterID, true);
            deflect_chance = monster_info->modifiers.missile_deflect_chance;
        }
        if (deflect_chance > 0 && Random(100) + 1 <= deflect_chance) {
            hit_result = W8_MISSILE_HIT_DEFLECTED;
        }
    }

    if (missile_table_index == 0x23) {
        g_combat_state->missile_hit_result = hit_result;
    } else if (g_combat_state != 0 && g_combat_state->engaged_missile != 0) {
        g_combat_state->missile_hit_result = hit_result;
        g_combat_state->TargetHit = combat_slot;
    } else if (g_missile_table[missile_table_index].spell_missile) {
        ResolveSpellMissileHit(this);
    } else {
        ResolveMissileHit(this, hit_result == W8_MISSILE_HIT_DEFLECTED);
    }
    EnterImpactCycle();
    active = false;
    return true;

miss:
    return false;
}

/* srMatrix3T<float>::RotateAboutX emitted for this TU; the primary template
   lives in srMath.h. */
