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
   OpenSplitStackDialog and consumed by SplitStackDialogResult. */
extern W8ItemInstance* g_split_item_source;

/* The result kind SplitStackDialogResult treats as acceptance. */
extern int g_split_result_kind;

/* The origin and inventory-mode kind shared by every split-item dialog; the
   NPC trade path in MainGameScreen.cpp selects the trade-kind globals instead. */
extern int g_split_dialog_x;
extern int g_split_dialog_y;
extern int g_split_dialog_kind;

/* The x origin the item info dialogs open at; the main game screen keeps its
   own y. */
extern int g_info_dialog_x;

/* 0x005EE65C: the character-quote event kind queued when an item action fails
   or an identification finishes. */
extern int g_character_event_kind_005ee65c;

void SetItemPageMode005B9FD0(char mode);
/* Index-bound button callbacks stored by the camp panel creators in
   ReviewCharacterScreen.cpp. */
void SetItemPageMode005B9FB0(void);
void SetItemPageMode005B9FC0(void);
void OpenItemInfoDialog(W8ItemInstance* item, W8DialogDestroyCallback destroy_callback);
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
void IdentifyAndOpenItemInfo(W8ItemInstance* item);
void DropHeldItem005BA3D0(void);
void OpenSplitStackDialog(W8ItemInstance* item);
void UseItem005BA4F0(W8ItemInstance* item);
void MergeItemStacksWithHeld(W8ItemInstance* item);
void ReportCastResult(int party_slot);
void UseHeldItemOnItem(W8ItemInstance* item);
void TargetCharacterWithHeldItem(unsigned int uiTargetChar);
bool CanCharacterUseItemEntry(W8Character* character, W8ItemInstance* item);
bool CanSplitItemStack(const W8ItemInstance* item);
void SplitStackDialogResult(W8DialogBase* dialog);
void UpdateItemCursorForState(int flag, W8ItemInstance* item, int slot);
void SetHandCursors(char mode);
void UnequipBothHands(void);
void TogglePartyRowFlag(void);
void SelectItemsRealmTab005BB1C0(void);
void SelectItemsRealmTab005BB1D0(void);
void SelectItemsRealmTab005BB1E0(void);
void SelectItemsRealmTab005BB1F0(void);
void SelectItemsRealmTab005BB200(void);
void SelectItemsRealmTab005BB210(void);
void SortPartyItemPool005BB220(void);
void SelectItemsRealmTab005BB250(int tab);
unsigned char BackpackRegionHandler(const InputAtom* event, W8Region* region);
unsigned char EquipSlotRegionHandler(const InputAtom* event, W8Region* region);
unsigned char ItemPoolRegionHandler(const InputAtom* event, W8Region* region);
unsigned char RealmTabRegionHandler(const InputAtom* event, W8Region* region);
unsigned char PanelTabRegionHandler(const InputAtom* event, W8Region* region);
void SetItemTooltip(W8ItemInstance* item, W8Region* region);
void DrawCampItemIcons(void);

bool IsSpecialItemId(W8ItemInstance* item);

/* 0x0061E7C4: the twelve equip-slot label message ids indexed by region
   callback_id; gap-owned table, also read by the AssayDialog TU. */
extern const unsigned short g_equip_slot_label_ids[12];
