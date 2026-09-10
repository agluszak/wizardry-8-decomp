#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/render_state.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/stParticle.h"
#include "surrender/srNode.h"

/* Engine Code\Cursor3d.cpp. The cursor state, its visibility query and the
   range reset the update path calls. */

// GLOBAL: WIZ8 0x0065ba8c
W8WorldCursorState* g_world_cursor_0065ba8c;

// GLOBAL: WIZ8 0x0060ab48
float g_float_60ab48 = 4000.0f;

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

// FUNCTION: WIZ8 0x004914C0
unsigned char IsWorldCursorVisible(void)
{
    return g_world_cursor_0065ba8c != 0 &&
           g_world_cursor_0065ba8c->visible_40 != 0;
}

/* Reset the cursor range to its default 4000 units; the camera-distance
   update path overwrites the same slot while the cursor is tracked. */
// FUNCTION: WIZ8 0x00492530
void SetFloat60AB48(void)
{
    g_float_60ab48 = 4000.0f;
}
