#include "wiz8/combat_state.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/xstatus.h"
#include "wiz8/character.h"
#include "wiz8/game_status.h"
#include "wiz8/item_instance.h"
#include "wiz8/screen_state.h"

/* Party encumbrance redistribution. The original translation-unit spelling is
   not established; this descriptive name is provisional. */

extern unsigned int GetItemStackWeight(const W8ItemInstance* item);
extern void Function4EE000(W8Character* character);
extern void Function4EE220(W8Character* character);
extern void Function4EE9D0(W8Character* character);
extern void RequestRedraw(unsigned int mask);

// FUNCTION: WIZ8 0x004edd20
void Function4EDD20(void)
{
    int capacity[8];
    int unassigned[8];
    float load_ratio[8];
    W8Character* characters = g_status_685170.buffers.characters;
    W8PartySlotRow* active = g_status_685170.buffers.party_rows;

    if (!g_status_685170.game_started) {
        return;
    }
    if (gXStatus.fCombatMode) {
        gXStatus.field_028 = true;
        return;
    }

    unsigned int slot;
    for (slot = 0; slot < 8; ++slot) {
        W8Character* character = &characters[slot];
        character->party_weight_share = 0;
        if (active[slot].occupied != 0 &&
            character->unknown_0b01 < 0x12) {
            capacity[slot] = character->carrying_capacity;
            unassigned[slot] =
                capacity[slot] - character->inventory_weight;
            load_ratio[slot] =
                (float)unassigned[slot] * 100.0f / (float)capacity[slot];
        }
    }

    unsigned int party_weight = 0;
    for (slot = 0;
         slot < (unsigned int)g_status_685170.party_item_count_1791;
         ++slot) {
        party_weight += GetItemStackWeight(
            &g_status_685170.party_item_pool_0021[slot]);
    }

    for (party_weight >>= 1; party_weight != 0; --party_weight) {
        unsigned int best_slot = (unsigned int)-1;
        float best_ratio = -999999.0f;
        for (slot = 0; slot < 8; ++slot) {
            W8Character* character = &characters[slot];
            if (active[slot].occupied != 0 &&
                character->unknown_0b01 < 0x12 &&
                load_ratio[slot] > best_ratio) {
                best_ratio = load_ratio[slot];
                best_slot = slot;
            }
        }
        if (best_slot == (unsigned int)-1) {
            return;
        }
        W8Character* character = &characters[best_slot];
        ++character->party_weight_share;
        --unassigned[best_slot];
        load_ratio[best_slot] =
            (float)unassigned[best_slot] * 100.0f /
            (float)capacity[best_slot];
    }

    for (slot = 0; slot < 8; ++slot) {
        if (active[slot].occupied == 0) {
            continue;
        }
        W8Character* character = &characters[slot];
        int carried = character->inventory_weight + character->party_weight_share;
        character->total_carried_weight = carried;
        unsigned int percent =
            (unsigned int)(carried * 100) /
            (unsigned int)character->carrying_capacity;
        int old_band = character->load_category;
        if (percent < 50) {
            character->load_category = 0;
        }
        else if (percent < 70) {
            character->load_category = 1;
        }
        else if (percent < 85) {
            character->load_category = 2;
        }
        else {
            character->load_category = (percent > 100) + 3;
        }
        if (old_band != character->load_category) {
            Function4EE000(character);
            Function4EE220(character);
            Function4EE9D0(character);
        }
    }

    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME) {
        if (gXStatus.field_01f == 0) {
            RequestRedraw(0xff);
            gXStatus.field_028 = false;
            return;
        }
    }
    else if (g_screen_state_0068ec78.id == W8_SCREEN_CAMP &&
             g_camp_screen_0069c0f4 != 0) {
        g_camp_screen_0069c0f4->redraw_flags |= 0x2100;
    }
    gXStatus.field_028 = false;
}
