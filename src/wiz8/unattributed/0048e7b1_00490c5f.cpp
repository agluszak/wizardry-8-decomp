#include "wiz8/world_cursor.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/render_state.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/engine_code/stParticle.h"

extern "C" {
extern int g_value_65ba5c;
extern unsigned char g_flag_6850fb;
// GLOBAL: WIZ8 0x006850fb
unsigned char g_flag_6850fb;
// GLOBAL: WIZ8 0x0065ba5c
int g_value_65ba5c;
}

// GLOBAL: WIZ8 0x0065ba8c
W8WorldCursorState* g_world_cursor_0065ba8c;

/* Address quarantine 0048e7b1-00490c5f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x0048ED00
int GetValue65BA5C(void)
{
    return g_value_65ba5c;
}
// FUNCTION: WIZ8 0x0048FE80
bool IsFlag6850FBSet(void)
{
    return g_flag_6850fb != 0xff;
}

/* Hide the world cursor: detach its monster from the world lists, clear the
   visible flag, deactivate its particle, then refresh the party state. */
// FUNCTION: WIZ8 0x00490B90
void Function490B90(void)
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
