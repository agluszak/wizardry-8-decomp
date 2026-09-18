#pragma once

void InitializeItemVideoObjects(void);
void ReleaseGenericItemNames(void);

#include <wchar.h>

#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/dialog_code/DialogBase.h"

struct W8ItemDatabaseRecord;
struct W8NpcState;

/* W8Character::equipment index domain. The five body-location slots are fixed
   by CalcArmorClasses' {0,4,10,5,11} location table and the per-location hit
   weights; GetItemDefaultEquipSlot independently fixes the torso/legs/head/
   feet/hands classes. The inventory paper-doll regions identify the two
   stacked accessory cells and the cloak cell, while the four weapon cells are
   already established by the hand-pairing code. */
enum W8EquipSlot {
    W8_EQUIP_SLOT_HEAD = 0,
    W8_EQUIP_SLOT_MISC_1 = 1,
    W8_EQUIP_SLOT_MISC_2 = 2,
    W8_EQUIP_SLOT_CLOAK = 3,
    W8_EQUIP_SLOT_TORSO = 4,
    W8_EQUIP_SLOT_FEET = 5,
    W8_EQUIP_SLOT_PRIMARY_WEAPON = 6,
    W8_EQUIP_SLOT_SECONDARY_WEAPON = 7,
    W8_EQUIP_SLOT_ALTERNATE_PRIMARY_WEAPON = 8,
    W8_EQUIP_SLOT_ALTERNATE_SECONDARY_WEAPON = 9,
    W8_EQUIP_SLOT_LEGS = 10,
    W8_EQUIP_SLOT_HANDS = 11,
    W8_EQUIP_SLOT_COUNT = 12
};

bool CanCharacterActivateItem(W8Character* character, const W8ItemInstance* item);

extern const int g_item_spell_presentation[11];
/* 0x00648C5C: the paper-doll icon of each of the twelve equipment slots. The
   two alternate-set hand slots have none, which is exactly the value the
   bound-item predicates refuse to hold a binding behind. */
extern const int g_equip_slot_icons[12];
int GetItemInHand(void);

void SetHandType(W8Character* character, unsigned int equip_slot);
unsigned int GetEquipmentBindingDifficulty(int character_index);
unsigned char CompatiblePartnerItems(int weapon_item_id, int off_hand_item_id); /* 0x0051C8F0 */
bool ItemHasQuantityKindFour(int item_id);
int GetPairedEquipSlot(int equip_slot);
wchar_t* GetItemDisplayName(const W8ItemInstance* item);

/* 0x0051B7B0 and 0x0051CCE0 are also expanded at their own call sites inside
   this unit: retail inlines the display-name body seven times in UseItem, three
   times in FormatItemDisplayName and twice in CastItemSpell0051EE70, and the
   name-kind test once in EquipMatchingPartnerItem, while nine and eight call
   sites in other units call the out-of-line copies. VC6 /O2 expands only
   inline-marked/member functions (measured: moving these definitions above their
   callers changes nothing), so the original marked them inline; with the body
   visible only here that marking also suppresses the out-of-line emission my
   build then needs. Kept as ordinary calls until the visibility question is
   settled by its owner. */
bool ItemHasSingledOutGenericName(int item_id);

bool AddItemToParty(W8ItemInstance* item, unsigned char announce, unsigned char skip_stacking);
unsigned char AddItemToPartyOrDrop(W8ItemInstance* item, unsigned char announce); /* 0x00522090 */
bool AddItemToCharacter(W8Character* character, W8ItemInstance* item, char equip_if_possible,
                        char announce, char skip_stacking);
void GetOriginOfCharacterItem(int character_index, W8ItemInstance* item, unsigned char* origin,
                              unsigned short* slot);

void UnequipUnusableItems(W8Character* character); /* 0x0051D960 */
void EmptyItemRecord(W8ItemInstance* item, W8Character* character, unsigned char refresh);
void EmptyAllCarriedItems(W8Character* character);
unsigned char TryIdentifyItemFor(W8Character* character, W8ItemInstance* item);
unsigned char GetItemSpell(const W8ItemInstance* item);
int GetItemSpellRange(const W8ItemInstance* item); /* 0x005207E0 */
void ApplyIdentifyAttempt(W8ItemInstance* item, unsigned int strength,
                          unsigned int percent); /* 0x00520B40 */
/* 0x00520BC0: reveal one character's worn bindings; 0 none bound, 1 some
   still hidden, 2 all revealed. */
int RevealCharacterItemBindings(unsigned int party_slot, int strength, unsigned int percent);
wchar_t* FormatItemDisplayName(const W8ItemInstance* item, unsigned char include_quantity);
unsigned int GetItemStackValue(const W8ItemInstance* item);
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
int GetItemSpellPresentation(const W8ItemDatabaseRecord* record);
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
void SwapItemInstances(W8ItemInstance* item, W8ItemInstance* destination, W8Character* character,
                       unsigned char refresh); /* 0x0051FD20 */
void NormalizeItemStack(W8ItemInstance* item);
unsigned char MergeItemStacks(W8ItemInstance* destination, W8ItemInstance* source,
                              unsigned char* partially_merged);
void MergeItemUses(W8Character* character, W8ItemInstance* into, W8ItemInstance* from);
void UpdateFactsAfterAcquiringItem(const W8ItemInstance* item);
void DeliverExceptionalItemReaction(W8ItemInstance* item, unsigned char choose_character,
                                    W8Character* character);
char PartyAttemptsToIdentifyItem(W8ItemInstance* item, int argument_2);

bool CanItemLeaveItsSlot(const W8ItemInstance* item);                              /* 0x0051F2B0 */
bool IsItemWornByCharacter(W8Character* character, const W8ItemInstance* item);    /* 0x00520F20 */
bool IsItemCarriedByCharacter(W8Character* character, const W8ItemInstance* item); /* 0x00520F60 */
bool DropItemInHand(int arg_1);                                                    /* 0x0051BE50 */
/* 0x0051BA00: give the cursor item to a party slot (or the pool). Unresolved
   gap body; declared for PortraitSelectRegionEvent. */
void Function51BA00(int party_slot, char force_to_party);
void BindEquippedItem(W8Character* character, int equip_slot);         /* 0x0051D0D0 */
bool CanUnequipSlotItem(const W8Character* character, int equip_slot); /* 0x0051D1C0 */
bool CanEquipItemInSlot(W8Character* character, int item_id, unsigned char equip_slot,
                        char ignore_worn_items); /* 0x0051CEA0 */
/* 0x0051F2F0: merges the held stack into one existing stack, announcing on
   refusal; unresolved gap body, declared for the RCSItemsPage call site. */
char MergeItems(W8Character* character, W8ItemInstance* item);

/* Same equipment class, and same unidentified display name. */
bool ItemsShareEquipClass(const W8ItemInstance* first, const W8ItemInstance* second);
bool ItemsShareUnidentifiedName(const W8ItemInstance* first, const W8ItemInstance* second);

bool CanCharacterUseItem(const W8Character* character, int item_id);

/* Equip-slot bucket used by the items-page realm filters: body locations are
   Head/Torso/Feet/Legs/Hands, accessory slots are Misc #1/#2 and Cloak, and
   the four weapon slots form the hand bucket. */
int GetItemEquipSlotGroup(int item_id);

unsigned int CountIdentifyAttemptsNeeded(W8ItemInstance* item, unsigned int percent);

bool ItemClassNormalizesTarget(const W8ItemDatabaseRecord* record);

bool StoreItemWithCharacterOrParty(W8Character* character, W8ItemInstance* item, char party_first,
                                   int arg_4, int arg_5); /* 0x0051C280 */
bool ItemHasHiddenProperties(int item_id);                /* 0x00520750 */
/* Find the first equipped, or optionally carried, item with a matching
   unidentified database name kind. */
char FindCharacterItemByDatabaseKind005213C0(W8Character* character, short item_kind,
                                             W8ItemInstance** out, int include_backpack);
void MoveItem(W8ItemInstance* to, W8ItemInstance* from, int arg_3, int arg_4);

/* 0x0051B910: per-item-class notice index into gppStringList used for the
   unidentified ("Uncursed item" style) display name. */
unsigned short GetItemUnidentifiedNameIndex(const W8ItemInstance* item);

/* PC Item.cpp GLOBAL at 0x0061E810: the per-item-class notice index. */
extern const unsigned short g_generic_item_name_notice[147];

/* 0x0068C108: one lazily built generic name per unidentified-name index. */
enum { W8_GENERIC_ITEM_NAME_COUNT = 147 };
extern wchar_t* g_generic_item_names[W8_GENERIC_ITEM_NAME_COUNT];

/* Defined in Dialog Code\AssayDialog.cpp (GLOBAL 0x0061E7DC): the
   gppStringList index of each equipment class's display name. */

/* Unresolved gap, declared for the split-stack dialog's trade-price labels:
   the gold price of a stack in the active trade context. The mode argument
   selects the pricing direction (0 for the buy side, 1 for the sell side). */
int CalculateTradeStackPrice(W8NpcState* npc, W8ItemInstance* item, int mode); /* 0x0055B5E0 */

extern int g_held_item_source_006840c0;
extern unsigned char g_held_item_origin_006840c4;
extern unsigned short g_held_item_slot_006840c5;
extern unsigned char g_byte_652da6;

enum W8ItemOrigin {
    W8_ITEM_ORIGIN_BACKPACK = 0,
    W8_ITEM_ORIGIN_EQUIPPED = 1,
    W8_ITEM_ORIGIN_PARTY_POOL = 2,
    W8_ITEM_ORIGIN_COUNT = 3
};

void BindCharacterItems(int party_slot, int arg_2); /* 0x0051D2C0 */
W8ItemInstance* FindCharacterItemAt(int party_slot, unsigned char origin,
                                    unsigned short slot); /* 0x00522180 */
void RecordItemOrigin(int party_slot, unsigned char origin, unsigned short slot);
void RemoveCharacterItem(W8Character* character, W8ItemInstance* item, char arg_3);
unsigned char RemovePartyItemByID005215D0(int item_id, char remove_all);

unsigned char CanUseItemForAction(int party_slot, const W8ItemInstance* item);

/* Unresolved gap callees, declared for the ReviewCharacterScreen.cpp camp item
   handler. 0x0051E980 scans the merge-kind table for the related
   unidentified-name kind of an item. 0x0051CDE0 reports whether the held item
   may occupy an equipment slot given the item in its paired hand slot.
   0x00521E20 shifts the party pool open and inserts the item at an index. */
char GetItemMergeKind0051E980(int item_id, short* related_kind);
char HeldItemFitsPairedSlot0051CDE0(int party_slot, int equip_slot);
char InsertItemIntoPartyPool00521E20(W8ItemInstance* item, int index);
int ChooseCharacterEquipSlot(W8Character* character, int item_id);

/* 0x0051EB90: fill an equipment slot from the item that pairs with the given
   one - the alternate hand when its own item is compatible, otherwise the
   named item found anywhere on the character. */
void EquipMatchingPartnerItem(W8Character* character, W8ItemInstance* item, int item_id,
                              int equip_slot);
void MergeMatchingPartnerItem(W8Character* character, W8ItemInstance* item); /* 0x0051EA90 */

/* 0x0051BC00: hand one item to a party member, preferring the character or the
   party pool according to the flag exactly as StoreItemWithCharacterOrParty
   does, and then consume the source record. */
unsigned char GiveItemToCharacterOrParty(int uiChar, W8ItemInstance* item,
                                         unsigned char party_first);

/* 0x0051BA00: the item-in-hand form of the same store, reached from the
   portrait screen, after the whole party has had its identification attempt. */
unsigned char GiveHeldItemToCharacterOrParty(int uiChar, unsigned char party_first);

/* 0x0051DDE0: use one item as a character's action. `out_uses` receives the
   fatigue cost of the attempt, or -1 when nothing was attempted. */
unsigned char UseItem(W8Character* character, W8ItemInstance* item, int* out_uses);

/* Unresolved gap callees, declared for the action paths that reach them.
   0x0051DCD0 rates how hard one attempt at an item's spell is for a character
   of this skill level. 0x0051EE70 applies the item's spell and consumes the
   uses it took. */
unsigned int GetItemUseDifficulty0051DCD0(const W8Character* character, int skill,
                                          unsigned int skill_level, unsigned int spell_id,
                                          unsigned int power);
int CastItemSpell0051EE70(W8Character* character, W8ItemInstance* item, unsigned int power);

/* The same batch's remaining bodies, recovered together with them. */
void AimItemUseAtCurrentTarget0051DB60(W8Character* character, W8ItemInstance* item);
unsigned int SwapWeaponSetSlots0051D3B0(int party_slot, char announce, unsigned char refresh);
void SplitThrowableStackBetweenHands0051ED30(W8Character* character, int equip_slot);
void RemovePartyPoolEntry00521C20(unsigned int index);
unsigned char FindItemByDatabaseKindOnParty00521480(unsigned short item_kind,
                                                    W8ItemInstance** found,
                                                    W8Character** found_character,
                                                    int include_backpack);
void EmptyBackpackSlot00521AC0(W8Character* character, int slot);
void EmptyPartyPoolEntry00521CD0(int index);
void UpgradeProfessionClassItem005218C0(W8Character* character);

int __cdecl CompareItemsForPool(const void* first, const void* second);
void UpdateGadgeteerOmnigun(W8Character* character);
unsigned int SwapCharacterWeaponSets(int party_slot, char announce, int refresh);
void BindEveryPartyItem(void); /* 0x0051D230 */
/* 0x00522A00: whether the item's equip class is directly usable (0x17/0x19). */
char IsUsableItemClass00522A00(W8ItemInstance* item);
/* 0x00522B80: validate an item's embedded spell for use now; nonzero reports
   use blocked with the reason notice queued through the callback. */
char ValidateItemSpellUse(int character_index, W8ItemInstance* item,
                          W8DialogDestroyCallback callback);
bool CharacterHasServiceItem(W8Character* character);  /* 0x00522D40 */
int CountUsableCharacterItems(W8Character* character); /* 0x0051F870 */
/* 0x0051BF40: take gold from the party, clamping at zero. */
void SpendPartyGold(unsigned int amount);
