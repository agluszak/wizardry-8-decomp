#pragma once

void InitializeItemVideoObjects(void);
void ReleaseGenericItemNames(void);

#include "wiz8/npc_trade_price.h"
#include "wiz8/character_skills.h"
#include <wchar.h>

#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/dialog_code/DialogBase.h"

struct W8ItemDatabaseRecord;
struct W8NpcState;
struct W8CombatSlot;

#include "wiz8/equipment_slots.h"

bool CanCharacterActivateItem(W8Character* character, const W8ItemInstance* item);

extern W8Skill g_item_spell_presentation[11];
/* The paper-doll icon of each of the twelve equipment slots. The
   two alternate-set hand slots have none, which is exactly the value the
   bound-item predicates refuse to hold a binding behind. */
extern int g_equip_slot_icons[12];
int GetItemInHand(void);

void SetHandType(W8Character* character, W8EquipSlot equip_slot);
unsigned int GetEquipmentBindingDifficulty(int character_index);
bool CompatiblePartnerItems(int weapon_item_id, int off_hand_item_id);
bool ItemUsesShots(int item_id);
W8EquipSlot GetPairedEquipSlot(W8EquipSlot equip_slot);
wchar_t* GetItemDisplayName(const W8ItemInstance* item);

bool ItemHasSingledOutGenericName(int item_id);

enum { W8_GENERIC_ITEM_NAME_COUNT = 147 };
/* The per-item-class notice index. */
extern unsigned short g_generic_item_name_notice[W8_GENERIC_ITEM_NAME_COUNT];
/* One lazily built generic name per unidentified-name index. */
extern wchar_t* g_generic_item_names[W8_GENERIC_ITEM_NAME_COUNT];
/* The message ids for what an item use did, read at every other entry; the
   last six are the item property labels. */
enum { W8_ITEM_PROPERTY_MESSAGE_FIRST = 19 };
extern unsigned short g_item_use_messages[25];

bool AddItemToParty(W8ItemInstance* item, bool announce, bool skip_stacking);
bool AddItemToPartyOrDrop(W8ItemInstance* item, bool announce);
bool AddItemToCharacter(W8Character* character, W8ItemInstance* item, bool equip_if_possible,
                        bool announce, bool skip_stacking);
void GetOriginOfCharacterItem(int character_index, W8ItemInstance* item, unsigned char* origin,
                              unsigned short* slot);

void UnequipUnusableItems(W8Character* character);
void EmptyItemRecord(W8ItemInstance* item, W8Character* character, bool refresh);
void EmptyAllCarriedItems(W8Character* character);
bool TryIdentifyItemFor(W8Character* character, W8ItemInstance* item);
int GetItemSpell(const W8ItemInstance* item);
W8RangeCategory GetItemSpellRange(const W8ItemInstance* item);
void ApplyIdentifyAttempt(W8ItemInstance* item, unsigned int strength, unsigned int percent);
/* Reveal one character's worn bindings; 0 none bound, 1 some
   still hidden, 2 all revealed. */
int RevealCharacterItemBindings(unsigned int party_slot, int strength, unsigned int percent);
wchar_t* FormatItemDisplayName(const W8ItemInstance* item, bool include_quantity);
unsigned int GetItemStackValue(const W8ItemInstance* item);
bool FindItemOnCharacter(W8Character* character, int item_id, W8ItemInstance** found,
                         int include_backpack, const W8ItemInstance* resume_after);
/* The whole-party counterpart. It tests the item in hand and the
   party item pool as well as the character slots, and reports which character
   held the match through the second output. */
bool FindItemOnParty(int item_id, W8ItemInstance** found, W8Character** found_character,
                     int include_backpack, const W8ItemInstance* resume_after);
/* The whole-party item count; the pool joins the scan when the
   caller asks for it. */
unsigned int CountItemOnParty(int item_id, W8ItemInstance** found, W8Character** first_holder,
                              int include_backpack);
/* Whether every occupied party slot carries one item. */
bool EveryCharacterHasItem(int item_id, int include_backpack);
unsigned int GetItemUnitWeight(const W8ItemInstance* item);
unsigned int GetItemStackWeight(const W8ItemInstance* item);
unsigned char GetItemEquipClass(const W8ItemInstance* item);
W8EquipSlot GetItemDefaultEquipSlot(int item_id);
W8Skill GetItemSpellPresentation(const W8ItemDatabaseRecord* record);
unsigned short GetItemEquipSlotMask(int item_id, bool primary_off_hand_free,
                                    bool alternate_off_hand_free, bool primary_main_hand_free,
                                    bool alternate_main_hand_free);
void CreateItemIntoHandOrPool(int item_id, bool quality);
void AddPartyGold(int amount, bool announce);

void CopyItemInstance(W8ItemInstance* destination, W8ItemInstance* source, W8Character* character,
                      bool refresh);
void SortPartyItemPool(void);
void RefreshAfterItemRecordChange(W8ItemInstance* item, W8Character* character, bool refresh);
void ReplaceOrCreateItem(W8ItemInstance* item, int item_id, bool maximum_quantity,
                         bool force_identified, bool mark_special);
void SwapItemInstances(W8ItemInstance* item, W8ItemInstance* destination, W8Character* character,
                       bool refresh);
void NormalizeItemStack(W8ItemInstance* item);
bool MergeItemStacks(W8ItemInstance* destination, W8ItemInstance* source, bool* partially_merged);
void MergeItemUses(W8Character* character, W8ItemInstance* into, W8ItemInstance* from);
void UpdateFactsAfterAcquiringItem(const W8ItemInstance* item);
void DeliverExceptionalItemReaction(W8ItemInstance* item, bool choose_character,
                                    W8Character* character);
/* How many of one item a character holds, counting a stack as its count and
   anything else as one, and optionally reporting the first slot it is in. */
int CountItemOnCharacter(W8Character* character, int item_id, W8ItemInstance** first,
                         int include_backpack);
/* Move the departing character's soul-bound equipment to the
   party pool or the held-item display before the slot is released. */
void StashDepartingCharacterItems(W8Character* character);
void StagePartySlotItemUse(int party_slot, W8ItemInstance* item, const W8CombatSlot* target);
bool PartyAttemptsToIdentifyItem(W8ItemInstance* item, int);

bool CanItemLeaveItsSlot(const W8ItemInstance* item);
bool IsItemWornByCharacter(W8Character* character, const W8ItemInstance* item);
bool IsItemCarriedByCharacter(W8Character* character, const W8ItemInstance* item);
bool DropItemInHand(int unused);
void BindEquippedItem(W8Character* character, W8EquipSlot equip_slot);
bool CanUnequipSlotItem(const W8Character* character, W8EquipSlot equip_slot);
bool AreAllHandSlotsEmpty(const W8Character* character);
bool CanEquipItemInSlot(W8Character* character, int item_id, unsigned char equip_slot,
                        bool ignore_worn_items);
/* Merges the held stack into one existing stack, announcing on
   refusal. */
bool MergeItems(W8Character* character, W8ItemInstance* item);

/* Same equipment class, and same unidentified display name. */
bool ItemsShareEquipClass(const W8ItemInstance* first, const W8ItemInstance* second);
bool ItemsShareUnidentifiedName(const W8ItemInstance* first, const W8ItemInstance* second);

bool CanCharacterUseItem(const W8Character* character, int item_id);

/* Equip-slot bucket used by the items-page realm filters: body locations are
   Head/Torso/Feet/Legs/Hands, accessory slots are Misc #1/#2 and Cloak, and
   the four weapon slots form the hand bucket. */
enum W8ItemEquipSlotGroup {
    W8_ITEM_EQUIP_GROUP_HAND = 2,
    W8_ITEM_EQUIP_GROUP_BODY = 3,
    W8_ITEM_EQUIP_GROUP_ACCESSORY = 4,
    W8_ITEM_EQUIP_GROUP_OTHER = 5
};
W8ItemEquipSlotGroup GetItemEquipSlotGroup(int item_id);

unsigned int CountIdentifyAttemptsNeeded(W8ItemInstance* item, unsigned int percent);

bool ItemClassNormalizesTarget(const W8ItemDatabaseRecord* record);

bool StoreItemWithCharacterOrParty(W8Character* character, W8ItemInstance* item, bool party_first,
                                   bool announce, bool equip_if_possible);
bool ItemHasHiddenProperties(int item_id);
/* Find the first equipped, or optionally carried, item with a matching
   unidentified database name kind. */
bool FindCharacterItemByDatabaseKind(W8Character* character, short item_kind, W8ItemInstance** out,
                                     int include_backpack);

/* Per-item-class notice index into gppStringList used for the
   unidentified ("Uncursed item" style) display name. */
unsigned short GetItemUnidentifiedNameIndex(const W8ItemInstance* item);

/* Gold price of a stack in the active trade context. price_kind uses
   W8TradePriceKind values. */
int CalculateTradeStackPrice(W8NpcState* npc, W8ItemInstance* item, char price_kind);
/* Whether any occupied, conscious party member can use the item. */
bool AnyPartyMemberCanUseItem(int item_id);

extern unsigned char g_party_has_slope_override_item;

void BindCharacterItems(int party_slot, bool announce);
/* Whether an item is bound to whoever is wearing it, which is what stops it
   being taken off or swapped away. */
bool IsItemBoundToWearer(const W8ItemInstance* item);
W8ItemInstance* FindCharacterItemAt(int party_slot, unsigned char origin, unsigned short slot);
void RemoveCharacterItem(W8Character* character, W8ItemInstance* item, bool remove_exhausted);
bool RemovePartyItemByID(int item_id, bool remove_all);

bool CanUseItemForAction(int party_slot, const W8ItemInstance* item);

/* Camp item operations: paired hand compatibility, merge-kind lookup, and
   insertion into the party's item pool. */
bool GetItemMergeKind(int item_id, short* related_kind);
bool HeldItemFitsPairedSlot(int party_slot, W8EquipSlot equip_slot);
bool InsertItemIntoPartyPool(W8ItemInstance* item, int index);
W8EquipSlot ChooseCharacterEquipSlot(W8Character* character, int item_id);

/* Fill an equipment slot from the item that pairs with the given
   one - the alternate hand when its own item is compatible, otherwise the
   named item found anywhere on the character. */
void EquipMatchingPartnerItem(W8Character* character, W8ItemInstance* item, int item_id,
                              W8EquipSlot equip_slot);
void MergeMatchingPartnerItem(W8Character* character, W8ItemInstance* item);

/* Hand one item to a party member, preferring the character or the
   party pool according to the flag exactly as StoreItemWithCharacterOrParty
   does, and then consume the source record. */
bool GiveItemToCharacterOrParty(int uiChar, W8ItemInstance* item, bool party_first);

/* The item-in-hand form of the same store, reached from the
   portrait screen, after the whole party has had its identification attempt. */
bool GiveHeldItemToCharacterOrParty(int uiChar, bool party_first);

/* Use one item as a character's action. `out_uses` receives the
   fatigue cost of the attempt, or -1 when nothing was attempted. */
unsigned char UseItem(W8Character* character, W8ItemInstance* item, int* out_uses);

/* How hard one attempt at an item's spell is for a character of this skill
   level. */
unsigned int GetItemUseDifficulty(const W8Character* character, W8Skill skill,
                                  unsigned int skill_level, unsigned int spell_id,
                                  unsigned int power);
int CastItemSpell(W8Character* character, W8ItemInstance* item, unsigned int power);

void AimItemUseAtCurrentTarget(W8Character* character, W8ItemInstance* item);
bool SwapWeaponSetSlots(int party_slot, bool announce, bool refresh);
void SplitThrowableStackBetweenHands(W8Character* character, W8EquipSlot equip_slot);
void RemovePartyPoolEntry(unsigned int index);
bool FindItemByDatabaseKindOnParty(unsigned short item_kind, W8ItemInstance** found,
                                   W8Character** found_character, int include_backpack);
void EmptyBackpackSlot(W8Character* character, int slot);
void EmptyPartyPoolEntry(int index);
void UpgradeProfessionClassItem(W8Character* character);

int __cdecl CompareItemsForPool(const void* first, const void* second);
void UpdateGadgeteerOmnigun(W8Character* character);
void BindEveryPartyItem(void);
/* Whether the item's equip class is directly usable (0x17/0x19). */
bool IsUsableItemClass(W8ItemInstance* item);
/* Validate an item's embedded spell for use now; nonzero reports
   use blocked with the reason notice queued through the callback. */
bool ValidateItemSpellUse(int character_index, W8ItemInstance* item,
                          W8DialogDestroyCallback callback);
bool CharacterHasServiceItem(W8Character* character);
int CountUsableCharacterItems(W8Character* character);
/* Take gold from the party, clamping at zero. */
void SpendPartyGold(unsigned int amount);
/* Add recharge uses to a stackable item (hourly tick, item 0x266). */
void AddItemUses(W8ItemInstance* item, char uses);
