#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/targeting.h"
#include "wiz8/render_state.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/stParticle.h"
#include "surrender/srNode.h"

#include <math.h>
#include <stdlib.h>

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

// FUNCTION: WIZ8 0x004914C0
bool IsWorldCursorVisible(void)
{
    return g_world_cursor_0065ba8c != 0 && g_world_cursor_0065ba8c->visible_40 != 0;
}

/* Reset the cursor range to its default 4000 units. The cursor release path
   (ReleaseWorldCursor004909C0) writes the flattened camera distance to the
   same slot before the cursor is freed. */
// FUNCTION: WIZ8 0x00492530
void SetFloat60AB48(void)
{
    g_float_60ab48 = 4000.0f;
}
