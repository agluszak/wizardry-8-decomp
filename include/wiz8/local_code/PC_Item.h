#pragma once

void InitializeItemVideoObjects(void);
void ReleaseGenericItemNames(void);

#include "wiz8/item_instance.h"
#include "wiz8/game_status.h"

#define g_game_started (g_status_685170.game_started)

extern const int g_item_spell_presentation[];
extern const int g_equip_slot_icons[];
extern unsigned int g_party_gold;
extern unsigned char g_shared_item_pool[];
extern unsigned int g_shared_item_pool_count;

void Function5201B0(W8Character* character, unsigned int equip_slot);
unsigned char CompatiblePartnerItems(int weapon_item_id, int off_hand_item_id);
bool ItemHasSingledOutGenericName(int item_id);
int GetPairedEquipSlot(int equip_slot);
bool ItemHasQuantityKindFour(int item_id);

bool AddItemToParty(
    W8ItemInstance* item, unsigned char announce, unsigned char skip_stacking);
bool AddItemToCharacter(
    W8Character* character, W8ItemInstance* item,
    char equip_if_possible, char announce, char skip_stacking);
void GetOriginOfCharacterItem(
    int character_index,
    void* item,
    unsigned char* origin,
    unsigned short* slot);

void Function520070(
    W8ItemInstance* item, W8Character* character, unsigned char refresh);
unsigned char TryIdentifyItemFor(W8Character* character, W8ItemInstance* item);
unsigned char GetItemSpell(const W8ItemInstance* item);
W8WideChar* FormatItemDisplayName(
    const W8ItemInstance* item, unsigned char include_quantity);
unsigned int GetItemStackValue(const W8ItemInstance* item);
W8WideChar* GetItemDisplayName(const W8ItemInstance* item);
bool FindItemOnCharacter(
    W8Character* character,
    int item_id,
    W8ItemInstance** found,
    int include_backpack,
    const W8ItemInstance* resume_after);
unsigned int GetItemStackWeight(const W8ItemInstance* item);
void CreateItemIntoHandOrPool(int item_id, unsigned char quality);
void AddPartyGold(int amount, char announce);
