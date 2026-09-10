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

/* Engine Code\stCube.cpp. The cursor's node table and the node selection
   pass; the cursor state itself lives in Cursor3d.cpp. */

extern "C" {
extern int g_value_65ba5c;
// GLOBAL: WIZ8 0x0065ba5c
int g_value_65ba5c;
}

// GLOBAL: WIZ8 0x0065ba64
int* g_array_65ba64;

/* The double selection range at 0x005ECAC8: node distances below it select the
   node. Read as 75000.0, not the zero a float view would give. */
// GLOBAL: WIZ8 0x005ecac8
const double g_double_005ecac8 = 75000.0;

// GLOBAL: WIZ8 0x0060a9b0
int g_cursor_node_index_0060a9b0 = -1;

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
                if (distance < g_double_005ecac8) {
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
                if (distance < g_double_005ecac8) {
                    g_cursor_node_index_0060a9b0 = index;
                    return 1;
                }
            }
        }
    }
    return 0;
}
