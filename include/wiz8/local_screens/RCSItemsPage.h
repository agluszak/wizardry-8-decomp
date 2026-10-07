#pragma once

#include "wiz8/character_skills.h"

#include "wiz8/dialog_code/DialogBase.h"
#include "wiz8/integer_constants.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/regions.h"

struct W8Character;
struct W8CombatSlot;

/* The UI orders Other before Accessory, and places the two non-group filters last. */
enum W8CampItemFilterButton {
    W8_CAMP_ITEM_FILTER_BUTTON_HAND = 0,
    W8_CAMP_ITEM_FILTER_BUTTON_BODY = 1,
    W8_CAMP_ITEM_FILTER_BUTTON_OTHER = 2,
    W8_CAMP_ITEM_FILTER_BUTTON_ACCESSORY = 3,
    W8_CAMP_ITEM_FILTER_BUTTON_USABLE = 4,
    W8_CAMP_ITEM_FILTER_BUTTON_UNIDENTIFIED = 5
};

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
void OpenAttributeInfoDialog(W8Attribute attribute);
void OpenSecondaryAttributeInfoDialog(unsigned int uiIndex);
void IdentifyAndOpenItemInfo(W8ItemInstance* item);
void DropHeldCampItem(void);
void OpenSplitStackDialog(W8ItemInstance* item);
void UseCampItem(W8ItemInstance* item);
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
void ToggleCampHandItemFilter(void);
void ToggleCampBodyItemFilter(void);
void ToggleCampOtherItemFilter(void);
void ToggleCampAccessoryItemFilter(void);
void ToggleCampUsabilityFilter(void);
void ToggleCampUnidentifiedItemFilter(void);
void SortCampItemPool(void);
void ToggleCampItemFilter(W8CampItemFilterButton tab);
unsigned char BackpackRegionHandler(const InputAtom* event, W8Region* region);
unsigned char EquipSlotRegionHandler(const InputAtom* event, W8Region* region);
unsigned char ItemPoolRegionHandler(const InputAtom* event, W8Region* region);
unsigned char RealmTabRegionHandler(const InputAtom* event, W8Region* region);
unsigned char PanelTabRegionHandler(const InputAtom* event, W8Region* region);
void SetItemTooltip(W8ItemInstance* item, W8Region* region);
void DrawCampItemIcons(void);

bool IsSpecialItemId(W8ItemInstance* item);

/* The twelve equip-slot label message ids indexed by region callback_id. */
extern unsigned short g_equip_slot_label_ids[12];

/* Camp panel lifecycle helpers. */
int CreateCampActionPanel(void);
void ReleaseCampActionPanel(void);
void EnableCampActionButtons(void);
void DisableCampActionButtons(void);
void RefreshCampActionPanel(bool invalidate);
int CreateItemsTabPanel(void);
void ReleaseItemsTabPanel(void);
void UpdateCampItemFilters(void);
void DisableItemsRealmTabs(void);
void RefreshItemsTabPanel(bool invalidate);
int CreateCampSecondaryPanel(void);
void ReleaseCampSecondaryPanel(void);
void InvalidateCampPanel(void);
void EnableCampSecondaryPanel(void);
void DisableCampSecondaryPanel(void);
void RefreshCampSecondaryPanel(bool invalidate);
