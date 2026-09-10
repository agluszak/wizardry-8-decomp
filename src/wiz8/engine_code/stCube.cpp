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
#include <stdlib.h>

/* Engine Code\stCube.cpp. The cursor's node table and the node selection
   pass; the cursor state itself lives in Cursor3d.cpp. */

// GLOBAL: WIZ8 0x0065ba5c
int g_value_65ba5c;

/* One record of the world cursor's node table. Only the members the table walk
   and its teardown reach are modeled; the teardown's delete establishes the
   virtual identity and the three fields around the scene node. No authored
   name survives, so the release body's address names the class. */
class W8CursorNode0048DB30 {
public:
    virtual ~W8CursorNode0048DB30() {}
    srNode* node_04;                      /* 0x04 */
    unsigned char unknown_08[0x10];
    void* buffer_18;                      /* 0x18 */
    int value_1c;                         /* 0x1c */
};

// GLOBAL: WIZ8 0x0065ba64
W8CursorNode0048DB30** g_array_65ba64;

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

    for (unsigned int index = 0; index < count; ++index) {
        W8CursorNode0048DB30* entry = g_array_65ba64[index];

        if (entry != 0) {
            srNode* parent = 0;
            if (visible != 0) {
                parent = g_world->dynamic_scene;
            }
            entry->node_04->setParent(parent, 1);
        }
    }
}

/* Select the cursor node nearest the camera within the selection distance,
   remembering it for the next call. Answers whether one was close enough. */
// FUNCTION: WIZ8 0x0048EFC0
unsigned char SelectWorldCursorNode0048EFC0(void)
{
    if (g_world != 0 && g_world->camera != 0) {
        srVector3T<float> camera_position;
        srVector3T<double> camera_location;

        GetCameraPosition(&camera_position);
        camera_location.SetFromFloat(&camera_position);
        int selected = g_cursor_node_index_0060a9b0;
        if (selected >= 0 && selected < g_value_65ba5c) {
            W8CursorNode0048DB30* entry = g_array_65ba64[selected];
            srVector3T<double> target = entry->node_04->getLocation();

            if (entry != 0) {
                srVector3T<double> delta = target;

                delta -= camera_location;
                if (delta.Length() < g_double_005ecac8) {
                    return 1;
                }
            }
        }
        int count = g_value_65ba5c;
        for (int index = 0; index < count; ++index) {
            W8CursorNode0048DB30* entry = g_array_65ba64[index];
            srVector3T<double> target = entry->node_04->getLocation();

            if (entry != 0) {
                srVector3T<double> delta = target;

                delta -= camera_location;
                if (delta.Length() < g_double_005ecac8) {
                    g_cursor_node_index_0060a9b0 = index;
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* Release every node the cursor table still holds: free its scratch buffer,
   detach and release its scene node, drop it from the table and run its own
   destructor. A null head with a nonzero count spins, as in retail. */
// FUNCTION: WIZ8 0x0048DB30
void ReleaseWorldCursorNodes0048DB30(void)
{
    while (g_value_65ba5c != 0) {
        W8CursorNode0048DB30* entry = g_array_65ba64[0];

        if (entry != 0) {
            if (entry->buffer_18 != 0) {
                free(entry->buffer_18);
                entry->buffer_18 = 0;
            }
            entry->value_1c = 0;
            entry->node_04->setParent(0, 1);
            entry->node_04->release();
            for (int index = 0; index < g_value_65ba5c; ++index) {
                if (g_array_65ba64[index] == entry) {
                    for (int shift = index; shift < g_value_65ba5c - 1;
                         ++shift) {
                        g_array_65ba64[shift] = g_array_65ba64[shift + 1];
                    }
                    --g_value_65ba5c;
                    break;
                }
            }
            delete entry;
        }
    }
}
