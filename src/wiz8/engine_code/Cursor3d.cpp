#include "wiz8/engine_code/Camera.h"
#include "wiz8/engine_code/Cursor3d.h"
#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/cursor.h"
#include "wiz8/regions.h"
#include "wiz8/layouts/game_status.h"
#include "soundman.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/sr_api.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/xstatus.h"
#include "wiz8/float_constants.h"
#include "surrender/srNode.h"
#include "surrender/srMaterial.h"
#include "surrender/srShader.h"
#include "surrender/srTexture.h"
#include "wiz8/engine_code/3dapi.h"

#include <math.h>
#include <stdlib.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/materials.h"

#define CURSOR3D_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Cursor3d.cpp"

/* Engine Code\Cursor3d.cpp. The cursor state, its visibility query and the
   range reset the update path calls. */

// GLOBAL: WIZ8 0x0065ba8c
W8WorldCursorState* gp3DCursor;

// GLOBAL: WIZ8 0x0065ba94
srNode* g_cursor_value_0065ba94;

/* The cursor scene node relocated to position_28 on each move. */
// GLOBAL: WIZ8 0x0065ba90
srNode* g_cursor_node_0065ba90;

/* Set when the cursor is opened while a shift key is held; while set the
   update keeps the latched dragged monster instead of re-picking. */
// GLOBAL: WIZ8 0x0065ba98
unsigned char g_cursor_pick_latch_0065ba98;

/* 0x60ab44: the saved world-cursor monster group id; -1 until a cursor is
   torn down. Seeds and restores monster_group_id_4c. */
// GLOBAL: WIZ8 0x0060ab44
int g_cursor_saved_group_id_60ab44 = -1;

// GLOBAL: WIZ8 0x0060ab48
float g_float_60ab48 = 4000.0f;

// GLOBAL: WIZ8 0x005ebc88
float g_float_005ebc88 = 10.0f;

// GLOBAL: WIZ8 0x005ecb08
const float g_float_005ecb08 = 750.0f;

// GLOBAL: WIZ8 0x005ecb20
const float g_float_005ecb20 = -10000000.0f;

// GLOBAL: WIZ8 0x005ec8d8
const double g_double_005ec8d8 = 15.625;

// GLOBAL: WIZ8 0x005ecb18
const double g_double_005ecb18 = 0.97;

// GLOBAL: WIZ8 0x005ecb10
const float g_float_005ecb10 = 25.0f;

// GLOBAL: WIZ8 0x005ecb0c
const float g_float_005ecb0c = 83.333335876464844f;

/* Build the world cursor on demand: allocate and clear the state block, load
   the 3DCursor monster cycle into it, create its tracking light and seed the
   probe box from the monster's animation bounds. The particle block is gated
   on particle_04 which is never set here - it stays dormant until the cursor
   gains a particle. */
// FUNCTION: WIZ8 0x00490210
void InitializeWorldCursor00490210(void)
{
    W8GrCycleLoadContext context;
    srVector3T<float> position;
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    srVector3T<float> camera_position;
    srVector4T<float> colour;
    srShader shader;
    srMaterial* material;

    DisableCursorScene00428010();
    SetMouseCursorHotspot(0, 0);
    if (gp3DCursor == 0) {
        gp3DCursor = static_cast<W8WorldCursorState*>(malloc(0xe0));
        if (gp3DCursor != 0) {
            memset(gp3DCursor, 0, 0xe0);
            gp3DCursor->monster_00 = 0;
            gp3DCursor->unknown_08 = 1;
            gp3DCursor->group_bind_pending_09 = 0;
            gp3DCursor->input_delta_0c.x = 0;
            gp3DCursor->input_delta_0c.y = 0;
            gp3DCursor->input_delta_0c.z = 0;
            gp3DCursor->light_24 = 0;
            gp3DCursor->enabled_40 = 1;
            gp3DCursor->track_ground_41 = 1;
            gp3DCursor->range_44 = 50000.0f;
            gp3DCursor->left_held_48 = 0;
            gp3DCursor->monster_group_id_4c = g_cursor_saved_group_id_60ab44;
            gp3DCursor->detached_50 = 0;
            gp3DCursor->march_enabled_51 = 1;
            gp3DCursor->footprint_mode_c0 = 0;
            gp3DCursor->dragged_info_dc = 0;
            context.directory_08 = "Data\\Monsters";
            context.world_00 = g_world;
            LoadMonsterCycle004C5910(&context, "3DCursor", &gp3DCursor->monster_00, -1, 1);
            MonsterSetCycle(gp3DCursor->monster_00, 0);
            gp3DCursor->last_published_34.Set(-100000000.0f, -100000000.0f, -100000000.0f);
            gp3DCursor->position_28.Set(0.0f, 0.0f, 0.0f);
            gp3DCursor->offset_18.Set(0.0f, 0.0f, 0.0f);
            gp3DCursor->monster_00->flag_215 = 1;
            WarpSystemCursor(0x140, 0xf0);
            gp3DCursor->input_delta_0c.x = 0;
            gp3DCursor->input_delta_0c.y = 0;
            gp3DCursor->input_delta_0c.z = 0;
            position = gp3DCursor->position_28;
            MonsterSetAdjustedPosition004C5F00(gp3DCursor->monster_00, &position);
            PLAdoptAppend(g_world->plsMonsters, gp3DCursor->monster_00);
            UpdateCycleRepresentation004C59B0(gp3DCursor->monster_00, g_world);
            MonsterSetStateA0(gp3DCursor->monster_00, 0);
            gp3DCursor->light_24 = CreateWorldLight0046E140(g_world, "3D Cursor Light");
            gp3DCursor->light_24->intensity_1d0 = 1.0f;
            ConfigureWorldLight0046E300(gp3DCursor->light_24, 2500.0f);
            gp3DCursor->light_24->ambient_198.Set(0.0f, 0.0f, 0.0f);
            gp3DCursor->light_24->diffuse_1a4.Set(1.0f, 1.0f, 1.0f);
            gp3DCursor->light_24->specular_1b0.Set(0.0f, 0.0f, 0.0f);
            gp3DCursor->light_24->setLocation(0.0, 1000.0, 0.0);
            if (gp3DCursor->particle_04 != 0) {
                material = SR_NEW(srMaterial);
                colour.Set(0.0f, 0.0f, 0.0f, 1.0f);
                material->setEmissive(colour);
                material->setDiffuse(colour);
                gp3DCursor->particle_04->SetRetainedObject0049ACA0(material);
                gp3DCursor->particle_04->SetTexture0049AB00(
                    LoadTexture004B95D0("Data\\Monsters\\Bitmaps\\", "particle.tga", 1));
                shader.value = 0x100c433;
                gp3DCursor->particle_04->SetRenderFlags004925A0(shader);
                gp3DCursor->particle_04->rotateX(-1.5707963);
                gp3DCursor->particle_04->particle_value_140 = 100.0;
                gp3DCursor->particle_04->emission_interval_1c8 = 300;
                gp3DCursor->particle_04->acceleration_1f4.Set(0.0f, -1000.0f, 0.0f);
                gp3DCursor->particle_04->has_acceleration_1a8 = 1;
                gp3DCursor->particle_04->initial_speed_210 = 500.0f;
                gp3DCursor->particle_04->placement_mode_1bc = 2;
                gp3DCursor->particle_04->emission_mode_1b0 = 1;
                gp3DCursor->particle_04->bounds_mode_1a4 = 0;
                gp3DCursor->particle_04->expiry_mode_1ac = 0;
                gp3DCursor->particle_04->lifetime_ms_1cc = 6000;
                gp3DCursor->particle_04->cone_yaw_208 = 1.5707963f;
                gp3DCursor->particle_04->cone_pitch_20c = 1.5707963f;
                gp3DCursor->particle_04->speed_min_214 = 500.0f;
                gp3DCursor->particle_04->speed_max_218 = 1000.0f;
                gp3DCursor->particle_04->SetFlutter0049AD10(2);
                gp3DCursor->particle_04->flutter_amplitude_200 = 50.0f;
                gp3DCursor->particle_04->flutter_period_204 = 1000;
            }
            ApplyWorldCursorInput00490C60();
            if (gp3DCursor != 0) {
                GetCameraPosition(&camera_position);
                if (gp3DCursor->detached_50 == 0) {
                    gp3DCursor->offset_18.x += camera_position.x;
                    gp3DCursor->offset_18.y += camera_position.y;
                    gp3DCursor->offset_18.z += camera_position.z;
                }
                gp3DCursor->detached_50 = 1;
            }
            ClearCombatSelection();
            gp3DCursor->monster_00->GetAnimationBounds(&minimum, &maximum);
            gp3DCursor->probe_center_54.Set((maximum.x + minimum.x) * g_double_005ebe80,
                                            (minimum.y + maximum.y) * g_double_005ebe80,
                                            (minimum.z + maximum.z) * g_double_005ebe80);
            gp3DCursor->probe_offsets_60[0] = maximum;
            gp3DCursor->probe_offsets_60[1].x = minimum.x;
            gp3DCursor->probe_offsets_60[1].y = maximum.y;
            gp3DCursor->probe_offsets_60[1].z = maximum.z;
            gp3DCursor->probe_offsets_60[2].x = minimum.x;
            gp3DCursor->probe_offsets_60[2].y = maximum.y;
            gp3DCursor->probe_offsets_60[2].z = minimum.z;
            gp3DCursor->probe_offsets_60[3].x = maximum.x;
            gp3DCursor->probe_offsets_60[3].y = maximum.y;
            gp3DCursor->probe_offsets_60[3].z = minimum.z;
            gp3DCursor->probe_offsets_60[4].x = maximum.x;
            gp3DCursor->probe_offsets_60[4].y = minimum.y;
            gp3DCursor->probe_offsets_60[4].z = maximum.z;
            gp3DCursor->probe_offsets_60[5].x = minimum.x;
            gp3DCursor->probe_offsets_60[5].y = minimum.y;
            gp3DCursor->probe_offsets_60[5].z = maximum.z;
            gp3DCursor->probe_offsets_60[6] = minimum;
            gp3DCursor->probe_offsets_60[7].x = maximum.x;
            gp3DCursor->probe_offsets_60[7].y = minimum.y;
            gp3DCursor->probe_offsets_60[7].z = minimum.z;
            UpdateWorldCursorPlacement00491EC0();
            if (g_flag_689b32 != 0 &&
                (g_flag_006f0530 != 0 || g_monster_combat_timer_enabled_006f0531 != 0)) {
                g_cursor_pick_latch_0065ba98 = 1;
            }
        }
    }
}

/* Tear the world cursor down completely: detach and delete its monster,
   release the particle, the tracked light and the carried value, then publish
   the retirement through the UI state. The shared camera-distance slot returns
   to the cursor's own last value and the cursor's vertical position is
   flattened before that distance is taken. */
// FUNCTION: WIZ8 0x004909C0
void ReleaseWorldCursor004909C0(void)
{
    W8WorldCursorState* cursor = gp3DCursor;
    srVector3T<float> camera;
    srVector3T<float> delta;

    if (cursor == 0) {
        return;
    }
    GetCameraPosition(&camera);
    camera.y = 0.0f;
    cursor->position_28.y = 0.0f;
    delta = cursor->position_28 - camera;
    g_float_60ab48 = delta.Length();

    PListRemove(g_world->plsMonsters, cursor->monster_00);
    DetachMonsterRepresentation(cursor->monster_00, g_world);
    DeleteMonster004C5860(cursor->monster_00);
    if (cursor->particle_04 != 0) {
        cursor->particle_04->release();
    }
    cursor->particle_04 = 0;
    if (g_cursor_value_0065ba94 != 0) {
        g_cursor_value_0065ba94->release();
        g_cursor_value_0065ba94 = 0;
    }
    if (cursor->light_24 != 0) {
        WorldRemoveLight(g_world, cursor->light_24);
        cursor->light_24 = 0;
    }
    cursor->group_bind_pending_09 = 0;
    g_cursor_saved_group_id_60ab44 = cursor->monster_group_id_4c;
    EnableCursorScene00428020();
    RequestRefreshPartyState();
    ClearTargetMarker();
    free(cursor);
    gp3DCursor = 0;
}

/* The tracked cursor position, or the origin while there is no cursor. */
// FUNCTION: WIZ8 0x00490BF0
void GetWorldCursorPosition00490BF0(srVector3T<float>* position)
{
    if (gp3DCursor != 0) {
        *position = gp3DCursor->position_28;
    } else {
        position->SetZero();
    }
}

/* Show the world cursor: reattach its monster to the world lists, mark it
   visible, reactivate its particle, then park the system cursor and clear the
   combat selection. */
// FUNCTION: WIZ8 0x00490B10
void ShowWorldCursor00490B10(void)
{
    if (gp3DCursor != 0 && gp3DCursor->enabled_40 == 0) {
        UpdateMonster(gp3DCursor->monster_00);
        UpdateCycleRepresentation004C59B0(gp3DCursor->monster_00, g_world);
        PLAdoptAppend(g_world->plsMonsters, gp3DCursor->monster_00);
        gp3DCursor->enabled_40 = 1;
        if (gp3DCursor->particle_04 != 0) {
            gp3DCursor->particle_04->SetActive(1);
        }
        SetMouseCursorHotspot(0, 0);
        DisableCursorScene00428010();
        ClearCombatSelection();
    }
}

/* Hide the world cursor: detach its monster from the world lists, clear the
   visible flag, deactivate its particle, then refresh the party state. */
// FUNCTION: WIZ8 0x00490B90
void HideWorldCursor00490B90(void)
{
    W8World* world;
    W8Monster* monster;
    W8WorldCursorState* cursor = gp3DCursor;

    if (cursor == 0 || cursor->enabled_40 == 0) {
        return;
    }
    monster = cursor->monster_00;
    world = g_world;
    PListRemove(world->plsMonsters, monster);
    DetachMonsterRepresentation(monster, world);
    cursor->enabled_40 = 0;
    if (cursor->particle_04 != 0) {
        cursor->particle_04->SetActive(0);
    }
    EnableCursorScene00428020();
    RequestRefreshPartyState();
}

// FUNCTION: WIZ8 0x00490C20
void GetWorldCursorAnchor00490C20(srVector3T<float>* position)
{
    if (gp3DCursor != 0) {
        *position = gp3DCursor->position_28;
    } else {
        position->SetZero();
    }
}

/* Consume the accumulated input deltas: scale them into a world-space step,
   latch the shift-dragged monster, then move the cursor. While the cursor is
   camera-locked the input feeds offset_18 and position_28 is rebuilt from the
   dynamic scene node (optionally settled onto the terrain); while detached the
   yaw-rotated delta moves position_28 directly inside the range and
   poster-distance clamps, optionally marched to a ground/sight target. A
   changed position is republished to the cursor monster, the cursor nodes and
   the dragged monster. */
// FUNCTION: WIZ8 0x00490C60
void ApplyWorldCursorInput00490C60(void)
{
    unsigned char hit;
    srMatrix3T<float> rotation;
    srVector3T<double> node_location;
    srVector3T<float> camera;
    srVector3T<float> delta;
    srVector3T<float> lifted;
    srVector3T<float> clamped;
    float saved_y = gp3DCursor->position_28.y;

    if (gp3DCursor == 0) {
        srAssertFail("gp3DCursor", CURSOR3D_CPP, 0x188, 0);
    }
    if (gp3DCursor->enabled_40 == 0) {
        srAssertFail("gp3DCursor->fEnabled", CURSOR3D_CPP, 0x189, 0);
    }
    delta.x = gp3DCursor->input_delta_0c.x * g_float_005ebc88;
    delta.y = gp3DCursor->input_delta_0c.y * g_float_005ebc88;
    delta.z = gp3DCursor->input_delta_0c.z * g_float_005ebc88;
    gp3DCursor->input_delta_0c.x = 0;
    gp3DCursor->input_delta_0c.y = 0;
    gp3DCursor->input_delta_0c.z = 0;
    if (g_flag_689b32 != 0) {
        if (gfKeyState[0x10] == 0 && gfKeyState[0x11] == 0) {
            if (g_cursor_pick_latch_0065ba98 != 0) {
                g_cursor_pick_latch_0065ba98 = 0;
            }
            if (gp3DCursor->dragged_info_dc != 0) {
                gp3DCursor->dragged_info_dc = 0;
            }
        } else if (g_cursor_pick_latch_0065ba98 == 0 && gp3DCursor->dragged_info_dc == 0) {
            gp3DCursor->dragged_info_dc = FindNearestMonsterInfo(&gp3DCursor->position_28, 2500.0);
        }
    }
    if (gp3DCursor->detached_50 == 0) {
        lifted.x = delta.x;
        lifted.y = delta.y + g_float_005ebc64;
        lifted.z = delta.z;
        if (gp3DCursor->range_44 > g_float_005ebb34 && gp3DCursor->range_44 < lifted.Length()) {
            delta.SetLength(gp3DCursor->range_44);
        }
        g_world->dynamic_scene->getRotation(rotation);
        gp3DCursor->offset_18 += delta;
        gp3DCursor->position_28 = rotation.Transform(gp3DCursor->offset_18);
        node_location = g_world->dynamic_scene->getLocation();
        gp3DCursor->position_28.x += node_location.x;
        gp3DCursor->position_28.y += node_location.y;
        gp3DCursor->position_28.z += node_location.z;
        if (gp3DCursor->track_ground_41 != 0) {
            if (gp3DCursor->position_28.y < saved_y) {
                gp3DCursor->position_28.y = saved_y;
            }
            gp3DCursor->position_28.y =
                g_octree_6598a4->SettleToGround(&gp3DCursor->position_28, &hit, 1, 500.0f);
        }
    } else {
        GetCameraPosition(&camera);
        camera.y -= g_default_world_height_00603ac8;
        rotation.SetIdentity();
        if (g_gd_camera_65a0f8->m_yaw != 0.0) {
            rotation.RotateAboutY(sin(g_gd_camera_65a0f8->m_yaw), cos(g_gd_camera_65a0f8->m_yaw));
        }
        delta = rotation.Transform(delta);
        delta += gp3DCursor->position_28;
        if (gp3DCursor->range_44 > g_float_005ebb34 &&
            gp3DCursor->range_44 < (camera - delta).Length()) {
            clamped = delta - camera;
            clamped.SetLength(gp3DCursor->range_44);
            delta = camera + clamped;
        }
        if ((camera - delta).Length() < g_monster_poster_max_distance_005ec3d8) {
            clamped = delta - camera;
            clamped.SetLength(g_monster_poster_max_distance_005ec3d8);
            delta = camera + clamped;
        }
        if (gp3DCursor->march_enabled_51 != 0) {
            MarchWorldCursorTarget004919E0(&delta);
        }
        gp3DCursor->position_28 = delta;
        gp3DCursor->offset_18 = delta;
    }
    if (gp3DCursor->last_published_34.x != gp3DCursor->position_28.x ||
        gp3DCursor->last_published_34.y != gp3DCursor->position_28.y ||
        gp3DCursor->last_published_34.z != gp3DCursor->position_28.z) {
        lifted = gp3DCursor->position_28;
        MonsterSetAdjustedPosition004C5F00(gp3DCursor->monster_00, &lifted);
        if (g_cursor_node_0065ba90 != 0) {
            node_location.SetFromFloat(&gp3DCursor->position_28);
            g_cursor_node_0065ba90->setLocation(node_location);
        }
        if (g_cursor_value_0065ba94 != 0) {
            node_location.SetFromFloat(&gp3DCursor->position_28);
            g_cursor_value_0065ba94->setLocation(node_location);
        }
        if (gp3DCursor->particle_04 != 0) {
            node_location.SetFromFloat(&gp3DCursor->position_28);
            gp3DCursor->particle_04->setLocation(node_location);
        }
        if (gp3DCursor->light_24 != 0) {
            node_location.SetFromFloat(&gp3DCursor->position_28);
            gp3DCursor->light_24->setLocation(node_location);
        }
        if (gp3DCursor->dragged_info_dc != 0) {
            gp3DCursor->dragged_info_dc->monster->SetPositionInternal00453590(
                &gp3DCursor->position_28);
            g_octree_6598a4->UpdateMonsterLocation(gp3DCursor->dragged_info_dc->location_id,
                                                   &gp3DCursor->position_28);
            if (gfKeyState[0x10] != 0) {
                MonsterForwardReferencePosition(gp3DCursor->dragged_info_dc->monster, 1);
            }
        }
        gp3DCursor->last_published_34 = gp3DCursor->position_28;
    }
    g_octree_6598a4->UpdatePathVisualization();
}

// FUNCTION: WIZ8 0x004914C0
bool IsWorldCursorVisible(void)
{
    return gp3DCursor != 0 && gp3DCursor->enabled_40 != 0;
}

/* Bind the cursor monster to its monster group once flagged: reset the
   particle timing, find the group recorded on the cursor (or the first group
   in the list as a fallback), walk up to the leader group, arm the located
   monster with the Test.msf script and move the group leader to the cursor
   point. */
// FUNCTION: WIZ8 0x004914E0
void BindCursorMonsterToGroup004914E0(void)
{
    srVector3T<float> position;
    W8MonsterGroup* monster_group;
    W8MonsterInfo* monster_info;
    unsigned int index;

    if (gp3DCursor->group_bind_pending_09 != 0 && gp3DCursor->light_24 != 0) {
        gp3DCursor->group_bind_pending_09 = 0;
        gp3DCursor->input_delta_0c.z = gp3DCursor->input_delta_0c.z + 1;
        if (gp3DCursor->particle_04 != 0) {
            gp3DCursor->particle_04->speed_min_214 = 1000.0f;
            gp3DCursor->particle_04->speed_max_218 = 2000.0f;
            gp3DCursor->particle_04->emission_interval_1c8 = 300;
        }
        if (gp3DCursor == 0) {
            position.SetZero();
        } else {
            position = gp3DCursor->position_28;
        }
        index = GetMonsterGroupIndexByID(0x237, CURSOR3D_CPP, gp3DCursor->monster_group_id_4c, 0);
        if (index == 0xffffffff) {
            index = PLLength(gXStatus.plsMonsterGroupList);
            if (index == 0) {
                return;
            }
            index = 0;
        }
        monster_group = GetMonsterGroupByListIndex(index);
        gp3DCursor->monster_group_id_4c = monster_group->group_id;
        if (monster_group != 0) {
            if (monster_group->leader_group_id != 0) {
                index = GetMonsterGroupIndexByID(0x245, CURSOR3D_CPP,
                                                 monster_group->leader_group_id, 1);
                monster_group = GetMonsterGroupByListIndex(index);
            }
            index =
                MonsterGetIndexByLocationID(0x247, CURSOR3D_CPP, monster_group->leader_id_9f, 1);
            monster_info = MonsterGetScriptPartByLocationIndex(index);
            if (monster_info != 0 && monster_info->monster != 0 &&
                monster_info->monster->SetScript004C7F10("Test.msf", 1) == 0) {
                ApplyToMonsterGroupLeader(monster_group, &position, 1);
            }
        }
    }
}

/* Reset the cursor range to its default 4000 units. The cursor release path
   (ReleaseWorldCursor004909C0) writes the flattened camera distance to the
   same slot before the cursor is freed. */
// FUNCTION: WIZ8 0x00492530
void SetFloat60AB48(void)
{
    g_float_60ab48 = 4000.0f;
}

/* 0x005EC260: hard ceiling for SetWorldCursorRange00491650. */
// GLOBAL: WIZ8 0x005ec260
float g_float_005ec260 = 50000.0f;

/* Install an action-range distance into the live cursor: subtract
   g_float_005ebcdc / distance * world_scale, clamp to 50000, write range_44,
   and force last_published_34 to the -1e8 republish sentinel. */
// FUNCTION: WIZ8 0x00491650
void SetWorldCursorRange00491650(float distance)
{
    if (gp3DCursor != 0) {
        distance = distance - (g_float_005ebcdc / distance) * g_world_scale_005ebc40;
        if (distance >= g_float_005ec260) {
            distance = g_float_005ec260;
        }
        gp3DCursor->range_44 = distance;
        gp3DCursor->last_published_34 = -100000000.0f;
    }
}

/* Store the selected monster group id on the world cursor. */
// FUNCTION: WIZ8 0x004916a0
void SetWorldCursorGroupId004916A0(int group_id)
{
    if (gp3DCursor != 0) {
        gp3DCursor->monster_group_id_4c = group_id;
    }
}

/* Toggle the 3D world cursor: release it if one exists, or initialize one
   if none does. Both branches tail-call into the respective functions. */
// FUNCTION: WIZ8 0x00490af0
void ToggleWorldCursor(void)
{
    if (gp3DCursor != 0) {
        ReleaseWorldCursor004909C0();
    } else {
        InitializeWorldCursor00490210();
    }
}

/* March the cursor target from the stored position toward the requested
   point in 15.625-unit steps. Each step ground-settles the four lower probe
   offsets scaled inward by 0.97; with ground tracking on, the end point
   lifts to the best settled height (an upper probe wins, otherwise all four
   lower probes must have settled). The eight-offset sight fan from the
   probe-box center then guards the step: the first blocked ray writes the
   last clear end point back through `target` and returns 1. Reaching the
   requested point writes it back and re-arms ground tracking when the final
   height ends within 83.33 of the lowest settled probe. */
/* Per-frame world-cursor update from MainGameScreenFrame: recenter the mouse
   onto the view, fold the movement into the accumulated input deltas, run the
   cursor move, then handle the left-button click - a release while the place
   mode is active line-of-sights and box-tests the resolved target, aims the
   selected character and resets the cursor, or plays the error beep. A moved
   cursor points the camera at it while detached and refreshes the target
   marker in place mode. */
// FUNCTION: WIZ8 0x004916C0
void UpdateWorldCursor004916C0(void)
{
    POINT cursor_point;
    srVector3T<float> camera;
    srVector3T<float> old_position;
    srVector3T<float> position;
    srVector3T<float> resolved;
    srVector3T<float> box_min;
    srVector3T<float> box_max;

    if (gp3DCursor == 0 || gp3DCursor->enabled_40 == 0) {
        return;
    }
    old_position = gp3DCursor->position_28;
    SyncSystemCursor();
    SGPMouseGetPos(&cursor_point);
    if (gfRightButtonState != 0) {
        if (gp3DCursor->group_bind_pending_09 == 0 && gp3DCursor->light_24 != 0) {
            gp3DCursor->group_bind_pending_09 = 1;
            gp3DCursor->input_delta_0c.z = gp3DCursor->input_delta_0c.z + 1;
            if (gp3DCursor->particle_04 != 0) {
                gp3DCursor->particle_04->speed_min_214 = 3000.0f;
                gp3DCursor->particle_04->speed_max_218 = 6000.0f;
                gp3DCursor->particle_04->emission_interval_1c8 = 0x14;
            }
        }
    } else {
        BindCursorMonsterToGroup004914E0();
    }
    if (gfRightButtonState != 0) {
        if (gp3DCursor->detached_50 != 0) {
            gp3DCursor->track_ground_41 = 0;
        }
        gp3DCursor->input_delta_0c.y = gp3DCursor->input_delta_0c.y + (0xf0 - cursor_point.y);
    } else {
        gp3DCursor->input_delta_0c.x = gp3DCursor->input_delta_0c.x + (cursor_point.x - 0x140);
        gp3DCursor->input_delta_0c.z = gp3DCursor->input_delta_0c.z + (0xf0 - cursor_point.y);
    }
    WarpSystemCursor(0x140, 0xf0);
    ApplyWorldCursorInput00490C60();
    if (gp3DCursor != 0) {
        position = gp3DCursor->position_28;
    } else {
        position.Set(0.0, 0.0, 0.0);
    }
    if (gfLeftButtonState == 0) {
        if (gp3DCursor->left_held_48 != 0 && gXStatus.iTargetingMode == 3) {
            GetCameraPosition(&camera);
            if (ResolveWorldCursorTarget004921E0(&resolved) != 0 &&
                g_octree_6598a4->TraceLineOfSight(&camera, &resolved, 1, -3, -3, 1, 0) == 0) {
                box_min = resolved + gp3DCursor->offset_c4;
                box_max = resolved + gp3DCursor->offset_d0;
                if (gp3DCursor->footprint_mode_c0 == 0 ||
                    g_octree_6598a4->TestBoxOccupied(&box_min, &box_max) == 0) {
                    AimAtPlace(g_status_685170.selected_character);
                    if (gp3DCursor == 0) {
                        InitializeWorldCursor00490210();
                    } else {
                        ReleaseWorldCursor004909C0();
                    }
                    return;
                }
            }
            // c-style-cast-ok: SGP SoundPlay spells filenames UINT8* - the
            // documented historical ABI boundary.
            SoundPlay((STR) "Data\\Sound\\Misc\\ErrorBeep.wav", 0);
        }
        gp3DCursor->left_held_48 = 0;
    } else {
        gp3DCursor->left_held_48 = 1;
    }
    if (old_position.x != position.x || old_position.y != position.y ||
        old_position.z != position.z) {
        if (gp3DCursor->detached_50 != 0) {
            position.y += g_float_005ecb08;
            PointCameraAtTarget(&position, 1, 0);
        }
        if (gXStatus.iTargetingMode == 3) {
            RefreshTargetMarker();
        }
    }
}

// FUNCTION: WIZ8 0x004919E0
char MarchWorldCursorTarget004919E0(srVector3T<float>* target)
{
    srVector3T<float> trace_from;
    srVector3T<float> origin;
    srVector3T<float> last_valid;
    srVector3T<float> step_pos;
    srVector3T<float> end_pos;
    srVector3T<float> dir;
    srVector3T<float> scaled;
    srVector3T<float> probe;
    unsigned char hit;
    double dist;
    double step;
    float lower_best = -1e+10f;
    float upper_best;
    float ground;
    int lower_count;
    bool upper_found;
    int i;

    trace_from = *target + gp3DCursor->probe_center_54;
    origin = gp3DCursor->position_28;
    dist = (*target - origin).Length();
    if (dist == g_zero_005ebb40) {
        return 0;
    }
    last_valid = gp3DCursor->position_28;
    step = 0.0;
    if (g_zero_005ebb40 < dist) {
        do {
            lower_count = 0;
            step += g_double_005ec8d8;
            upper_found = false;
            upper_best = -1e+10f;
            lower_best = -1e+10f;
            if (dist < step) {
                step = dist;
            }
            dir = *target - origin;
            scaled = dir;
            scaled.SetLength(step);
            step_pos = origin + scaled;
            end_pos = step_pos;
            for (i = 4; i < 8; i++) {
                probe = gp3DCursor->probe_offsets_60[i];
                probe *= g_double_005ecb18;
                probe += step_pos;
                ground = g_octree_6598a4->SettleToGround(&probe, &hit, 1, 500.0f);
                if (ground <= step_pos.y) {
                    if (ground < step_pos.y) {
                        lower_count++;
                        if (lower_best < ground) {
                            lower_best = ground;
                        }
                    }
                } else if (ground - step_pos.y < g_float_005ebc64 && upper_best < ground) {
                    upper_found = true;
                    upper_best = ground;
                }
            }
            if (gp3DCursor->track_ground_41 != 0) {
                if (upper_found) {
                    end_pos.y = upper_best + g_float_005ecb10;
                    target->y = end_pos.y;
                    origin.y = end_pos.y;
                } else if (lower_count == 4) {
                    end_pos.y = lower_best + g_float_005ecb10;
                    target->y = end_pos.y;
                    origin.y = end_pos.y;
                }
            }
            trace_from = end_pos + gp3DCursor->probe_center_54;
            for (i = 0; i < 8; i++) {
                probe = end_pos + gp3DCursor->probe_offsets_60[i];
                if (g_octree_6598a4->TraceLineOfSight(&trace_from, &probe, 1, -3, -3, 1, 0) != 0) {
                    *target = last_valid;
                    return 1;
                }
            }
            last_valid = end_pos;
        } while (step < dist);
    }
    *target = last_valid;
    if (target->y - lower_best < g_float_005ecb0c) {
        gp3DCursor->track_ground_41 = 1;
    }
    return 0;
}

/* Update the world cursor's placement: derive a point g_float_60ab48 units
   in front of the yaw-rotated camera, resolve it through the cursor walk and
   store the result as the cursor position and camera-relative offset. */
// FUNCTION: WIZ8 0x00491EC0
void UpdateWorldCursorPlacement00491EC0(void)
{
    srMatrix3T<float> rotation;
    srMatrix3T<float> axis;
    srVector3T<float> camera(0.0f, 1.0f, 0.0f);
    srVector3T<float> forward;
    srVector3T<float> target;
    srVector3T<float> first;
    srVector3T<float> second;
    srVector3T<float> third;
    W8WorldCursorState* cursor;
    float cosine;
    float sine;

    rotation.SetIdentity();
    if (static_cast<double>(g_gd_camera_65a0f8->m_yaw) != g_zero_005ebb40) {
        cosine = static_cast<float>(cos(g_gd_camera_65a0f8->m_yaw));
        sine = static_cast<float>(sin(g_gd_camera_65a0f8->m_yaw));
        first.Set(cosine, 0.0, sine);
        second.Set(0.0, 1.0, 0.0);
        third.Set(-sine, 0.0, cosine);
        axis.SetRows(first, second, third);
        rotation.MultiplyBy(axis);
    }
    GetCameraPosition(&camera);
    camera.y = camera.y - g_default_world_height_00603ac8;
    cursor = gp3DCursor;
    cursor->last_published_34 = -100000000.0f;
    forward.Set(0.0, 0.0, g_float_60ab48);
    target = camera + rotation.Transform(forward);
    cursor->position_28 = camera;
    cursor->track_ground_41 = 1;
    if (MarchWorldCursorTarget004919E0(&target) == 0) {
        cursor->position_28 = target;
        cursor->offset_18 = cursor->position_28;
        if (cursor->detached_50 == 0) {
            cursor->offset_18 -= camera;
        }
        target.y = target.y + g_float_005ecb08;
        PointCameraAtTarget(&target, 1, 0);
    } else {
        cursor->position_28 = target;
    }
}

/* Arm the world cursor's footprint mode and install the two fixed probe
   offsets the target resolver probes in place of the probe box. */
// FUNCTION: WIZ8 0x00492190
void SetWorldCursorExtents00492190(const srVector3T<float>* minimum,
                                   const srVector3T<float>* maximum)
{
    if (gp3DCursor == 0) {
        return;
    }
    gp3DCursor->footprint_mode_c0 = 1;
    gp3DCursor->offset_c4 = *minimum;
    gp3DCursor->offset_d0 = *maximum;
}

/* Resolve the world cursor's target position: start from the cursor's stored
   position, ground-probe its candidate corner offsets and lift the target to
   the best settled height. With the fixed-offset flag the two stored offsets
   and their crossed corner combinations are probed instead, and a settled
   height too far from the input height fails the resolution. */
// FUNCTION: WIZ8 0x004921E0
int ResolveWorldCursorTarget004921E0(srVector3T<float>* position)
{
    W8WorldCursorState* cursor = gp3DCursor;
    srVector3T<float> probe;
    float best = g_float_005ecb20;
    int i;

    if (cursor == 0) {
        position->SetZero();
    } else {
        *position = cursor->position_28;
    }
    if (cursor == 0) {
        return 1;
    }
    if (cursor->footprint_mode_c0 == 0) {
        for (i = 0; i < 4; i++) {
            probe = *position + cursor->probe_offsets_60[i + 4];
            if (best <= g_octree_6598a4->SettleToGround(&probe, 0, 1, 500.0f)) {
                best = g_octree_6598a4->SettleToGround(&probe, 0, 1, 500.0f);
            }
        }
    } else {
        probe = *position + cursor->offset_c4;
        if (best <= g_octree_6598a4->SettleToGround(&probe, 0, 1, 500.0f)) {
            best = g_octree_6598a4->SettleToGround(&probe, 0, 1, 500.0f);
        }
        probe = *position + cursor->offset_d0;
        if (best <= g_octree_6598a4->SettleToGround(&probe, 0, 1, 500.0f)) {
            best = g_octree_6598a4->SettleToGround(&probe, 0, 1, 500.0f);
        }
        probe.x = cursor->offset_c4.x + position->x;
        probe.y = position->y;
        probe.z = cursor->offset_d0.z + position->z;
        if (best <= g_octree_6598a4->SettleToGround(&probe, 0, 1, 500.0f)) {
            best = g_octree_6598a4->SettleToGround(&probe, 0, 1, 500.0f);
        }
        probe.x = cursor->offset_d0.x + position->x;
        probe.y = position->y;
        probe.z = cursor->offset_c4.z + position->z;
        if (best <= g_octree_6598a4->SettleToGround(&probe, 0, 1, 500.0f)) {
            best = g_octree_6598a4->SettleToGround(&probe, 0, 1, 500.0f);
        }
        if (static_cast<float>(g_monster_poster_max_distance_005ec3d8) <=
            fabs(best - position->y)) {
            position->y = best + g_float_005ebc64;
            return 0;
        }
    }
    position->y = best + g_float_005ebc64;
    return 1;
}

/* Forward the resolved cursor position to the caller's vector. */
// FUNCTION: WIZ8 0x00492500
void GetWorldCursorTargetPosition00492500(srVector3T<float>* position)
{
    srVector3T<float> resolved;

    ResolveWorldCursorTarget004921E0(&resolved);
    *position = resolved;
}