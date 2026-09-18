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

#include <math.h>
#include <stdlib.h>
#include "wiz8/engine_code/GameData.h"

#define CURSOR3D_CPP "C:\\Projects\\Wizardry 8\\Engine Code\\Cursor3d.cpp"

/* Engine Code\Cursor3d.cpp. The cursor state, its visibility query and the
   range reset the update path calls. */

// GLOBAL: WIZ8 0x0065ba8c
W8WorldCursorState* g_world_cursor_0065ba8c;

// GLOBAL: WIZ8 0x0065ba94
srNode* g_cursor_value_0065ba94;

/* The cursor scene node relocated to position_28 on each move. */
// GLOBAL: WIZ8 0x0065ba90
srNode* g_cursor_node_0065ba90;

/* Set when the cursor is opened while a shift key is held; while set the
   update keeps the latched dragged monster instead of re-picking. */
// GLOBAL: WIZ8 0x0065ba98
unsigned char g_cursor_pick_latch_0065ba98;

/* 0x60ab44: the world cursor's saved slot value; -1 until a cursor is torn
   down. Only ever copied whole between here and the cursor, so its domain is
   still unknown. */
// GLOBAL: WIZ8 0x0060ab44
int g_cursor_saved_value_60ab44 = -1;

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

/* Tear the world cursor down completely: detach and delete its monster,
   release the particle, the tracked light and the carried value, then publish
   the retirement through the UI state. The shared camera-distance slot returns
   to the cursor's own last value and the cursor's vertical position is
   flattened before that distance is taken. */
// FUNCTION: WIZ8 0x004909C0
void ReleaseWorldCursor004909C0(void)
{
    W8WorldCursorState* cursor = g_world_cursor_0065ba8c;
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
    g_cursor_saved_value_60ab44 = cursor->value_4c;
    SetFlag603C60();
    RequestRefreshPartyState();
    ClearTargetMarker();
    free(cursor);
    g_world_cursor_0065ba8c = 0;
}

/* The tracked cursor position, or the origin while there is no cursor. */
// FUNCTION: WIZ8 0x00490BF0
void GetWorldCursorPosition00490BF0(srVector3T<float>* position)
{
    if (g_world_cursor_0065ba8c != 0) {
        *position = g_world_cursor_0065ba8c->position_28;
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
    if (g_world_cursor_0065ba8c != 0 && g_world_cursor_0065ba8c->enabled_40 == 0) {
        UpdateMonster(g_world_cursor_0065ba8c->monster_00);
        UpdateCycleRepresentation004C59B0(g_world_cursor_0065ba8c->monster_00, g_world);
        PLAdoptAppend(g_world->plsMonsters, g_world_cursor_0065ba8c->monster_00);
        g_world_cursor_0065ba8c->enabled_40 = 1;
        if (g_world_cursor_0065ba8c->particle_04 != 0) {
            g_world_cursor_0065ba8c->particle_04->SetActive(1);
        }
        SetMouseCursorHotspot(0, 0);
        ClearFlag603C60();
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
    W8WorldCursorState* cursor = g_world_cursor_0065ba8c;

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
    SetFlag603C60();
    RequestRefreshPartyState();
}

// FUNCTION: WIZ8 0x00490C20
void GetWorldCursorAnchor00490C20(srVector3T<float>* position)
{
    if (g_world_cursor_0065ba8c != 0) {
        *position = g_world_cursor_0065ba8c->position_28;
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
    float saved_y = g_world_cursor_0065ba8c->position_28.y;

    if (g_world_cursor_0065ba8c == 0) {
        srAssertFail("gp3DCursor", CURSOR3D_CPP, 0x188, 0);
    }
    if (g_world_cursor_0065ba8c->enabled_40 == 0) {
        srAssertFail("gp3DCursor->fEnabled", CURSOR3D_CPP, 0x189, 0);
    }
    delta.x = g_world_cursor_0065ba8c->input_delta_0c[0] * g_float_005ebc88;
    delta.y = g_world_cursor_0065ba8c->input_delta_0c[1] * g_float_005ebc88;
    delta.z = g_world_cursor_0065ba8c->input_delta_0c[2] * g_float_005ebc88;
    g_world_cursor_0065ba8c->input_delta_0c[0] = 0;
    g_world_cursor_0065ba8c->input_delta_0c[1] = 0;
    g_world_cursor_0065ba8c->input_delta_0c[2] = 0;
    if (g_flag_689b32 != 0) {
        if (gfKeyState[0x10] == 0 && gfKeyState[0x11] == 0) {
            if (g_cursor_pick_latch_0065ba98 != 0) {
                g_cursor_pick_latch_0065ba98 = 0;
            }
            if (g_world_cursor_0065ba8c->dragged_info_dc != 0) {
                g_world_cursor_0065ba8c->dragged_info_dc = 0;
            }
        } else if (g_cursor_pick_latch_0065ba98 == 0 &&
                   g_world_cursor_0065ba8c->dragged_info_dc == 0) {
            g_world_cursor_0065ba8c->dragged_info_dc =
                FindNearestMonsterInfo(&g_world_cursor_0065ba8c->position_28, 2500.0);
        }
    }
    if (g_world_cursor_0065ba8c->detached_50 == 0) {
        lifted.x = delta.x;
        lifted.y = delta.y + g_float_005ebc64;
        lifted.z = delta.z;
        if (g_world_cursor_0065ba8c->range_44 > g_float_005ebb34 &&
            g_world_cursor_0065ba8c->range_44 < lifted.Length()) {
            delta.SetLength(g_world_cursor_0065ba8c->range_44);
        }
        g_world->dynamic_scene->getRotation(rotation);
        g_world_cursor_0065ba8c->offset_18 += delta;
        g_world_cursor_0065ba8c->position_28 =
            rotation.Transform(g_world_cursor_0065ba8c->offset_18);
        node_location = g_world->dynamic_scene->getLocation();
        g_world_cursor_0065ba8c->position_28.x += node_location.x;
        g_world_cursor_0065ba8c->position_28.y += node_location.y;
        g_world_cursor_0065ba8c->position_28.z += node_location.z;
        if (g_world_cursor_0065ba8c->track_ground_41 != 0) {
            if (g_world_cursor_0065ba8c->position_28.y < saved_y) {
                g_world_cursor_0065ba8c->position_28.y = saved_y;
            }
            g_world_cursor_0065ba8c->position_28.y = g_octree_6598a4->SettleToGround(
                &g_world_cursor_0065ba8c->position_28, &hit, 1, 500.0f);
        }
    } else {
        GetCameraPosition(&camera);
        camera.y -= g_default_world_height_00603ac8;
        rotation.SetIdentity();
        if (g_gd_camera_65a0f8->m_yaw != 0.0) {
            rotation.RotateAboutY(sin(g_gd_camera_65a0f8->m_yaw), cos(g_gd_camera_65a0f8->m_yaw));
        }
        delta = rotation.Transform(delta);
        delta += g_world_cursor_0065ba8c->position_28;
        if (g_world_cursor_0065ba8c->range_44 > g_float_005ebb34 &&
            g_world_cursor_0065ba8c->range_44 < (camera - delta).Length()) {
            clamped = delta - camera;
            clamped.SetLength(g_world_cursor_0065ba8c->range_44);
            delta = camera + clamped;
        }
        if ((camera - delta).Length() < g_monster_poster_max_distance_005ec3d8) {
            clamped = delta - camera;
            clamped.SetLength(g_monster_poster_max_distance_005ec3d8);
            delta = camera + clamped;
        }
        if (g_world_cursor_0065ba8c->march_enabled_51 != 0) {
            MarchWorldCursorTarget004919E0(&delta);
        }
        g_world_cursor_0065ba8c->position_28 = delta;
        g_world_cursor_0065ba8c->offset_18 = delta;
    }
    if (g_world_cursor_0065ba8c->last_published_34.x != g_world_cursor_0065ba8c->position_28.x ||
        g_world_cursor_0065ba8c->last_published_34.y != g_world_cursor_0065ba8c->position_28.y ||
        g_world_cursor_0065ba8c->last_published_34.z != g_world_cursor_0065ba8c->position_28.z) {
        lifted = g_world_cursor_0065ba8c->position_28;
        MonsterSetAdjustedPosition004C5F00(g_world_cursor_0065ba8c->monster_00, &lifted);
        if (g_cursor_node_0065ba90 != 0) {
            node_location.x = g_world_cursor_0065ba8c->position_28.x;
            node_location.y = g_world_cursor_0065ba8c->position_28.y;
            node_location.z = g_world_cursor_0065ba8c->position_28.z;
            g_cursor_node_0065ba90->setLocation(node_location);
        }
        if (g_cursor_value_0065ba94 != 0) {
            node_location.SetFromFloat(&g_world_cursor_0065ba8c->position_28);
            g_cursor_value_0065ba94->setLocation(node_location);
        }
        if (g_world_cursor_0065ba8c->particle_04 != 0) {
            node_location.SetFromFloat(&g_world_cursor_0065ba8c->position_28);
            g_world_cursor_0065ba8c->particle_04->setLocation(node_location);
        }
        if (g_world_cursor_0065ba8c->light_24 != 0) {
            node_location.SetFromFloat(&g_world_cursor_0065ba8c->position_28);
            g_world_cursor_0065ba8c->light_24->setLocation(node_location);
        }
        if (g_world_cursor_0065ba8c->dragged_info_dc != 0) {
            g_world_cursor_0065ba8c->dragged_info_dc->monster->SetPositionInternal00453590(
                &g_world_cursor_0065ba8c->position_28);
            g_octree_6598a4->UpdateMonsterLocation(
                g_world_cursor_0065ba8c->dragged_info_dc->location_id,
                &g_world_cursor_0065ba8c->position_28);
            if (gfKeyState[0x10] != 0) {
                MonsterForwardReferencePosition(g_world_cursor_0065ba8c->dragged_info_dc->monster,
                                                1);
            }
        }
        g_world_cursor_0065ba8c->last_published_34 = g_world_cursor_0065ba8c->position_28;
    }
    g_octree_6598a4->UpdatePathVisualization();
}

// FUNCTION: WIZ8 0x004914C0
bool IsWorldCursorVisible(void)
{
    return g_world_cursor_0065ba8c != 0 && g_world_cursor_0065ba8c->enabled_40 != 0;
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

    if (g_world_cursor_0065ba8c->group_bind_pending_09 != 0 &&
        g_world_cursor_0065ba8c->light_24 != 0) {
        g_world_cursor_0065ba8c->group_bind_pending_09 = 0;
        g_world_cursor_0065ba8c->input_delta_0c[2] = g_world_cursor_0065ba8c->input_delta_0c[2] + 1;
        if (g_world_cursor_0065ba8c->particle_04 != 0) {
            g_world_cursor_0065ba8c->particle_04->value_214 = 1000.0f;
            g_world_cursor_0065ba8c->particle_04->value_218 = 2000.0f;
            g_world_cursor_0065ba8c->particle_04->value_1c8 = 300;
        }
        if (g_world_cursor_0065ba8c == 0) {
            position.SetZero();
        } else {
            position = g_world_cursor_0065ba8c->position_28;
        }
        index = GetMonsterGroupIndexByID(0x237, CURSOR3D_CPP, g_world_cursor_0065ba8c->value_4c, 0);
        if (index == 0xffffffff) {
            index = PLLength(gXStatus.plsMonsterGroupList);
            if (index == 0) {
                return;
            }
            index = 0;
        }
        monster_group = GetMonsterGroupByListIndex(index);
        g_world_cursor_0065ba8c->value_4c = monster_group->group_id;
        if (monster_group != 0) {
            if (monster_group->leader_group_id != 0) {
                index = GetMonsterGroupIndexByID(0x245, CURSOR3D_CPP,
                                                 monster_group->leader_group_id, 1);
                monster_group = GetMonsterGroupByListIndex(index);
            }
            index = MonsterGetIndexByLocationID(0x247, CURSOR3D_CPP, monster_group->value_9f, 1);
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
    if (g_world_cursor_0065ba8c != 0) {
        distance = distance - (g_float_005ebcdc / distance) * g_world_scale_005ebc40;
        if (distance >= g_float_005ec260) {
            distance = g_float_005ec260;
        }
        g_world_cursor_0065ba8c->range_44 = distance;
        g_world_cursor_0065ba8c->last_published_34 = -100000000.0f;
    }
}

/* Toggle the 3D world cursor: release it if one exists, or initialize one
   if none does. Both branches tail-call into the respective functions. */
// FUNCTION: WIZ8 0x00490af0
void ToggleWorldCursor(void)
{
    if (g_world_cursor_0065ba8c != 0) {
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

    if (g_world_cursor_0065ba8c == 0 || g_world_cursor_0065ba8c->enabled_40 == 0) {
        return;
    }
    old_position = g_world_cursor_0065ba8c->position_28;
    SyncSystemCursor();
    SGPMouseGetPos(&cursor_point);
    if (gfRightButtonState != 0) {
        if (g_world_cursor_0065ba8c->group_bind_pending_09 == 0 &&
            g_world_cursor_0065ba8c->light_24 != 0) {
            g_world_cursor_0065ba8c->group_bind_pending_09 = 1;
            g_world_cursor_0065ba8c->input_delta_0c[2] =
                g_world_cursor_0065ba8c->input_delta_0c[2] + 1;
            if (g_world_cursor_0065ba8c->particle_04 != 0) {
                g_world_cursor_0065ba8c->particle_04->value_214 = 3000.0f;
                g_world_cursor_0065ba8c->particle_04->value_218 = 6000.0f;
                g_world_cursor_0065ba8c->particle_04->value_1c8 = 0x14;
            }
        }
    } else {
        BindCursorMonsterToGroup004914E0();
    }
    if (gfRightButtonState != 0) {
        if (g_world_cursor_0065ba8c->detached_50 != 0) {
            g_world_cursor_0065ba8c->track_ground_41 = 0;
        }
        g_world_cursor_0065ba8c->input_delta_0c[1] =
            g_world_cursor_0065ba8c->input_delta_0c[1] + (0xf0 - cursor_point.y);
    } else {
        g_world_cursor_0065ba8c->input_delta_0c[0] =
            g_world_cursor_0065ba8c->input_delta_0c[0] + (cursor_point.x - 0x140);
        g_world_cursor_0065ba8c->input_delta_0c[2] =
            g_world_cursor_0065ba8c->input_delta_0c[2] + (0xf0 - cursor_point.y);
    }
    WarpSystemCursor(0x140, 0xf0);
    ApplyWorldCursorInput00490C60();
    if (g_world_cursor_0065ba8c != 0) {
        position = g_world_cursor_0065ba8c->position_28;
    } else {
        position.Set(0.0, 0.0, 0.0);
    }
    if (gfLeftButtonState == 0) {
        if (g_world_cursor_0065ba8c->left_held_48 != 0 && gXStatus.iTargetingMode == 3) {
            GetCameraPosition(&camera);
            if (ResolveWorldCursorTarget004921E0(&resolved) != 0 &&
                g_octree_6598a4->TraceLineOfSight(&camera, &resolved, 1, -3, -3, 1, 0) == 0) {
                box_min = resolved + g_world_cursor_0065ba8c->offset_c4;
                box_max = resolved + g_world_cursor_0065ba8c->offset_d0;
                if (g_world_cursor_0065ba8c->footprint_mode_c0 == 0 ||
                    g_octree_6598a4->TestBoxOccupied(&box_min, &box_max) == 0) {
                    AimAtPlace(g_status_685170.selected_character);
                    if (g_world_cursor_0065ba8c == 0) {
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
        g_world_cursor_0065ba8c->left_held_48 = 0;
    } else {
        g_world_cursor_0065ba8c->left_held_48 = 1;
    }
    if (old_position.x != position.x || old_position.y != position.y ||
        old_position.z != position.z) {
        if (g_world_cursor_0065ba8c->detached_50 != 0) {
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

    trace_from = *target + g_world_cursor_0065ba8c->probe_center_54;
    origin = g_world_cursor_0065ba8c->position_28;
    dist = (*target - origin).Length();
    if (dist == g_zero_005ebb40) {
        return 0;
    }
    last_valid = g_world_cursor_0065ba8c->position_28;
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
                probe = g_world_cursor_0065ba8c->probe_offsets_60[i];
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
            if (g_world_cursor_0065ba8c->track_ground_41 != 0) {
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
            trace_from = end_pos + g_world_cursor_0065ba8c->probe_center_54;
            for (i = 0; i < 8; i++) {
                probe = end_pos + g_world_cursor_0065ba8c->probe_offsets_60[i];
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
        g_world_cursor_0065ba8c->track_ground_41 = 1;
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
    cursor = g_world_cursor_0065ba8c;
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

/* Resolve the world cursor's target position: start from the cursor's stored
   position, ground-probe its candidate corner offsets and lift the target to
   the best settled height. With the fixed-offset flag the two stored offsets
   and their crossed corner combinations are probed instead, and a settled
   height too far from the input height fails the resolution. */
// FUNCTION: WIZ8 0x004921E0
int ResolveWorldCursorTarget004921E0(srVector3T<float>* position)
{
    W8WorldCursorState* cursor = g_world_cursor_0065ba8c;
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