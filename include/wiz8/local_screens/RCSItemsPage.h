#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/regions.h"

struct W8Character;
struct W8CombatSlot;

/* The caster slot the use-item-on-character path aims from. */
extern int giCasterCharSlot;

/* The stack being split while the split-stack dialog is open; set by
   OpenSplitStackDialog005BA400 and consumed by SplitStackDialogResult005BAA80. */
extern W8ItemInstance* g_split_item_source_0069c424;

/* The result kind SplitStackDialogResult005BAA80 treats as acceptance. */
extern int g_split_result_kind_005efb44;

/* The origin and inventory-mode kind shared by every split-item dialog; the
   NPC trade path in MainGameScreen.cpp selects the trade-kind globals instead. */
extern int g_split_dialog_x_005efb4c;
extern int g_split_dialog_y_005efb50;
extern int g_split_dialog_kind_005efb64;

/* The x origin the item info dialogs open at; the main game screen keeps its
   own y. */
extern int g_info_dialog_x_005ef958;

/* 0x005EE65C: the character-quote event kind queued when an item action fails
   or an identification finishes. */
extern int g_character_event_kind_005ee65c;

void SetItemPageMode005B9FD0(char mode);
/* Index-bound button callbacks stored by the camp panel creators in
   ReviewCharacterScreen.cpp. */
void SetItemPageMode005B9FB0(void);
void SetItemPageMode005B9FC0(void);
void OpenItemInfoDialog005BA110(W8ItemInstance* item, W8DialogDestroyCallback destroy_callback);
void OpenStatInfoDialog005BA200(void);
void OpenStatInfoDialog005BA210(void);
void OpenStatInfoDialog005BA220(void);
void OpenStatInfoDialog005BA230(void);
void OpenStatInfoDialog005BA240(void);
void OpenStatInfoDialog005BA250(void);
void OpenStatInfoDialog005BA260(void);
void OpenSecondaryStatInfoDialog005BA270(void);
void OpenSecondaryStatInfoDialog005BA280(void);
void OpenSecondaryStatInfoDialog005BA290(void);
void OpenSecondaryStatInfoDialog005BA2A0(void);
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
bool CanCharacterUseItemEntry005BAA10(W8Character* character, W8ItemInstance* item);
bool CanSplitItemStack005BAA50(const W8ItemInstance* item);
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
unsigned char BackpackRegionHandler005BB350(const InputAtom* event, W8Region* region);
unsigned char EquipSlotRegionHandler005BB560(const InputAtom* event, W8Region* region);
unsigned char ItemPoolRegionHandler005BB900(const InputAtom* event, W8Region* region);
unsigned char RealmTabRegionHandler005BBBB0(const InputAtom* event, W8Region* region);
unsigned char PanelTabRegionHandler005BBC70(const InputAtom* event, W8Region* region);
void SetItemTooltip005BBD30(W8ItemInstance* item, W8Region* region);
void DrawCampItemIcons005BBE30(void);

/* Unresolved gap callee, declared for the call sites in this unit. */
bool IsSpecialItemId004DA0F0(W8ItemInstance* item);

/* 0x0061E7C4: the twelve equip-slot label message ids indexed by region
   callback_id; gap-owned table, also read by the AssayDialog TU. */
extern const unsigned short g_equip_slot_label_ids_61e7c4[12];
