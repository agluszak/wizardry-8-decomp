#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/item_instance.h"
#include "wiz8/regions.h"

struct W8Character;
struct W8CombatSlot;

/* The caster slot the use-item-on-character path aims from. */
extern int giCasterCharSlot;

/* The stack being split while the split-stack dialog is open; set by
   OpenSplitStackDialog005BA400 and consumed by SplitStackDialogResult005BAA80. */
extern W8ItemInstance* g_split_item_source_0069c424;

void SetItemPageMode005B9FD0(char mode);
void OpenItemInfoDialog005BA110(W8ItemInstance* item, W8DialogDestroyCallback destroy_callback);
void OpenStatInfoDialog005BA2B0(unsigned int uiIndex);
void OpenSecondaryStatInfoDialog005BA310(unsigned int uiIndex);
void IdentifyAndOpenItemInfo005BA370(W8ItemInstance* item);
void DropHeldItem005BA3D0(void);
void OpenSplitStackDialog005BA400(W8ItemInstance* item);
void UseItem005BA4F0(W8ItemInstance* item);
void MergeItemStacksWithHeld005BA5D0(W8ItemInstance* item);
void ReportCastResult005BA620(int party_slot);
void UseHeldItemOnItem005BA740(W8ItemInstance* item);
void TargetCharacterWithHeldItem005BA8E0(unsigned int uiTargetChar);
unsigned char CanCharacterUseItemEntry005BAA10(W8Character* character, W8ItemInstance* item);
unsigned char CanSplitItemStack005BAA50(const W8ItemInstance* item);
void SplitStackDialogResult005BAA80(W8DialogBase* dialog);
void UpdateItemCursorForState005BAD20(int flag, W8ItemInstance* item, int slot);
void SetHandCursors005BAFC0(char mode);
void UnequipBothHands005BB010(void);
void TogglePartyRowFlag005BB140(void);
void SelectItemsRealmTab005BB1C0(void);
void SelectItemsRealmTab005BB1D0(void);
void SelectItemsRealmTab005BB1E0(void);
void SelectItemsRealmTab005BB1F0(void);
void SelectItemsRealmTab005BB200(void);
void SelectItemsRealmTab005BB210(void);
void SortPartyItemPool005BB220(void);
void SelectItemsRealmTab005BB250(int tab);
unsigned char BackpackRegionHandler005BB350(const W8RegionEvent* event, W8Region* region);
unsigned char EquipSlotRegionHandler005BB560(const W8RegionEvent* event, W8Region* region);
unsigned char ItemPoolRegionHandler005BB900(const W8RegionEvent* event, W8Region* region);
unsigned char RealmTabRegionHandler005BBBB0(const W8RegionEvent* event, W8Region* region);
unsigned char PanelTabRegionHandler005BBC70(const W8RegionEvent* event, W8Region* region);
void SetItemTooltip005BBD30(W8ItemInstance* item, W8Region* region);

/* 0x005DCED0 split-stack dialog (unresolved gap; the class is not recovered).
   Function5DCED0 is its 0xd8-byte constructor invoked __thiscall on fresh
   storage; the destroy callback reads the two fields it fills at +0xc0 and
   +0xc8, so they are declared here from the observed offsets only. */
void* Function5DCED0(void* memory, int kind, W8ItemInstance* item, int param);

/* Unresolved gap callees, declared for the call sites in this unit. */
char Function522A00(W8ItemInstance* item);
char Function522B80(int character_index, W8ItemInstance* item, int arg_3);
void Function52FE80(int party_slot, char arg_2);
int Function548E20(int party_slot, unsigned int arg_2);
char Function4DA0F0(W8ItemInstance* item);
unsigned int Function51D3B0(int party_slot, char arg_2, int arg_3);
char Function53C2C0(int party_slot);
void Function5A49D0(unsigned int realm);
void Function5A4A00(void);
void Function5A4C00(const wchar_t* text, void* callback, int arg_3, int arg_4);
void Function5A4C70(W8ItemInstance* item, int slot_index, unsigned int origin);
char Function5A5F30(char arg_1);
void Function5A6020(W8ItemInstance* item);
char Function5A6310(W8ItemInstance* item);
int Function5A6340(int party_slot, int action, int detail_count, W8CombatSlot* target);

/* Gap-owned value externs read by this unit (split-dialog geometry/kind and
   the character event id queued by UseHeldItemOnItem005BA740). */
extern int g_value_005efb44;
extern int g_value_005efb4c;
extern int g_value_005efb50;
extern int g_value_005efb64;
extern int g_value_005ef958;
extern int g_value_005ef95c;
extern int g_value_005ee65c;

/* 0x0061E7C4: the twelve equip-slot label message ids indexed by region
   callback_id; gap-owned table, also read by the AssayDialog TU. */
extern const unsigned short g_equip_slot_label_ids_61e7c4[12];
