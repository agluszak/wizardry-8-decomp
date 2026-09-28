#pragma once

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/integer_constants.h"
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

void SetCampInfoPageMode(char mode);
/* Index-bound button callbacks stored by the camp panel creators in
   ReviewCharacterScreen.cpp. */
void ShowCampItemsPage(void);
void ShowCampCharacterPage(void);
void OpenItemInfoDialog(W8ItemInstance* item, W8DialogDestroyCallback destroy_callback);
void OpenStrengthInfoDialog(void);
void OpenIntelligenceInfoDialog(void);
void OpenPietyInfoDialog(void);
void OpenVitalityInfoDialog(void);
void OpenDexterityInfoDialog(void);
void OpenSpeedInfoDialog(void);
void OpenSensesInfoDialog(void);
void OpenSecondaryAttributeInfoDialog0(void);
void OpenSecondaryAttributeInfoDialog1(void);
void OpenSecondaryAttributeInfoDialog3(void);
void OpenSecondaryAttributeInfoDialog4(void);
void OpenAttributeInfoDialog(unsigned int uiIndex);
void OpenSecondaryAttributeInfoDialog(unsigned int uiIndex);
void IdentifyAndOpenItemInfo(W8ItemInstance* item);
void DropHeldItem005BA3D0(void);
void OpenSplitStackDialog(W8ItemInstance* item);
void UseItem005BA4F0(W8ItemInstance* item);
void MergeItemStacksWithHeld(W8ItemInstance* item);
void ReportCastResult(int party_slot);
void UseHeldItemOnItem(W8ItemInstance* item);
void TargetCharacterWithHeldItem(unsigned int uiTargetChar);
int CanCharacterUseItemEntry(W8Character* character, W8ItemInstance* item);
bool CanSplitItemStack(const W8ItemInstance* item);
void SplitStackDialogResult(W8DialogBase* dialog);
void UpdateItemCursorForState(int flag, W8ItemInstance* item, int slot);
void SetHandCursors(char mode);
void UnequipBothHands(void);
void TogglePartyRowFlag(void);
void SelectCampRealmTab0(void);
void SelectCampRealmTab1(void);
void SelectCampRealmTab2(void);
void SelectCampRealmTab3(void);
void SelectCampRealmTab4(void);
void SelectCampRealmTab5(void);
void SortCampItemPool(void);
void SelectCampRealmTab(int tab);
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
extern unsigned short g_equip_slot_label_ids[12];

/* Camp panel create/enable/disable helpers; retail places them in the
   ReviewCharacterScreen.cpp span, but each has a direct or bounded
   RCSItemsPage.cpp anchor in the demo build. */
int CreateCampActionPanel(void);      /* 0x005B9070 */
void EnableCampActionButtons(void);   /* 0x005B9270 */
void DisableCampActionButtons(void);  /* 0x005B9310 */
int CreateItemsTabPanel(void);        /* 0x005B9350 */
void UpdateItemsRealmTabs(void);      /* 0x005B97B0 */
void DisableItemsRealmTabs(void);     /* 0x005B98C0 */
int CreateCampSecondaryPanel(void);   /* 0x005B9900 */
void EnableCampSecondaryPanel(void);  /* 0x005B9F00 */
void DisableCampSecondaryPanel(void); /* 0x005B9F60 */
