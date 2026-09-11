#pragma once

void InitializeItemVideoObjects(void);
void ReleaseGenericItemNames(void);

#include "wiz8/item_instance.h"
#include "wiz8/game_status.h"

struct W8ItemDatabaseRecord;

unsigned char CanCharacterActivateItem(
    W8Character* character, const W8ItemInstance* item);

#define g_game_started (g_status_685170.game_started)

extern const int g_item_spell_presentation[11];
extern const int g_equip_slot_icons[6];
extern unsigned int g_party_gold;
extern unsigned char g_shared_item_pool[];
extern unsigned int g_shared_item_pool_count;

void SetHandType(W8Character* character, unsigned int equip_slot);
unsigned int Function520C70(int character_index);
unsigned char CompatiblePartnerItems(int weapon_item_id, int off_hand_item_id); /* 0x0051C8F0 */
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

void Function51D960(W8Character* character); /* 0x0051D960 */
void Function520070(W8ItemInstance* item, W8Character* character, unsigned char refresh);
void Function520310(W8Character* character);
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
/* 0x00521060: the whole-party counterpart. It tests the item in hand and the
   party item pool as well as the character slots, and reports which character
   held the match through the second output. */
bool FindItemOnParty(
    int item_id,
    W8ItemInstance** found,
    W8Character** found_character,
    int include_backpack,
    const W8ItemInstance* resume_after); /* 0x00521060 */
unsigned int GetItemStackWeight(const W8ItemInstance* item);
void CreateItemIntoHandOrPool(int item_id, unsigned char quality);
void AddPartyGold(int amount, char announce);

void CopyItemInstance(
    W8ItemInstance* destination,
    W8ItemInstance* source,
    W8Character* character,
    unsigned char refresh);
void SortPartyItemPool(void);
void Function520D10(
    W8ItemInstance* item, W8Character* character, unsigned char refresh);
void ReplaceOrCreateItem(
    W8ItemInstance* item, int item_id, unsigned char maximum_quantity,
    unsigned char force_identified, unsigned char mark_special);
void Function51FD20(
    W8ItemInstance* item, W8ItemInstance* destination, W8Character* character,
    unsigned char flag); /* 0x0051FD20 */
unsigned char Function51F900(
    W8ItemInstance* destination,
    W8ItemInstance* source,
    unsigned char* partially_merged);
void UpdateFactsAfterAcquiringItem(const W8ItemInstance* item);
void Function5227D0(
    W8ItemInstance* item, unsigned char choose_character, W8Character* character);
char PartyAttemptsToIdentifyItem(W8ItemInstance* item, int argument_2);

/* Same equipment class, and same unidentified display name. */
bool ItemsShareEquipClass(const W8ItemInstance* first, const W8ItemInstance* second);
bool ItemsShareUnidentifiedName(const W8ItemInstance* first, const W8ItemInstance* second);

bool CanCharacterUseItem(const W8Character* character, int item_id);

unsigned int CountIdentifyAttemptsNeeded(
    W8ItemInstance* item, unsigned int percent);

bool ItemClassNormalizesTarget(const W8ItemDatabaseRecord* record);

void MoveItem(W8ItemInstance* to, W8ItemInstance* from, int arg_3, int arg_4);

extern int g_held_item_source_006840c0;
extern unsigned char g_held_item_origin_006840c4;
extern unsigned short g_held_item_slot_006840c5;
extern unsigned char g_byte_652da6;

void BindCharacterItems(int party_slot, int arg_2);              /* 0x0051D2C0 */
W8ItemInstance* FindCharacterItemAt(
    int party_slot, unsigned char origin, unsigned short slot);          /* 0x00522180 */
void RecordItemOrigin(int party_slot, unsigned char origin, unsigned short slot);
void RemoveCharacterItem(int party_slot, W8ItemInstance* item, int arg_3);
unsigned char RemovePartyItemByID005215D0(int item_id, char remove_all);


unsigned char Function522A30(int party_slot, const W8ItemInstance* item);

