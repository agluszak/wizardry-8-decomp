#include "wiz8/combat_state.h"
#include "wiz8/game_status.h"
#include "wiz8/item_instance.h"
#include "wiz8/screen_state.h"

/* This party-load recalculation sits in the reviewed attribution gap between
   Combat.cpp and GameplayCode.cpp.  Keep it in the address quarantine until
   source evidence assigns the original translation unit. */

extern unsigned int GetItemStackWeight(const W8ItemInstance* item);
extern void Function4EE000(void* character);
extern void Function4EE220(void* character);
extern void Function4EE9D0(void* character);
extern void RequestRedraw(unsigned int mask);

// FUNCTION: WIZ8 0x004edd20
void Function4EDD20(void)
{
    int capacity[8];
    int unassigned[8];
    float load_ratio[8];
    unsigned char* characters = reinterpret_cast<unsigned char*>(
        g_status_685170.buffers.characters);
    W8PartySlotRow* active = g_status_685170.buffers.party_rows;

    if (!g_status_685170.game_started_000c) {
        return;
    }
    if (g_in_combat_00683f94) {
        g_flag_683fa0 = true;
        return;
    }

    unsigned int slot;
    for (slot = 0; slot < 8; ++slot) {
        unsigned char* character =
            characters + slot * W8_CHARACTER_SERIALIZED_SIZE;
        character[0xbbd] = 0;
        character[0xbbe] = 0;
        character[0xbbf] = 0;
        character[0xbc0] = 0;
        if (active[slot].occupied != 0 &&
            *(unsigned int*)(character + 0xb01) < 0x12) {
            capacity[slot] = *(int*)(character + 0xbc5);
            unassigned[slot] =
                capacity[slot] - *(int*)(character + 0xbb9);
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
            unsigned char* character =
                characters + slot * W8_CHARACTER_SERIALIZED_SIZE;
            if (active[slot].occupied != 0 &&
                *(unsigned int*)(character + 0xb01) < 0x12 &&
                load_ratio[slot] > best_ratio) {
                best_ratio = load_ratio[slot];
                best_slot = slot;
            }
        }
        if (best_slot == (unsigned int)-1) {
            return;
        }
        unsigned char* character =
            characters + best_slot * W8_CHARACTER_SERIALIZED_SIZE;
        ++*(int*)(character + 0xbbd);
        --unassigned[best_slot];
        load_ratio[best_slot] =
            (float)unassigned[best_slot] * 100.0f /
            (float)capacity[best_slot];
    }

    for (slot = 0; slot < 8; ++slot) {
        if (active[slot].occupied == 0) {
            continue;
        }
        unsigned char* character =
            characters + slot * W8_CHARACTER_SERIALIZED_SIZE;
        int carried = *(int*)(character + 0xbb9) +
                      *(int*)(character + 0xbbd);
        *(int*)(character + 0xbc1) = carried;
        unsigned int percent =
            (unsigned int)(carried * 100) /
            *(unsigned int*)(character + 0xbc5);
        int old_band = *(int*)(character + 0xbc9);
        if (percent < 50) {
            *(int*)(character + 0xbc9) = 0;
        }
        else if (percent < 70) {
            *(int*)(character + 0xbc9) = 1;
        }
        else if (percent < 85) {
            *(int*)(character + 0xbc9) = 2;
        }
        else {
            *(int*)(character + 0xbc9) = (percent > 100) + 3;
        }
        if (old_band != *(int*)(character + 0xbc9)) {
            Function4EE000(character);
            Function4EE220(character);
            Function4EE9D0(character);
        }
    }

    if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME) {
        if (g_flag_00683f97 == 0) {
            RequestRedraw(0xff);
            g_flag_683fa0 = false;
            return;
        }
    }
    else if (g_screen_state_0068ec78.id == W8_SCREEN_CAMP &&
             g_camp_screen_0069c0f4 != 0) {
        g_camp_screen_0069c0f4->redraw_flags |= 0x2100;
    }
    g_flag_683fa0 = false;
}
