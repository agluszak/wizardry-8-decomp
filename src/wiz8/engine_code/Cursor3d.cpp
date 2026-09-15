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
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/stParticle.h"
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
srClass* g_cursor_value_0065ba94;

/* 0x60ab44: the world cursor's saved slot value; -1 until a cursor is torn
   down. Only ever copied whole between here and the cursor, so its domain is
   still unknown. */
// GLOBAL: WIZ8 0x0060ab44
int g_cursor_saved_value_60ab44 = -1;

// GLOBAL: WIZ8 0x0060ab48
float g_float_60ab48 = 4000.0f;

// GLOBAL: WIZ8 0x005ecb08
const float g_float_005ecb08 = 750.0f;

// GLOBAL: WIZ8 0x005ecb20
const float g_float_005ecb20 = -10000000.0f;

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
    cursor->flag_09 = 0;
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
        position->x = 0.0f;
        position->y = 0.0f;
        position->z = 0.0f;
    }
}

/* Show the world cursor: reattach its monster to the world lists, mark it
   visible, reactivate its particle, then park the system cursor and clear the
   combat selection. */
// FUNCTION: WIZ8 0x00490B10
void ShowWorldCursor00490B10(void)
{
    if (g_world_cursor_0065ba8c != 0 && g_world_cursor_0065ba8c->visible_40 == 0) {
        UpdateMonster(g_world_cursor_0065ba8c->monster_00);
        UpdateCycleRepresentation004C59B0(g_world_cursor_0065ba8c->monster_00, g_world);
        PLAdoptAppend(g_world->plsMonsters, g_world_cursor_0065ba8c->monster_00);
        g_world_cursor_0065ba8c->visible_40 = 1;
        if (g_world_cursor_0065ba8c->particle_04 != 0) {
            g_world_cursor_0065ba8c->particle_04->SetActive(1);
        }
        Function427F00(0, 0);
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

    if (cursor == 0 || cursor->visible_40 == 0) {
        return;
    }
    monster = cursor->monster_00;
    world = g_world;
    PListRemove(world->plsMonsters, monster);
    DetachMonsterRepresentation(monster, world);
    cursor->visible_40 = 0;
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
        position->x = 0.0f;
        position->y = 0.0f;
        position->z = 0.0f;
    }
}

// FUNCTION: WIZ8 0x004914C0
bool IsWorldCursorVisible(void)
{
    return g_world_cursor_0065ba8c != 0 && g_world_cursor_0065ba8c->visible_40 != 0;
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

    if (g_world_cursor_0065ba8c->flag_09 != 0 && g_world_cursor_0065ba8c->light_24 != 0) {
        g_world_cursor_0065ba8c->flag_09 = 0;
        g_world_cursor_0065ba8c->value_14 = g_world_cursor_0065ba8c->value_14 + 1;
        if (g_world_cursor_0065ba8c->particle_04 != 0) {
            g_world_cursor_0065ba8c->particle_04->value_214 = 1000.0f;
            g_world_cursor_0065ba8c->particle_04->value_218 = 2000.0f;
            g_world_cursor_0065ba8c->particle_04->value_1c8 = 300;
        }
        if (g_world_cursor_0065ba8c == 0) {
            position.x = 0.0f;
            position.y = 0.0f;
            position.z = 0.0f;
        } else {
            position = g_world_cursor_0065ba8c->position_28;
        }
        index = GetMonsterGroupIndexByID(0x237, CURSOR3D_CPP, g_world_cursor_0065ba8c->value_4c, 0);
        if (index == 0xffffffff) {
            index = ILLength(
                reinterpret_cast<W8IList*>( // reinterpret-ok: retail passes the
                                            // monster-group PList to ILLength;
                                            // the two list layouts share the
                                            // length field.
                    gXStatus.plsMonsterGroupList));
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
    cursor->value_34 = -100000000.0f;
    forward.Set(0.0, 0.0, g_float_60ab48);
    target = camera + rotation.Transform(forward);
    cursor->position_28 = camera;
    cursor->flag_41 = 1;
    if (Function4919E0(&target) == 0) {
        cursor->position_28 = target;
        cursor->offset_18 = cursor->position_28;
        if (cursor->flag_50 == 0) {
            cursor->offset_18 -= camera;
        }
        target.y = target.y + g_float_005ecb08;
        Function48F800(&target, 1, 0);
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
        position->x = 0.0f;
        position->y = 0.0f;
        position->z = 0.0f;
    } else {
        *position = cursor->position_28;
    }
    if (cursor == 0) {
        return 1;
    }
    if (cursor->flag_c0 == 0) {
        for (i = 0; i < 4; i++) {
            probe = *position + cursor->corner_offsets_90[i];
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
