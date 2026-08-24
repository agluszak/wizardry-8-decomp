#pragma once

#include "wiz8/item_instance.h"
#include "wiz8/game_status.h"

#define g_game_started (g_status_685170.game_started_000c)

extern "C" {

extern const int g_item_spell_presentation[];
extern const int g_equip_slot_icons[];
extern unsigned int g_party_gold;
extern unsigned char g_shared_item_pool[];
extern unsigned int g_shared_item_pool_count;

bool AddItemToParty(
    W8ItemInstance* item, unsigned char announce, unsigned char skip_stacking);
void GetOriginOfCharacterItem(
    int character_index,
    void* item,
    unsigned char* origin,
    unsigned short* slot);

}
