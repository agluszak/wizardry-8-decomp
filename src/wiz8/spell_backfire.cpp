#include "wiz8/gameplay_boundaries.h"

// FUNCTION: WIZ8 0x004ACA60
bool CanSpellBackfire(int spell_id)
{
    switch (g_spell_records[spell_id].target_type) {
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        switch (spell_id) {
        case 77:
        case 118:
        case 131:
            break;
        default:
            return true;
        }
        break;
    case 0:
    case 1:
    case 2:
        switch (spell_id) {
        case 3:
        case 13:
        case 16:
        case 34:
        case 44:
        case 56:
        case 58:
        case 74:
            return true;
        }
        break;
    case 10:
        return spell_id == 39;
    }
    return false;
}
