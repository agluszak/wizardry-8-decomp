#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/render_state.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/stParticle.h"
#include "surrender/srNode.h"

#include <math.h>

extern "C" {
extern int g_value_65ba5c;
// GLOBAL: WIZ8 0x0065ba5c
int g_value_65ba5c;
}

// GLOBAL: WIZ8 0x0065ba64
int* g_array_65ba64;

// GLOBAL: WIZ8 0x005ecac8
const float g_float_005ecac8 = 0.0f;

// GLOBAL: WIZ8 0x0060a9b0
int g_cursor_node_index_0060a9b0 = -1;

// GLOBAL: WIZ8 0x0065ba8c
W8WorldCursorState* g_world_cursor_0065ba8c;

/* Address quarantine 0048e7b1-00490c5f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

/* The tracked cursor position, or the origin while there is no cursor. */
// FUNCTION: WIZ8 0x00490BF0
void GetWorldCursorPosition00490BF0(srVector3T<float>* position)
{
    if (g_world_cursor_0065ba8c != 0) {
        *position = g_world_cursor_0065ba8c->position_28;
    }
    else {
        position->x = 0.0f;
        position->y = 0.0f;
        position->z = 0.0f;
    }
}

// FUNCTION: WIZ8 0x0048ED00
int GetValue65BA5C(void)
{
    return g_value_65ba5c;
}

/* Reparent the world's cursor-attached nodes onto the dynamic scene, or
   detach them when hidden. Levels.cpp drives this from the world-cursor
   flag. */
// FUNCTION: WIZ8 0x0048ED70
void SetWorldCursorNodesVisible0048ED70(unsigned char visible)
{
    unsigned int count = g_value_65ba5c;
    unsigned int index = 0;

    if (count != 0) {
        do {
            int* entry = g_array_65ba64;
            if (index < g_value_65ba5c) {
                entry += index;
            }
            if (*entry != 0) {
                srNode* parent = 0;
                if (visible != 0) {
                    parent = g_world->dynamic_scene;
                }
                reinterpret_cast<srNode*>(entry[1])->setParent(parent, 1);
            }
            ++index;
        } while (index < count);
    }
}

/* Select the cursor node nearest the camera within the selection distance,
   remembering it for the next call. Answers whether one was close enough. */
// FUNCTION: WIZ8 0x0048EFC0
unsigned char SelectWorldCursorNode0048EFC0(void)
{
    if (g_world != 0 && g_world->camera != 0) {
        srVector3T<float> camera_position;
        GetCameraPosition(&camera_position);
        int selected = g_cursor_node_index_0060a9b0;
        if (selected >= 0 && selected < g_value_65ba5c) {
            int entry = g_array_65ba64[selected];
            srVector3T<double> target =
                (*reinterpret_cast<srNode**>(entry + 4))->getLocation();
            if (entry != 0) {
                double distance = sqrt(
                    (target.x - camera_position.x) *
                        (target.x - camera_position.x) +
                    (target.y - camera_position.y) *
                        (target.y - camera_position.y) +
                    (target.z - camera_position.z) *
                        (target.z - camera_position.z));
                if (distance < g_float_005ecac8) {
                    return 1;
                }
            }
        }
        int count = g_value_65ba5c;
        for (int index = 0; index < count; ++index) {
            int* slot = g_array_65ba64;
            if (index < g_value_65ba5c) {
                slot += index;
            }
            int entry = *slot;
            srVector3T<double> target =
                (*reinterpret_cast<srNode**>(entry + 4))->getLocation();
            if (entry != 0) {
                double distance = sqrt(
                    (target.x - camera_position.x) *
                        (target.x - camera_position.x) +
                    (target.y - camera_position.y) *
                        (target.y - camera_position.y) +
                    (target.z - camera_position.z) *
                        (target.z - camera_position.z));
                if (distance < g_float_005ecac8) {
                    g_cursor_node_index_0060a9b0 = index;
                    return 1;
                }
            }
        }
    }
    return 0;
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
    Function4C59C0(monster, world);
    cursor->visible_40 = 0;
    if (cursor->particle_04 != 0) {
        cursor->particle_04->SetActive(0);
    }
    SetFlag603C60();
    RequestRefreshPartyState();
}
