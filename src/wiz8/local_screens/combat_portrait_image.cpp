#include "wiz8/local_screens/MGSPortraits.h"
#include "wiz8/local_screens/MGSButtons.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/character_skills.h"
#include "wiz8/layouts/character.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/targeting.h"

/* The combat-strip portrait catalog index for one slot's chosen action and
   status. Most actions map to a fixed base; attacks pick per weapon skill and
   spells per realm, and the status selects the frame variant beside it. */
// FUNCTION: WIZ8 0x0059A180
short GetCombatPortraitImage(W8ActionKind action, int detail, char status, short slot)
{
    int image;

    switch (action) {
    case W8_ACTION_ATTACK:
        image = 14 * GetAttackMenuWeaponOffset(g_status.buffers.Char[slot].Hand[0].weapon_skill);
        break;
    case W8_ACTION_BERSERK:
        image = 0x7e;
        break;
    case W8_ACTION_BREATHE:
        image = 0x8c;
        break;
    case W8_ACTION_TURN_UNDEAD:
        image = 0x9a;
        break;
    case W8_ACTION_PRAY:
        image = 0xa8;
        break;
    case W8_ACTION_DEFEND:
        image = 0xb6;
        break;
    case W8_ACTION_PROTECT:
        image = 0xc4;
        break;
    case W8_ACTION_USE_ITEM:
        image = 0x142;
        break;
    case W8_ACTION_EQUIP:
        image = 0x134;
        break;
    case W8_ACTION_CAST_SPELL:
        switch (g_spell_records[detail].realm) {
        case W8_SPELL_REALM_FIRE:
            image = 0x126;
            break;
        case W8_SPELL_REALM_WATER:
            image = 0xe0;
            break;
        case W8_SPELL_REALM_AIR:
            image = 0x118;
            break;
        case W8_SPELL_REALM_EARTH:
            image = 0xee;
            break;
        case W8_SPELL_REALM_MENTAL:
            image = 0xfc;
            break;
        case W8_SPELL_REALM_DIVINE:
            image = 0x10a;
            break;
        default:
            image = 0xd2;
        }
        break;
    case W8_ACTION_WALK:
        image = 0x15e;
        break;
    case W8_ACTION_RUN:
        image = 0x16c;
        break;
    default:
        if (status == 2) {
            return 0x17d;
        }
        return CanAnyHandReachTarget(slot) ? 0x17a : 0x17c;
    }
    switch (status) {
    case 0:
        break;
    case 1:
        return image + 2;
    case 2:
        return image + 3;
    case 3:
        return image + 4;
    default:
        image = 0x17a;
        break;
    }
    return image;
}
