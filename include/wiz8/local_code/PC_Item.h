#pragma once

void InitializeItemVideoObjects(void);
void ReleaseGenericItemNames(void);

#include "wiz8/item_instance.h"
#include "wiz8/game_status.h"

struct W8ItemDatabaseRecord;

unsigned char CanCharacterActivateItem(W8Character* character, const W8ItemInstance* item);

extern const int g_item_spell_presentation[11];
extern const int g_equip_slot_icons[6];
int GetItemInHand(void);

void SetHandType(W8Character* character, unsigned int equip_slot);
unsigned int GetEquipmentBindingDifficulty(int character_index);
unsigned char CompatiblePartnerItems(int weapon_item_id, int off_hand_item_id); /* 0x0051C8F0 */
bool ItemHasSingledOutGenericName(int item_id);
int GetPairedEquipSlot(int equip_slot);
bool ItemHasQuantityKindFour(int item_id);

bool AddItemToParty(W8ItemInstance* item, unsigned char announce, unsigned char skip_stacking);
bool AddItemToCharacter(W8Character* character, W8ItemInstance* item, char equip_if_possible,
                        char announce, char skip_stacking);
void GetOriginOfCharacterItem(int character_index, void* item, unsigned char* origin,
                              unsigned short* slot);

void UnequipUnusableItems(W8Character* character); /* 0x0051D960 */
void EmptyItemRecord(W8ItemInstance* item, W8Character* character, unsigned char refresh);
void EmptyAllCarriedItems(W8Character* character);
unsigned char TryIdentifyItemFor(W8Character* character, W8ItemInstance* item);
unsigned char GetItemSpell(const W8ItemInstance* item);
W8WideChar* FormatItemDisplayName(const W8ItemInstance* item, unsigned char include_quantity);
unsigned int GetItemStackValue(const W8ItemInstance* item);
W8WideChar* GetItemDisplayName(const W8ItemInstance* item);
bool FindItemOnCharacter(W8Character* character, int item_id, W8ItemInstance** found,
                         int include_backpack, const W8ItemInstance* resume_after);
/* 0x00521060: the whole-party counterpart. It tests the item in hand and the
   party item pool as well as the character slots, and reports which character
   held the match through the second output. */
bool FindItemOnParty(int item_id, W8ItemInstance** found, W8Character** found_character,
                     int include_backpack, const W8ItemInstance* resume_after); /* 0x00521060 */
/* 0x00521240: the whole-party item count; the pool joins the scan when the
   caller asks for it. */
unsigned int CountItemOnParty(int item_id, W8ItemInstance** found, W8Character** first_holder,
                              int include_backpack);
/* 0x00521360: whether every occupied party slot carries one item. */
bool EveryCharacterHasItem(int item_id, int include_backpack);
unsigned int GetItemUnitWeight(const W8ItemInstance* item);
unsigned int GetItemStackWeight(const W8ItemInstance* item);
unsigned char GetItemEquipClass(const W8ItemInstance* item);
int GetItemDefaultEquipSlot(int item_id);
unsigned short GetItemEquipSlotMask(int item_id, char primary_off_hand_free,
                                    char alternate_off_hand_free, char primary_main_hand_free,
                                    char alternate_main_hand_free);
void CreateItemIntoHandOrPool(int item_id, unsigned char quality);
void AddPartyGold(int amount, char announce);

void CopyItemInstance(W8ItemInstance* destination, W8ItemInstance* source, W8Character* character,
                      unsigned char refresh);
void SortPartyItemPool(void);
void RefreshAfterItemRecordChange(W8ItemInstance* item, W8Character* character,
                                  unsigned char refresh);
void ReplaceOrCreateItem(W8ItemInstance* item, int item_id, unsigned char maximum_quantity,
                         unsigned char force_identified, unsigned char mark_special);
void Function51FD20(W8ItemInstance* item, W8ItemInstance* destination, W8Character* character,
                    unsigned char flag); /* 0x0051FD20 */
void NormalizeItemStack(W8ItemInstance* item);
unsigned char MergeItemStacks(W8ItemInstance* destination, W8ItemInstance* source,
                              unsigned char* partially_merged);
void UpdateFactsAfterAcquiringItem(const W8ItemInstance* item);
void DeliverExceptionalItemReaction(W8ItemInstance* item, unsigned char choose_character,
                                    W8Character* character);
char PartyAttemptsToIdentifyItem(W8ItemInstance* item, int argument_2);

/* Same equipment class, and same unidentified display name. */
bool ItemsShareEquipClass(const W8ItemInstance* first, const W8ItemInstance* second);
bool ItemsShareUnidentifiedName(const W8ItemInstance* first, const W8ItemInstance* second);

bool CanCharacterUseItem(const W8Character* character, int item_id);

unsigned int CountIdentifyAttemptsNeeded(W8ItemInstance* item, unsigned int percent);

bool ItemClassNormalizesTarget(const W8ItemDatabaseRecord* record);

bool StoreItemWithCharacterOrParty(W8Character* character, W8ItemInstance* item, char party_first,
                                   int arg_4, int arg_5); /* 0x0051C280 */
bool ItemHasHiddenProperties(int item_id);                /* 0x00520750 */
/* Unresolved gap, declared for the Party Import.cpp call site: scans the
   character's carried items for one whose database kind matches. */
char Function5213C0(W8Character* character, short item_kind, int* out, int arg_4); /* 0x005213C0 */
void MoveItem(W8ItemInstance* to, W8ItemInstance* from, int arg_3, int arg_4);

extern int g_held_item_source_006840c0;
extern unsigned char g_held_item_origin_006840c4;
extern unsigned short g_held_item_slot_006840c5;
extern unsigned char g_byte_652da6;

void BindCharacterItems(int party_slot, int arg_2); /* 0x0051D2C0 */
W8ItemInstance* FindCharacterItemAt(int party_slot, unsigned char origin,
                                    unsigned short slot); /* 0x00522180 */
void RecordItemOrigin(int party_slot, unsigned char origin, unsigned short slot);
void RemoveCharacterItem(int party_slot, W8ItemInstance* item, int arg_3);
unsigned char RemovePartyItemByID005215D0(int item_id, char remove_all);

unsigned char Function522A30(int party_slot, const W8ItemInstance* item);
