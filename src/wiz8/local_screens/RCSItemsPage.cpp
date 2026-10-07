#include <windows.h>
#include "wiz8/spell_ids.h"
#include "wiz8/local_screens/RCSItemsPage.h"
#include "wiz8/integer_constants.h"

#include "wiz8/layouts/character.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/cursor.h"
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/StatInfoDialogs.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/Widget.h"
#include "wiz8/notices.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/sr_api.h"
#include "wiz8/fonts.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"

#include "input.h"
#include "soundman.h"
#include "Font.h"
#include "vsurface.h"
#include "line.h"

#include <new>
#include <wchar.h>
#include "wiz8/local_screens/OptionsScreen.h"

/* Retail Local Screens\RCSItemsPage.cpp - the items page of the camp
   review screen: page-mode toggling, the item-info/split/stat dialogs,
   use-item and targeting entry points, and the five region callbacks the
   camp region table registers for the backpack, equipment, item-pool,
   realm-tab and panel-tab areas. */

static void DrawCampItemLabel(W8ItemInstance* item, int left, int top, char flag);

// GLOBAL: WIZ8 0x0069C0EC
int giCasterCharSlot;

// GLOBAL: WIZ8 0x0069C424
W8ItemInstance* g_split_item_source;

// GLOBAL: WIZ8 0x0069c490
unsigned int g_camp_secondary_region_set;

/* The twelve equip-slot label message ids indexed by region
   callback_id. */
// GLOBAL: WIZ8 0x0061E7C4
unsigned short g_equip_slot_label_ids[12] = {
    0x433, 0x434, 0x435, 0x436, 0x437, 0x438, 0x439, 0x43a, 0x43b, 0x43c, 0x43d, 0x43e,
};

/* The two page-tab primary callbacks: the Items and Character info buttons
   created by CreateCampSecondaryPanel. */
// FUNCTION: WIZ8 0x005B9FB0
void ShowCampItemsPage(void)
{
    SetCampInfoPageMode(W8_CAMP_INFO_PAGE_ITEMS);
}

// FUNCTION: WIZ8 0x005B9FC0
void ShowCampCharacterPage(void)
{
    SetCampInfoPageMode(W8_CAMP_INFO_PAGE_CHARACTER);
}

/* The items page swaps two control panels in and out: mode zero shows the
   item page, mode one the character page. */
// FUNCTION: WIZ8 0x005B9FD0
void SetCampInfoPageMode(char mode)
{
    int index;

    switch (mode) {
    default:
        srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x544,
                     "Info toggle error");
        break;
    case W8_CAMP_INFO_PAGE_CHARACTER:
        g_camp_page_tabs[0]->DisableSecondaryState(true);
        g_camp_page_tabs[1]->EnableSecondaryState(true);
        g_camp_help_text->SetActive(false);
        g_camp_screen->character_info->SetEnabled(true);
        for (index = 0; index < 7; ++index) {
            g_camp_stat_labels[index]->SetActive(false);
        }
        for (index = 0; index < 4; ++index) {
            g_camp_info_labels[index]->SetActive(false);
        }
        break;
    case W8_CAMP_INFO_PAGE_ITEMS:
        g_camp_page_tabs[0]->EnableSecondaryState(true);
        g_camp_page_tabs[1]->DisableSecondaryState(true);
        g_camp_help_text->SetActive(true);
        g_camp_screen->character_info->SetEnabled(false);
        for (index = 0; index < 7; ++index) {
            g_camp_stat_labels[index]->SetActive(true);
        }
        for (index = 0; index < 4; ++index) {
            g_camp_info_labels[index]->SetActive(true);
        }
        break;
    }
    g_camp_screen->info_page = mode;
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_CHARACTER_INFO;
}

// FUNCTION: WIZ8 0x005BA110
void OpenItemInfoDialog(W8ItemInstance* item, W8DialogDestroyCallback destroy_callback)
{
    W8DialogBase* dialog;

    if (!IsItemWornByCharacter(g_review_character, item) &&
        !IsItemCarriedByCharacter(g_review_character, item)) {
        dialog = new W8AssayDialog(item, 0);
    } else {
        dialog = new W8AssayDialog(item, g_review_character);
    }
    dialog->SetText(&g_empty_wide_string);
    dialog->SetOrigin(g_info_dialog_x, g_info_dialog_y);
    dialog->m_destroy_callback = destroy_callback;
    DisplayCampDialog(dialog);
}

/* The secondary-activation callbacks the seven attribute labels and four
   secondary labels on the camp secondary panel carry: each opens the stat
   info dialog at its own index. Note retail wires both
   g_camp_info_labels[2] and [3] to the index-3 thunk. */
// FUNCTION: WIZ8 0x005BA200
void OpenStrengthInfoDialog(void)
{
    OpenAttributeInfoDialog(W8_ATTRIBUTE_STRENGTH);
}

// FUNCTION: WIZ8 0x005BA210
void OpenIntelligenceInfoDialog(void)
{
    OpenAttributeInfoDialog(W8_ATTRIBUTE_INTELLIGENCE);
}

// FUNCTION: WIZ8 0x005BA220
void OpenPietyInfoDialog(void)
{
    OpenAttributeInfoDialog(W8_ATTRIBUTE_PIETY);
}

// FUNCTION: WIZ8 0x005BA230
void OpenVitalityInfoDialog(void)
{
    OpenAttributeInfoDialog(W8_ATTRIBUTE_VITALITY);
}

// FUNCTION: WIZ8 0x005BA240
void OpenDexterityInfoDialog(void)
{
    OpenAttributeInfoDialog(W8_ATTRIBUTE_DEXTERITY);
}

// FUNCTION: WIZ8 0x005BA250
void OpenSpeedInfoDialog(void)
{
    OpenAttributeInfoDialog(W8_ATTRIBUTE_SPEED);
}

// FUNCTION: WIZ8 0x005BA260
void OpenSensesInfoDialog(void)
{
    OpenAttributeInfoDialog(W8_ATTRIBUTE_SENSES);
}

// FUNCTION: WIZ8 0x005BA270
void OpenSecondaryAttributeInfoDialog0(void)
{
    OpenSecondaryAttributeInfoDialog(0);
}

// FUNCTION: WIZ8 0x005BA280
void OpenSecondaryAttributeInfoDialog1(void)
{
    OpenSecondaryAttributeInfoDialog(1);
}

// FUNCTION: WIZ8 0x005BA290
void OpenSecondaryAttributeInfoDialog3(void)
{
    OpenSecondaryAttributeInfoDialog(3);
}

// FUNCTION: WIZ8 0x005BA2A0
void OpenSecondaryAttributeInfoDialog4(void)
{
    OpenSecondaryAttributeInfoDialog(4);
}

// FUNCTION: WIZ8 0x005BA2B0
void OpenAttributeInfoDialog(W8Attribute attribute)
{
    DisplayCampDialog(new W8AttributeInfoDialog(attribute));
}

// FUNCTION: WIZ8 0x005BA310
void OpenSecondaryAttributeInfoDialog(unsigned int uiIndex)
{
    DisplayCampDialog(new W8SecondaryAttributeInfoDialog(uiIndex));
}

// FUNCTION: WIZ8 0x005BA370
void IdentifyAndOpenItemInfo(W8ItemInstance* item)
{
    if (CanItemLeaveItsSlot(item)) {
        if (PartyAttemptsToIdentifyItem(item, 0) &&
            g_camp_screen->item_filters[W8_CAMP_ITEM_FILTER_UNIDENTIFIED] != 0) {
            RebuildCampItemList();
            g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
        }
    }
    OpenItemInfoDialog(item, 0);
    SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
}

// FUNCTION: WIZ8 0x005BA3D0
void DropHeldCampItem(void)
{
    if (ResolvePendingCampCharacter(true)) {
        if (DropItemInHand(0)) {
            SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
        }
    }
}

// FUNCTION: WIZ8 0x005BA400
void OpenSplitStackDialog(W8ItemInstance* item)
{
    W8DialogBase* dialog;

    g_split_item_source = 0;
    if (item->iItemNo != -1 && item->stack_count > 1 &&
        (g_item_records[item->iItemNo].flags & W8_ITEM_FLAG_NO_DISCARD) == 0 &&
        g_item_records[item->iItemNo].quantity_kind == W8_ITEM_QUANTITY_STACK) {
        g_split_item_source = item;
        dialog = new W8SplitItemDialog(g_item_split_inventory_mode, item, -1);
        dialog->SetText(&g_empty_wide_string);
        dialog->SetOrigin(g_split_dialog_x, g_split_dialog_y);
        dialog->m_destroy_callback = SplitStackDialogResult;
        DisplayCampDialog(dialog);
        SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
    }
}

// FUNCTION: WIZ8 0x005BA4F0
void UseCampItem(W8ItemInstance* item)
{
    unsigned short slot;

    if (ValidateItemSpellUse(giReviewCharSlot, item, 0)) {
        SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
        return;
    }
    if (CanCastFromItem(g_review_character, item)) {
        LearnSpellFromItem(g_review_character, item);
    } else {
        if (!IsUsableItemClass(item) || IsSpecialItemId(item)) {
            gXStatus.item_drag_active = true;
            gXStatus.dragged_item = item;
            gXStatus.dragged_character_slot = static_cast<char>(giReviewCharSlot);
            GetOriginOfCharacterItem(giReviewCharSlot, item, &gXStatus.dragged_item_origin, &slot);
        } else {
            if (!g_status.item_in_cursor) {
                MarkCampCharacterPending(item);
                CopyItemInstance(&g_status.item_in_hand, item, g_review_character, true);
            }
        }
        DismissSelectedPartyCharacter();
    }
    RebuildCampItemList();
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
    SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
}

// FUNCTION: WIZ8 0x005BA5D0
void MergeItemStacksWithHeld(W8ItemInstance* item)
{
    if (item->iItemNo != -1) {
        if (MergeItems(g_review_character, item)) {
            RebuildCampItemList();
            g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
            SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
        }
    }
}

// FUNCTION: WIZ8 0x005BA620
void ReportCastResult(int party_slot)
{
    wchar_t* text;
    W8Character* character;
    int result;

    StartBreathCycle(giReviewCharSlot, false);
    result = RevealCharacterItemBindingsByProfession(giReviewCharSlot, party_slot);
    character = g_status.buffers.Char + party_slot;
    if (result == 0) {
        SoundPlay("Data\\Sound\\Misc\\Spell Fizzle 01.wav", 0);
        text = FormatWideString(gppStringList[0x1be], character->name, 0, 1, 0);
        ShowCampNoticeLine(text, 0, true, false);
    } else if (result == 1) {
        SoundPlay("Data\\Sound\\Misc\\GeneralMagic.wav", 0);
        text = FormatWideString(gppStringList[0x1bd], character->name, 0, 1, 0);
        ShowCampNoticeLine(text, 0, true, false);
    } else if (result == 2) {
        SoundPlay("Data\\Sound\\Misc\\GeneralMagic.wav", 0);
        text = FormatWideString(gppStringList[0x1bc], character->name, 0, 1, 0);
        ShowCampNoticeLine(text, 0, true, false);
    }
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
    SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
}

// FUNCTION: WIZ8 0x005BA740
void UseHeldItemOnItem(W8ItemInstance* item)
{
    W8CombatSlot target;

    if (giCasterCharSlot < 0) {
        srAssertFail("giCasterCharSlot >= 0",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x668, 0);
    }
    if (giCasterCharSlot > 7) {
        srAssertFail("giCasterCharSlot < MAX_CHARS",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x669, 0);
    }
    if (!g_status.buffers.XChar[giCasterCharSlot].fOccupied) {
        srAssertFail("fCHAR_OCCUPIED(giCasterCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x66a, 0);
    }
    if (item->iItemNo != -1) {
        if (CanItemLeaveItsSlot(item)) {
            ResetCombatSlot(&target);
            target.iType = W8_TARGET_KIND_ITEM;
            if (CommitPartySlotSpell(giCasterCharSlot, W8_SPELL_IDENTIFY_ITEM, 8, &target) == 1) {
                OpenItemInfoDialog(item, 0);
                if (!item->identified) {
                    QueueCharacterEvent(g_status.buffers.Char + giCasterCharSlot,
                                        g_character_event_kind2, 0, g_character_event_no_flags,
                                        g_character_event_full_volume);
                }
            }
            RebuildCampItemList();
            g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
            SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
            giCasterCharSlot = -1;
            return;
        }
        QueueCharacterEvent(g_status.buffers.Char + giCasterCharSlot, g_character_event_kind2, 0,
                            g_character_event_no_flags, g_character_event_full_volume);
    }
}

// FUNCTION: WIZ8 0x005BA8E0
void TargetCharacterWithHeldItem(unsigned int uiTargetChar)
{
    W8CombatSlot target;

    if (uiTargetChar > 7) {
        srAssertFail("uiTargetChar < MAX_CHARS",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x692, 0);
    }
    if (!g_status.buffers.XChar[uiTargetChar].fOccupied) {
        srAssertFail("fCHAR_OCCUPIED(uiTargetChar)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x693, 0);
    }
    if (giCasterCharSlot < 0) {
        srAssertFail("giCasterCharSlot >= 0",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x694, 0);
    }
    if (giCasterCharSlot > 7) {
        srAssertFail("giCasterCharSlot < MAX_CHARS",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x695, 0);
    }
    if (!g_status.buffers.XChar[giCasterCharSlot].fOccupied) {
        srAssertFail("fCHAR_OCCUPIED(giCasterCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x696, 0);
    }
    ResetCombatSlot(&target);
    target.iType = W8_TARGET_KIND_CHARACTER;
    target.iChar = uiTargetChar;
    CommitPartySlotSpell(giCasterCharSlot, W8_SPELL_REMOVE_CURSE, 8, &target);
    RebuildCampItemList();
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
    SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
    giCasterCharSlot = -1;
}

// FUNCTION: WIZ8 0x005BAA10
int CanCharacterUseItemEntry(W8Character* character, W8ItemInstance* item)
{
    if (CanCharacterActivateItem(character, item) || CanCastFromItem(character, item) ||
        IsUsableItemClass(item)) {
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005BAA50
bool CanSplitItemStack(const W8ItemInstance* item)
{
    const W8ItemDatabaseRecord* record = g_item_records + item->iItemNo;
    if ((record->flags & W8_ITEM_FLAG_NO_DISCARD) != 0) {
        return false;
    }
    return record->quantity_kind == W8_ITEM_QUANTITY_STACK;
}

// FUNCTION: WIZ8 0x005BAA80
void SplitStackDialogResult(W8DialogBase* dialog)
{
    W8ItemInstance split;
    W8ItemInstance* destination;
    W8Character* character;
    unsigned int count;
    unsigned char remaining;
    unsigned char carried;

    if (static_cast<W8SplitItemDialog*>(dialog)->split_result != g_item_split_confirm_result) {
        return;
    }
    count = static_cast<W8SplitItemDialog*>(dialog)->split_count;
    remaining = g_split_item_source->stack_count - static_cast<unsigned char>(count);
    carried = static_cast<unsigned char>(count);
    if (count == 0) {
        return;
    }
    if (g_status.item_in_cursor) {
        if (count == g_split_item_source->stack_count) {
            return;
        }
        split = *g_split_item_source;
        split.stack_count = remaining;
        /* The remainder goes back to the slot the held stack came from when that
           slot can take it, otherwise anywhere on that character, then into the
           party pool, and as a last resort stays in hand. */
        destination = 0;
        character = 0;
        if (gXStatus.held_item_source != -1 &&
            (gXStatus.held_item_origin == W8_ITEM_ORIGIN_BACKPACK ||
             gXStatus.held_item_origin == W8_ITEM_ORIGIN_EQUIPPED)) {
            character = g_status.buffers.Char + gXStatus.held_item_source;
            if (gXStatus.held_item_origin == W8_ITEM_ORIGIN_EQUIPPED) {
                if (character->EquippedItem[static_cast<short>(gXStatus.held_item_slot)].iItemNo ==
                        -1 &&
                    CanEquipItemInSlot(character, g_status.item_in_hand.iItemNo,
                                       static_cast<unsigned char>(gXStatus.held_item_slot),
                                       false) &&
                    CanCharacterUseItem(character, g_status.item_in_hand.iItemNo)) {
                    destination =
                        character->EquippedItem + static_cast<short>(gXStatus.held_item_slot);
                }
            } else if (character->backpack[static_cast<short>(gXStatus.held_item_slot)].iItemNo ==
                       -1) {
                destination = character->backpack + static_cast<short>(gXStatus.held_item_slot);
            }
        }
        if (destination == 0 &&
            (character == 0 || !AddItemToCharacter(character, &split, false, false, false)) &&
            !AddItemToParty(&split, false, false)) {
            ShowCampNoticeLine(gppStringList[0x915], 0, true, false);
            g_status.item_in_hand.stack_count = remaining;
            if (ResolvePendingCampCharacter(true) && DropItemInHand(0)) {
                SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
            }
            destination = &g_status.item_in_hand;
        }
        if (destination != 0) {
            CopyItemInstance(destination, &split, 0, true);
        }
    } else {
        if (count == g_split_item_source->stack_count) {
            CopyItemInstance(&g_status.item_in_hand, g_split_item_source, g_review_character, true);
            carried = g_status.item_in_hand.stack_count;
        } else {
            split = *g_split_item_source;
            split.stack_count = static_cast<unsigned char>(count);
            g_split_item_source->stack_count = remaining;
            CopyItemInstance(&g_status.item_in_hand, &split, 0, true);
            gXStatus.held_item_source = giReviewCharSlot;
            GetOriginOfCharacterItem(giReviewCharSlot, g_split_item_source,
                                     &gXStatus.held_item_origin, &gXStatus.held_item_slot);
            carried = g_status.item_in_hand.stack_count;
        }
    }
    g_status.item_in_hand.stack_count = carried;
    RebuildEquipmentAndDerivedStatsForSlot(giReviewCharSlot);
    RebuildCampItemList();
    RecalculateCharacterDerivedStats(g_status.buffers.Char + giReviewCharSlot);
    RecalculateCarriedWeight(g_review_character);
    RedistributePartyEncumbrance();
    SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
}

/* Mode/tail bits the update pass keys on: the camp item_action byte at +0xd3f
   selects which use paths are live, and iTargetingMode selects what the item
   cursor is allowed to target. */
// FUNCTION: WIZ8 0x005BAD20
void UpdateItemCursorForState(int flag, W8ItemInstance* item, int slot)
{
    if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_NONE) {
        return;
    }
    switch (gXStatus.iTargetingMode) {
    case W8_TARGET_NEED_ALLY:
        if ((g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_CAST_SPELL ||
             g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_USE_ON_CHARACTER) &&
            g_status.buffers.XChar[slot].fOccupied &&
            g_status.buffers.Char[slot].uiCondition[W8_CONDITION_MISSING] == 0) {
            SetTargetCursor(GetTargetingCursorForState(flag));
            return;
        }
        if (CanPartySlotParticipate(slot)) {
            SetTargetCursor(GetTargetingCursorForState(flag));
        }
        return;
    case W8_TARGET_NEED_ITEM:
        if (item == 0) {
            srAssertFail("pItem", "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp",
                         0x763, 0);
        }
        if (item->iItemNo == -1) {
            return;
        }
        if ((g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_IDENTIFY_SPELL ||
             g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_USE_ON_ITEM) &&
            CanItemLeaveItsSlot(item)) {
            SetTargetCursor(GetTargetingCursorForState(flag));
            return;
        }
        if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_IDENTIFY ||
            g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_DROP) {
            SetTargetCursor(GetTargetingCursorForState(flag));
            return;
        }
        if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_SPLIT_STACK) {
            const W8ItemDatabaseRecord* record = g_item_records + item->iItemNo;
            if ((record->flags & W8_ITEM_FLAG_NO_DISCARD) != 0 ||
                record->quantity_kind != W8_ITEM_QUANTITY_STACK) {
                return;
            }
            if (gXStatus.fCombatMode && !IsEquippableItemClass(item) &&
                (g_combat_state->equip_phase == 0 ||
                 g_status.buffers.XChar[giReviewCharSlot].pending_action != W8_ACTION_EQUIP)) {
                return;
            }
            SetTargetCursor(GetTargetingCursorForState(flag));
            return;
        }
        if (g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_USE) {
            if (CanCharacterActivateItem(g_review_character, item) ||
                CanCastFromItem(g_review_character, item) || IsUsableItemClass(item)) {
                SetTargetCursor(GetTargetingCursorForState(flag));
            }
            return;
        }
        if (g_camp_screen->item_action != W8_CAMP_ITEM_ACTION_MOVE || item->iItemNo == -1) {
            return;
        }
        if (static_cast<char>(flag) == 0) {
            if (!g_status.item_in_cursor) {
                SetTargetCursor(3);
                return;
            }
            SetTargetCursor(0xf);
            SetItemCursor(0xe);
            return;
        }
        if (!g_status.item_in_cursor) {
            SetTargetCursor(4);
            return;
        }
        SetTargetCursor(0xe);
        SetItemCursor(0xf);
        return;
    case W8_TARGET_NEED_CHARACTER_INDIRECT:
        if (IsDeadCharacterTargetable(slot)) {
            SetTargetCursor(GetTargetingCursorForState(flag));
        }
        return;
    default:
        return;
    }
}

// FUNCTION: WIZ8 0x005BAFC0
void SetHandCursors(char mode)
{
    if (mode == 0) {
        if (g_status.item_in_cursor) {
            SetTargetCursor(0xf);
            SetItemCursor(0xe);
            return;
        }
        SetTargetCursor(3);
        return;
    }
    if (g_status.item_in_cursor) {
        SetTargetCursor(0xe);
        SetItemCursor(0xf);
        return;
    }
    SetTargetCursor(4);
}

/* Unequips both weapon slots of the selected character; refused outright in
   combat and while the party is moving, and per-slot when a bound item will
   not come off. */
// FUNCTION: WIZ8 0x005BB010
void UnequipBothHands(void)
{
    W8Character* character;

    if (gXStatus.fCombatMode && !g_combat_state->round_active && !gXStatus.fPartyMovementMode &&
        g_combat_state->equip_phase == 0) {
        ShowCampNoticeLine(gppStringList[0x903], 0, true, false);
        return;
    }
    if (!IsPartySlotEligible(giReviewCharSlot)) {
        ShowCampNoticeLine(gppStringList[0x917], 0, true, false);
        return;
    }
    character = g_status.buffers.Char + giReviewCharSlot;
    BindEquippedItem(character, W8_EQUIP_SLOT_PRIMARY_WEAPON);
    BindEquippedItem(character, W8_EQUIP_SLOT_SECONDARY_WEAPON);
    if (CanUnequipSlotItem(character, W8_EQUIP_SLOT_PRIMARY_WEAPON) &&
        CanUnequipSlotItem(character, W8_EQUIP_SLOT_SECONDARY_WEAPON)) {
        SwapWeaponSetSlots(giReviewCharSlot, false, true);
        g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_EQUIPMENT;
        g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_PORTRAIT;
        g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_CHARACTER_INFO;
        return;
    }
    ShowCampNoticeLine(gppStringList[0x916], 0, true, false);
}

// FUNCTION: WIZ8 0x005BB140
void TogglePartyRowFlag(void)
{
    if ((g_camp_action_buttons[1]->m_stateFlags & g_W8TextControlStateSecondary) != 0) {
        g_status.buffers.XChar[giReviewCharSlot].item_action_pending = true;
        g_camp_action_buttons[0]->SetEnabled(false);
        g_camp_action_buttons[0]->Invalidate(false);
        return;
    }
    g_status.buffers.XChar[giReviewCharSlot].item_action_pending = false;
    g_camp_action_buttons[0]->SetEnabled(true);
    g_camp_action_buttons[0]->Invalidate(false);
}

// FUNCTION: WIZ8 0x005BB1C0
void ToggleCampHandItemFilter(void)
{
    ToggleCampItemFilter(W8_CAMP_ITEM_FILTER_BUTTON_HAND);
}

// FUNCTION: WIZ8 0x005BB1D0
void ToggleCampBodyItemFilter(void)
{
    ToggleCampItemFilter(W8_CAMP_ITEM_FILTER_BUTTON_BODY);
}

// FUNCTION: WIZ8 0x005BB1E0
void ToggleCampOtherItemFilter(void)
{
    ToggleCampItemFilter(W8_CAMP_ITEM_FILTER_BUTTON_OTHER);
}

// FUNCTION: WIZ8 0x005BB1F0
void ToggleCampAccessoryItemFilter(void)
{
    ToggleCampItemFilter(W8_CAMP_ITEM_FILTER_BUTTON_ACCESSORY);
}

// FUNCTION: WIZ8 0x005BB200
void ToggleCampUsabilityFilter(void)
{
    ToggleCampItemFilter(W8_CAMP_ITEM_FILTER_BUTTON_USABLE);
}

// FUNCTION: WIZ8 0x005BB210
void ToggleCampUnidentifiedItemFilter(void)
{
    ToggleCampItemFilter(W8_CAMP_ITEM_FILTER_BUTTON_UNIDENTIFIED);
}

// FUNCTION: WIZ8 0x005BB220
void SortCampItemPool(void)
{
    SortPartyItemPool();
    g_camp_screen->item_scroll = 0;
    RebuildCampItemList();
    g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_POOL;
}

/* UI button order differs from the item-filter order. */
// FUNCTION: WIZ8 0x005BB250
void ToggleCampItemFilter(W8CampItemFilterButton tab)
{
    W8CampItemFilter filter;
    int index;

    switch (tab) {
    case W8_CAMP_ITEM_FILTER_BUTTON_HAND:
        filter = W8_CAMP_ITEM_FILTER_HAND;
        break;
    case W8_CAMP_ITEM_FILTER_BUTTON_BODY:
        filter = W8_CAMP_ITEM_FILTER_BODY;
        break;
    case W8_CAMP_ITEM_FILTER_BUTTON_OTHER:
        filter = W8_CAMP_ITEM_FILTER_OTHER;
        break;
    case W8_CAMP_ITEM_FILTER_BUTTON_ACCESSORY:
        filter = W8_CAMP_ITEM_FILTER_ACCESSORY;
        break;
    case W8_CAMP_ITEM_FILTER_BUTTON_USABLE:
        filter = W8_CAMP_ITEM_FILTER_USABLE;
        break;
    case W8_CAMP_ITEM_FILTER_BUTTON_UNIDENTIFIED:
        filter = W8_CAMP_ITEM_FILTER_UNIDENTIFIED;
        break;
    default:
        filter = static_cast<W8CampItemFilter>(tab);
    }
    if ((g_camp_item_filter_buttons[tab]->m_stateFlags & g_W8TextControlStateSecondary) == 0) {
        g_camp_screen->item_filters[filter] = 0;
    } else {
        g_camp_screen->item_filters[filter] = 1;
        if (filter != W8_CAMP_ITEM_FILTER_USABLE && filter != W8_CAMP_ITEM_FILTER_UNIDENTIFIED) {
            ClearOtherCampItemGroupFilters(filter);
            for (index = 0; index < 4; ++index) {
                if (index != tab && (g_camp_item_filter_buttons[index]->m_stateFlags &
                                     g_W8TextControlStateSecondary) != 0) {
                    g_camp_item_filter_buttons[index]->DisableSecondaryState(true);
                }
            }
        }
    }
    g_camp_screen->item_scroll = 0;
    RebuildCampItemList();
    if (!gfKeyState[VK_CONTROL]) {
        g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_POOL;
        return;
    }
    g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
}

/* Region reason values are bit codes set by the region manager: 0x8 left
   release, 0x10 left down, 0x40 right press, 0x80 right release, 0x100
   double-click/activate, 0x400 enter/leave transition, 0x800 mouse wheel. */
/* Descriptive name for the right-click identify/show operation shared by
   backpack, equipment and party-pool slots. */
static void ShowCampItemInfo(W8ItemInstance* item)
{
    g_camp_identifying_character = g_review_character;
    if (CanItemLeaveItsSlot(item) && PartyAttemptsToIdentifyItem(item, 0) &&
        g_camp_screen->item_filters[W8_CAMP_ITEM_FILTER_UNIDENTIFIED] != 0) {
        RebuildCampItemList();
        g_camp_screen->redraw_flags |= W8_CAMP_REDRAW_ALL;
    }
    OpenItemInfoDialog(item, 0);
    SetCampItemActionMode(W8_CAMP_ITEM_ACTION_NONE);
}

// FUNCTION: WIZ8 0x005BB350
unsigned char BackpackRegionHandler(const InputAtom* event, W8Region* region)
{
    int slot;
    W8ItemInstance* item;

    slot = region->callback_id;
    item = g_review_character->backpack + slot;
    if (item->iItemNo == -1 && !g_status.item_in_cursor) {
        PushButtonSoundScheme(0, true);
    }
    if (event->usEvent < (RIGHT_BUTTON_DOWN + 1)) {
        if (event->usEvent == RIGHT_BUTTON_DOWN) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (event->usEvent == LEFT_BUTTON_DOWN) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        } else {
            if (event->usEvent != LEFT_BUTTON_UP) {
                return 0;
            }
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
                return 1;
            }
            HandleCampItemClick(item, slot, W8_ITEM_ORIGIN_BACKPACK);
            if (item->iItemNo != -1) {
                SetItemTooltip(item, region);
                SetRegionHelpForceEnabled(true);
                return 1;
            }
        }
        DisableRegionHelpFlag(region);
        return 1;
    }
    if (event->usEvent == RIGHT_BUTTON_UP) {
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && item->iItemNo != -1 &&
            g_camp_screen->item_action != W8_CAMP_ITEM_ACTION_IDENTIFY) {
            ShowCampItemInfo(item);
        }
        return 1;
    }
    if (event->usEvent == MOUSE_POS) {
        if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
            if (item->iItemNo != -1 || g_status.item_in_cursor) {
                g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_BACKPACK_CELL_FIRST
                                                    << (slot & 0x1f);
            }
            UpdateItemCursorForState((region->flags & W8_REGION_MOUSE_ENTER) != 0, item, 0);
            if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
                if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
                    return 0;
                }
            } else if (item->iItemNo != -1) {
                SetItemTooltip(item, region);
                SetRegionHelpForceEnabled(true);
                return 0;
            }
            DisableRegionHelpFlag(region);
        }
        return 0;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005BB560
unsigned char EquipSlotRegionHandler(const InputAtom* event, W8Region* region)
{
    int slot;
    int delay;
    W8ItemInstance* item;
    wchar_t* name;

    slot = region->callback_id;
    item = g_review_character->EquippedItem + slot;
    if (item->iItemNo == -1 &&
        (!g_status.item_in_cursor || g_camp_screen->item_action == W8_CAMP_ITEM_ACTION_MOVE ||
         !CanEquipItemInSlot(g_review_character, g_status.item_in_hand.iItemNo,
                             static_cast<unsigned char>(slot), true) ||
         !CanCharacterUseItem(g_review_character, g_status.item_in_hand.iItemNo))) {
        PushButtonSoundScheme(0, true);
    }
    if (event->usEvent < (RIGHT_BUTTON_DOWN + 1)) {
        if (event->usEvent == RIGHT_BUTTON_DOWN) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (event->usEvent == LEFT_BUTTON_DOWN) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            DisableRegionHelpFlag(region);
            return 1;
        }
        if (event->usEvent != LEFT_BUTTON_UP) {
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            HandleCampItemClick(item, slot, W8_ITEM_ORIGIN_EQUIPPED);
            delay = g_settings.tooltip_delay_ms;
            if (static_cast<unsigned int>(delay) > 300) {
                delay = 300;
            }
            SetRegionHelpDelay(delay);
            SetRegionHelpForceEnabled(true);
            EnableRegionHelpFlag(region);
            if (item->iItemNo != -1) {
                name = FormatItemDisplayName(item, false);
                swprintf(g_camp_screen->text_buffer, L"%s (%s)", name,
                         gppStringList[g_equip_slot_label_ids[slot]]);
                SetRegionHelpText(g_camp_screen->text_buffer);
                return 1;
            }
            swprintf(g_camp_screen->text_buffer, g_format_s,
                     gppStringList[g_equip_slot_label_ids[slot]]);
            SetRegionHelpText(g_camp_screen->text_buffer);
            return 1;
        }
    } else {
        if (event->usEvent != RIGHT_BUTTON_UP) {
            if (event->usEvent != MOUSE_POS) {
                return 0;
            }
            if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                if (item->iItemNo != -1 || g_status.item_in_cursor) {
                    g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_EQUIPMENT_CELL_FIRST
                                                        << (slot & 0x1f);
                }
                UpdateItemCursorForState(1, item, 0);
                EnableRegionHelpFlag(region);
                delay = g_settings.tooltip_delay_ms;
                if (static_cast<unsigned int>(delay) > 300) {
                    delay = 300;
                }
                SetRegionHelpDelay(delay);
                SetRegionHelpForceEnabled(true);
                if (item->iItemNo == -1) {
                    swprintf(g_camp_screen->text_buffer, g_format_s,
                             gppStringList[g_equip_slot_label_ids[slot]]);
                } else {
                    name = FormatItemDisplayName(item, false);
                    swprintf(g_camp_screen->text_buffer, L"%s (%s)", name,
                             gppStringList[g_equip_slot_label_ids[slot]]);
                }
                SetRegionHelpText(g_camp_screen->text_buffer);
            }
            if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
                if (item->iItemNo != -1 || g_status.item_in_cursor) {
                    g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_EQUIPMENT_CELL_FIRST
                                                        << (slot & 0x1f);
                }
                UpdateItemCursorForState(0, item, 0);
                DisableRegionHelpFlag(region);
            }
            return 0;
        }
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && item->iItemNo != -1 &&
            g_camp_screen->item_action != W8_CAMP_ITEM_ACTION_IDENTIFY) {
            ShowCampItemInfo(item);
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x005BB900
unsigned char ItemPoolRegionHandler(const InputAtom* event, W8Region* region)
{
    unsigned int slot;
    unsigned int pool_index;
    int delta;
    int count;
    W8ItemInstance* item;

    slot = region->callback_id;
    pool_index = g_camp_screen->item_scroll + slot;
    if (g_camp_screen->item_list_count <= pool_index ||
        g_status.party_item_pool[g_camp_screen->item_list[pool_index]].iItemNo == -1) {
        pool_index = g_status.party_item_count;
    } else {
        pool_index = g_camp_screen->item_list[pool_index];
    }
    item = g_status.party_item_pool + pool_index;
    if (item->iItemNo == -1 && !g_status.item_in_cursor) {
        PushButtonSoundScheme(0, true);
    }
    if (event->usEvent < 0x101) {
        if (event->usEvent == RIGHT_BUTTON_UP) {
            if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 &&
                pool_index < g_status.party_item_count &&
                g_camp_screen->item_action != W8_CAMP_ITEM_ACTION_IDENTIFY) {
                ShowCampItemInfo(item);
            }
            return 1;
        }
        if (event->usEvent == LEFT_BUTTON_DOWN) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            DisableRegionHelpFlag(region);
            return 1;
        }
        if (event->usEvent != LEFT_BUTTON_UP) {
            if (event->usEvent != RIGHT_BUTTON_DOWN) {
                return 0;
            }
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        HandleCampItemClick(item, pool_index, W8_ITEM_ORIGIN_PARTY_POOL);
        if (item->iItemNo != -1) {
            SetItemTooltip(item, region);
            return 1;
        }
    } else {
        if (event->usEvent == MOUSE_POS) {
            if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
                if (item->iItemNo != -1 || g_status.item_in_cursor) {
                    g_camp_screen->item_redraw_flags |= W8_CAMP_ITEM_REDRAW_POOL_CELL_FIRST
                                                        << (slot & 0x1f);
                }
                UpdateItemCursorForState((region->flags & W8_REGION_MOUSE_ENTER) != 0, item, 0);
                if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
                    if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
                        return 0;
                    }
                } else if (item->iItemNo != -1) {
                    SetItemTooltip(item, region);
                    return 0;
                }
                DisableRegionHelpFlag(region);
            }
            return 0;
        }
        if (event->usEvent != MOUSE_WHEEL) {
            return 0;
        }
        delta = GetMouseWheelDeltaValue(event->usParam);
        count = delta;
        if (delta > 0) {
            do {
                g_camp_screen->item_range->m_range->Decrement();
                --count;
            } while (count != 0);
            delta = 0;
        }
        if (delta < 0) {
            count = -delta;
            do {
                g_camp_screen->item_range->m_range->Increment();
                --count;
            } while (count != 0);
        }
        pool_index = g_camp_screen->item_scroll + slot;
        if (g_camp_screen->item_list_count <= pool_index) {
            pool_index = g_status.party_item_count;
        } else {
            pool_index = g_camp_screen->item_list[pool_index];
        }
        if (g_status.party_item_pool[pool_index].iItemNo != -1) {
            SetItemTooltip(g_status.party_item_pool + pool_index, region);
            return 1;
        }
    }
    DisableRegionHelpFlag(region);
    return 1;
}

/* Reason 0x40/0x8 press the tab control, 0x10 releases it, and the 0x400
   enter/leave transition forwards the matching mouse events. */
// FUNCTION: WIZ8 0x005BBBB0
unsigned char RealmTabRegionHandler(const InputAtom* event, W8Region* region)
{
    return DispatchButtonRegionEvent(event, region,
                                     g_camp_item_filter_buttons[region->callback_id]);
}

// FUNCTION: WIZ8 0x005BBC70
unsigned char PanelTabRegionHandler(const InputAtom* event, W8Region* region)
{
    return DispatchButtonRegionEvent(event, region, g_camp_action_buttons[region->callback_id]);
}

// FUNCTION: WIZ8 0x005BBD30
void SetItemTooltip(W8ItemInstance* item, W8Region* region)
{
    int delay;
    wchar_t* name;
    const W8ItemDatabaseRecord* record;

    delay = g_settings.tooltip_delay_ms;
    if (static_cast<unsigned int>(delay) > 300) {
        delay = 300;
    }
    SetRegionHelpDelay(delay);
    SetRegionHelpForceEnabled(true);
    EnableRegionHelpFlag(region);
    name = FormatItemDisplayName(item, false);
    wcscpy(g_camp_screen->text_buffer, name);
    record = g_item_records + item->iItemNo;
    if (record->category == W8_ITEM_CATEGORY_SPELL_SOURCE) {
        if (record->spell_id == W8_SPELL_NONE) {
            srAssertFail("ubSpell != SPELL_NONE",
                         "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0xa7a, 0);
        }
        if (g_review_character->spell_learned[record->spell_id] == 1 &&
            (item->identified || item->spell_hint)) {
            wcscat(g_camp_screen->text_buffer, L" (");
            wcscat(g_camp_screen->text_buffer, gppStringList[0x930]);
            wcscat(g_camp_screen->text_buffer, L")");
        }
    }
    SetRegionHelpText(g_camp_screen->text_buffer);
}

/* The items page's per-row label boxes: the backpack grid (two columns of
   four at 0x0d/0xc2), the visible pool rows (0x22c/0xc2), and the twelve
   equipment icons laid out by g_camp_screen_regions. Each occupied
   slot gets a hover label drawn by DrawCampItemLabel. */
// FUNCTION: WIZ8 0x005bbe30
void DrawCampItemIcons(void)
{
    unsigned int index;
    unsigned int count;
    int row;
    int y;
    const W8CampScreenRegion* region;
    W8ItemInstance* item;
    W8Character* character = g_review_character;
    W8CampScreenState* state = g_camp_screen;

    for (index = 0; index < 8; ++index) {
        item = &character->backpack[index];
        if (item->iItemNo != -1) {
            DrawCampItemLabel(item, (index & 1) * 0x31 + 0xd,
                              (index >> 1) * 0x39 + 0xc2 + (index & 1) * 0x10, 0);
        }
    }
    index = 0;
    while (true) {
        count = state->item_list_count - state->item_scroll;
        if (count > 8) {
            count = 8;
        }
        if (count <= index) {
            break;
        }
        y = (index >> 1) * 0x39 + 0xc2;
        if ((index & 1) == 0) {
            y = (index >> 1) * 0x39 + 0xd2;
        }
        DrawCampItemLabel(&g_status.party_item_pool[state->item_list[state->item_scroll + index]],
                          (index & 1) * 0x31 + 0x22c, y, 1);
        ++index;
    }
    region = g_camp_screen_regions;
    for (index = 0; index < 12; ++index) {
        item = &character->EquippedItem[index];
        if (item->iItemNo != -1) {
            DrawCampItemLabel(item, region->label_x, region->label_y,
                              static_cast<char>(region->label_flag));
        }
        ++region;
    }
    ResetTransientRenderScenes();
}

/* Draws one boxed item name: the buffer measures the formatted display name,
   the box is left-shifted by its width for the pool column, then the filled
   background, the text and a one-pixel outline are painted straight into the
   locked primary surface. */
// FUNCTION: WIZ8 0x005bbf40
static void DrawCampItemLabel(W8ItemInstance* item, int left, int top, char flag)
{
    wchar_t* name;
    W8ControlsRect bounds;
    W8TextBuffer* text;
    int width;
    int height;
    unsigned int pitch;
    char* buffer;

    name = FormatItemDisplayName(item, false);
    wcscpy(g_camp_screen->text_buffer, name);
    bounds.left = left;
    bounds.top = top;
    bounds.right = left + 0xfa;
    bounds.bottom = top + 0xfa;
    text = new W8TextBuffer(&bounds, g_camp_screen->text_buffer, g_font10arial,
                            g_W8TextBufferAlignTop | g_W8TextBufferAlignLeft, 4);
    if (text != 0) {
        height = text->m_lineCount;
        width = text->m_maxLineWidth + 4;
        height = GetFontHeight(g_font10arial) * height + 2;
        if (flag != 0) {
            bounds.right -= width;
            bounds.left -= width;
            text->SetLayoutBounds(&bounds, true, true);
        }
        ColorFillVideoSurfaceArea(0xfffffff2, bounds.left, bounds.top, bounds.left + width,
                                  bounds.top + height, 0x8000);
        buffer = static_cast<char*>(LockPrimarySurface(&pitch));
        if (buffer != 0) {
            // reinterpret-ok: SGP frame-buffer bytes to W8TextBuffer's byte view
            text->RenderText(reinterpret_cast<unsigned char*>(buffer), pitch, 2, 1, true);
            LineDraw(0, bounds.left, bounds.top, bounds.left + width, bounds.top, -0x6613, buffer);
            LineDraw(0, bounds.left + width, bounds.top, bounds.left + width, bounds.top + height,
                     -0x6613, buffer);
            LineDraw(0, bounds.left + width, bounds.top + height, bounds.left, bounds.top + height,
                     -0x6613, buffer);
            LineDraw(0, bounds.left, bounds.top + height, bounds.left, bounds.top, -0x6613, buffer);
            UnlockPrimarySurface();
            delete text;
        }
    }
}

/* The camp screen's clickable labels: the seven attribute rows and the four
   secondary value labels on the character-info panel are plain text controls
   that additionally play the button sound on entry and on left presses. The
   left-button handlers compile identically to W8HelpTextControl's and fold to
   0x005B7CB0/0x005B7CD0. */
// VTABLE: WIZ8 0x005ef2b0
class W8CampInfoLabel : public W8TextControl {
public:
    W8CampInfoLabel(Controls* panel, unsigned int region, int left, int top, int right, int bottom,
                    int image_object, int image_frame, int normal_sprite, int pressed_sprite,
                    int alternate_normal_sprite, int alternate_pressed_sprite, int disabled_sprite)
        : W8TextControl(panel, region, left, top, right, bottom, image_object, image_frame,
                        normal_sprite, pressed_sprite, alternate_normal_sprite,
                        alternate_pressed_sprite, disabled_sprite)
    {
    }

    virtual void OnMouseEnter(int event) override;
    virtual void OnLeftButtonDown(int event) override;
    virtual void OnLeftButtonUp(int event) override;
    virtual void OnLeftButtonDoubleClick(int event) override;
};

/* Identical body to W8HelpTextControl::OnLeftButtonDown; ICF folds it to
   0x005B7CB0. */
void W8CampInfoLabel::OnLeftButtonDown(int event)
{
    PushButtonSoundScheme(0, true);
    W8TextControl::OnLeftButtonDown(event);
}

/* Identical body to W8HelpTextControl::OnLeftButtonUp; ICF folds it to
   0x005B7CD0. */
void W8CampInfoLabel::OnLeftButtonUp(int event)
{
    PushButtonSoundScheme(0, true);
    W8TextControl::OnLeftButtonUp(event);
}

// FUNCTION: WIZ8 0x005b7c90
void W8CampInfoLabel::OnMouseEnter(int event)
{
    PushButtonSoundScheme(0, true);
    W8TextControl::OnMouseEnter(event);
}

// FUNCTION: WIZ8 0x005b7cf0
void W8CampInfoLabel::OnLeftButtonDoubleClick(int event)
{
    PushButtonSoundScheme(0, true);
    W8TextControl::OnLeftButtonDoubleClick(event);
}

/* The bottom-left panel and its two buttons: drop the held equipment and
   toggle the character's party-row flag. */
// FUNCTION: WIZ8 0x005b9070
int CreateCampActionPanel(void)
{
    int index;

    g_camp_action_panel = 0;
    for (index = 0; index < 2; ++index) {
        if (g_camp_action_buttons[index] != 0) {
            g_camp_action_buttons[index] = 0;
        }
    }
    g_camp_action_panel = new Controls(0x78, 0x19c, 0xbf, 0x1b4, -1, 0, 0);
    if (g_camp_action_panel == 0) {
        return 0;
    }
    g_camp_action_buttons[0] =
        new W8TextControl(g_camp_action_panel, 0x117, 5, 0, 0x21, 0x18, 0x11e, 0, 0, 2, 1, 2, 3);
    g_camp_action_buttons[1] = new W8TextControl(g_camp_action_panel, 0x118, 0x26, 0, 0x42, 0x18,
                                                 0x11e, 0, 8, 4, 9, 5, 0xb);
    index = 0;
    while (g_camp_action_buttons[index] != 0) {
        ++index;
        if (index > 1) {
            g_camp_action_buttons[1]->AddLayoutFlags(g_W8TextControlLayoutToggle);
            g_camp_action_buttons[0]->m_primaryActivationCallback = UnequipBothHands;
            g_camp_action_buttons[1]->m_primaryActivationCallback = TogglePartyRowFlag;
            g_camp_action_panel->SetEnabled(true);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005B9220
void ReleaseCampActionPanel(void)
{
    DestroyControlPanel(g_camp_action_panel);
    DestroyTextControls(g_camp_action_buttons, 2);
}

// FUNCTION: WIZ8 0x005b9270
void EnableCampActionButtons(void)
{
    g_camp_action_buttons[0]->SetActive(true);
    g_camp_action_buttons[1]->SetActive(true);
    if (g_status.game_started) {
        g_camp_action_buttons[1]->SetEnabled(true);
        if (g_status.buffers.XChar[giReviewCharSlot].item_action_pending) {
            g_camp_action_buttons[1]->EnableSecondaryState(false);
            g_camp_action_buttons[0]->SetEnabled(false);
        } else {
            g_camp_action_buttons[1]->DisableSecondaryState(false);
            g_camp_action_buttons[0]->SetEnabled(true);
        }
    } else {
        g_camp_action_buttons[0]->SetEnabled(false);
        g_camp_action_buttons[1]->SetEnabled(false);
    }
}

// FUNCTION: WIZ8 0x005b9310
void DisableCampActionButtons(void)
{
    g_camp_action_buttons[0]->SetActive(false);
    g_camp_action_buttons[1]->SetActive(false);
}

// FUNCTION: WIZ8 0x005b9330
void RefreshCampActionPanel(bool invalidate)
{
    if (invalidate) {
        g_camp_action_panel->Invalidate(0);
    }
    g_camp_action_panel->Redraw();
}

/* The right-hand panel of seven tabs: the six realm filters and the pool sort
   button. */
// FUNCTION: WIZ8 0x005b9350
int CreateItemsTabPanel(void)
{
    int index;

    g_camp_item_filter_panel = 0;
    for (index = 0; index < 7; ++index) {
        if (g_camp_item_filter_buttons[index] != 0) {
            g_camp_item_filter_buttons[index] = 0;
        }
    }
    g_camp_item_filter_panel = new Controls(0x1e5, 0xc1, 0x1fd, 0x189, -1, 0, 0);
    if (g_camp_item_filter_panel == 0) {
        return 0;
    }
    g_camp_item_filter_buttons[0] = new W8TextControl(g_camp_item_filter_panel, 0x110, 0, 0, 0x18,
                                                      0x18, 0x11f, 0, 0, 2, 1, 4, 3);
    g_camp_item_filter_buttons[1] =
        new W8TextControl(g_camp_item_filter_panel, 0x111, 0, 0x19, 0x18, 0x31, 0x11f, 0, 0xf, 0x11,
                          0x10, 0x13, 0x12);
    g_camp_item_filter_buttons[2] =
        new W8TextControl(g_camp_item_filter_panel, 0x112, 0, 0x4b, 0x18, 0x63, 0x11f, 0, 0x14,
                          0x16, 0x15, 0x18, 0x17);
    g_camp_item_filter_buttons[3] = new W8TextControl(g_camp_item_filter_panel, 0x113, 0, 0x32,
                                                      0x18, 0x4a, 0x11f, 0, 5, 7, 6, 9, 8);
    g_camp_item_filter_buttons[4] = new W8TextControl(g_camp_item_filter_panel, 0x114, 0, 0x6c,
                                                      0x18, 0x84, 0x11f, 0, 10, 0xc, 0xb, 0xe, 0xd);
    g_camp_item_filter_buttons[5] =
        new W8TextControl(g_camp_item_filter_panel, 0x115, 0, 0x8e, 0x18, 0xa6, 0x11f, 0, 0x28,
                          0x2a, 0x29, 0x2c, 0x2b);
    g_camp_item_filter_buttons[6] =
        new W8TextControl(g_camp_item_filter_panel, 0x116, 0, 0xb0, 0x18, 0xc8, 0x11f, 0, 0x1e,
                          0x20, 0x1f, 0x22, 0x21);
    index = 0;
    while (g_camp_item_filter_buttons[index] != 0) {
        ++index;
        if (index > 6) {
            for (index = 0; index < 6; ++index) {
                g_camp_item_filter_buttons[index]->AddLayoutFlags(g_W8TextControlLayoutToggle);
            }
            g_camp_item_filter_buttons[0]->m_primaryActivationCallback = ToggleCampHandItemFilter;
            g_camp_item_filter_buttons[1]->m_primaryActivationCallback = ToggleCampBodyItemFilter;
            g_camp_item_filter_buttons[2]->m_primaryActivationCallback = ToggleCampOtherItemFilter;
            g_camp_item_filter_buttons[3]->m_primaryActivationCallback =
                ToggleCampAccessoryItemFilter;
            g_camp_item_filter_buttons[4]->m_primaryActivationCallback = ToggleCampUsabilityFilter;
            g_camp_item_filter_buttons[5]->m_primaryActivationCallback =
                ToggleCampUnidentifiedItemFilter;
            g_camp_item_filter_buttons[6]->m_primaryActivationCallback = SortCampItemPool;
            g_camp_item_filter_panel->SetEnabled(true);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005B9760
void ReleaseItemsTabPanel(void)
{
    DestroyControlPanel(g_camp_item_filter_panel);
    DestroyTextControls(g_camp_item_filter_buttons, 7);
}

/* Item filter buttons keep their secondary state when the corresponding
   filter is set. The sort button has no item-filter flag. */
// FUNCTION: WIZ8 0x005b97b0
void UpdateCampItemFilters(void)
{
    int index;
    unsigned char flag;

    for (index = 0; index < 7; ++index) {
        g_camp_item_filter_buttons[index]->SetActive(true);
    }
    if (!g_status.game_started) {
        for (index = 0; index < 7; ++index) {
            g_camp_item_filter_buttons[index]->SetEnabled(false);
        }
        return;
    }
    for (index = 0; index < 7; ++index) {
        g_camp_item_filter_buttons[index]->SetEnabled(true);
        switch (index) {
        case 0:
            flag = g_camp_screen->item_filters[W8_CAMP_ITEM_FILTER_HAND];
            break;
        case 1:
            flag = g_camp_screen->item_filters[W8_CAMP_ITEM_FILTER_BODY];
            break;
        case 2:
            flag = g_camp_screen->item_filters[W8_CAMP_ITEM_FILTER_OTHER];
            break;
        case 3:
            flag = g_camp_screen->item_filters[W8_CAMP_ITEM_FILTER_ACCESSORY];
            break;
        case 4:
            flag = g_camp_screen->item_filters[W8_CAMP_ITEM_FILTER_USABLE];
            break;
        case 5:
            flag = g_camp_screen->item_filters[W8_CAMP_ITEM_FILTER_UNIDENTIFIED];
            break;
        default:
            continue;
        }
        if (flag != 0) {
            g_camp_item_filter_buttons[index]->EnableSecondaryState(false);
        }
    }
}

// FUNCTION: WIZ8 0x005b98c0
void DisableItemsRealmTabs(void)
{
    int index;

    for (index = 0; index < 7; ++index) {
        g_camp_item_filter_buttons[index]->SetActive(false);
    }
}

// FUNCTION: WIZ8 0x005b98e0
void RefreshItemsTabPanel(bool invalidate)
{
    if (invalidate) {
        g_camp_item_filter_panel->Invalidate(0);
    }
    g_camp_item_filter_panel->Redraw();
}

/* The top secondary panel: the Items/Character info page tabs, the help line,
   the seven attribute labels and the four secondary value labels. The labels
   are W8CampInfoLabel controls created with absolute coordinates
   relative to the panel origin. */
// FUNCTION: WIZ8 0x005b9900
int CreateCampSecondaryPanel(void)
{
    Controls* panel;
    int index;
    int left;
    int right;
    int top;

    g_camp_secondary_panel = 0;
    for (index = 0; index < 2; ++index) {
        if (g_camp_page_tabs[index] != 0) {
            g_camp_page_tabs[index] = 0;
        }
    }
    g_camp_help_text = 0;
    for (index = 0; index < 7; ++index) {
        if (g_camp_stat_labels[index] != 0) {
            g_camp_stat_labels[index] = 0;
        }
    }
    for (index = 0; index < 4; ++index) {
        if (g_camp_info_labels[index] != 0) {
            g_camp_info_labels[index] = 0;
        }
    }
    panel = new Controls(0x13c, 7, 0x154, 0x34, 0x120, 0, 0);
    g_camp_secondary_panel = panel;
    if (panel == 0) {
        return 0;
    }
    panel->AcquireRegionSet(&g_camp_secondary_region_set);
    g_camp_page_tabs[0] = new W8TextControl(panel, -1, 2, 2, 0x16, 0x16, 0x121, 0, 0, 2, 1, 4, 3);
    g_camp_page_tabs[1] =
        new W8TextControl(panel, -1, 2, 0x17, 0x16, 0x2b, 0x121, 0, 5, 7, 6, 9, 8);
    for (index = 0; index < 2; ++index) {
        if (g_camp_page_tabs[index] == 0) {
            return 0;
        }
    }
    g_camp_help_text = new W8HelpTextControl(panel, -1, 5, 0x4d, 0x7c, 0x59);
    if (g_camp_help_text == 0) {
        return 0;
    }
    left = 0x1c2 - panel->m_bounds.left;
    right = 0x20e - panel->m_bounds.left;
    top = 0x3a - panel->m_bounds.top;
    for (index = 0; index < 7; ++index) {
        g_camp_stat_labels[index] =
            new W8CampInfoLabel(panel, -1, left, top, right, top + 0xc, -1, -1, -1, -1, -1, -1, -1);
        g_camp_stat_labels[index]->EnableRegionHelp(0x958);
        top += 0xe;
    }
    left = 0x144 - panel->m_bounds.left;
    right = 0x1b9 - panel->m_bounds.left;
    top = -panel->m_bounds.top;
    g_camp_info_labels[0] = new W8CampInfoLabel(panel, -1, left, top + 0x3a, right, top + 0x46, -1,
                                                -1, -1, -1, -1, -1, -1);
    g_camp_info_labels[1] = new W8CampInfoLabel(panel, -1, left, top + 0x48, right, top + 0x54, -1,
                                                -1, -1, -1, -1, -1, -1);
    g_camp_info_labels[2] = new W8CampInfoLabel(panel, -1, left, top + 0x80, right, top + 0x8c, -1,
                                                -1, -1, -1, -1, -1, -1);
    g_camp_info_labels[3] = new W8CampInfoLabel(panel, -1, left, top + 0x8e, right, top + 0x9a, -1,
                                                -1, -1, -1, -1, -1, -1);
    g_camp_stat_labels[0]->m_secondaryActivationCallback = OpenStrengthInfoDialog;
    g_camp_stat_labels[1]->m_secondaryActivationCallback = OpenIntelligenceInfoDialog;
    g_camp_stat_labels[2]->m_secondaryActivationCallback = OpenPietyInfoDialog;
    g_camp_stat_labels[3]->m_secondaryActivationCallback = OpenVitalityInfoDialog;
    g_camp_stat_labels[4]->m_secondaryActivationCallback = OpenDexterityInfoDialog;
    g_camp_stat_labels[5]->m_secondaryActivationCallback = OpenSpeedInfoDialog;
    g_camp_stat_labels[6]->m_secondaryActivationCallback = OpenSensesInfoDialog;
    g_camp_info_labels[0]->m_secondaryActivationCallback = OpenSecondaryAttributeInfoDialog0;
    g_camp_info_labels[1]->m_secondaryActivationCallback = OpenSecondaryAttributeInfoDialog1;
    g_camp_info_labels[2]->m_secondaryActivationCallback = OpenSecondaryAttributeInfoDialog3;
    g_camp_info_labels[3]->m_secondaryActivationCallback = OpenSecondaryAttributeInfoDialog3;
    for (index = 0; index < 4; ++index) {
        g_camp_info_labels[index]->EnableRegionHelp(0x958);
    }
    g_camp_help_text->m_secondaryActivationCallback = OpenSecondaryAttributeInfoDialog4;
    g_camp_page_tabs[0]->AddLayoutFlags(g_W8TextControlLayoutToggle);
    g_camp_page_tabs[1]->AddLayoutFlags(g_W8TextControlLayoutToggle);
    g_camp_page_tabs[0]->m_primaryActivationCallback = ShowCampItemsPage;
    g_camp_page_tabs[1]->m_primaryActivationCallback = ShowCampCharacterPage;
    g_camp_page_tabs[0]->EnableRegionHelp(0x95d);
    g_camp_page_tabs[1]->EnableRegionHelp(0x95e);
    panel->SetEnabled(true);
    return 1;
}

// FUNCTION: WIZ8 0x005B9EA0
void ReleaseCampSecondaryPanel(void)
{
    DestroyControlPanel(g_camp_secondary_panel);
    DestroyTextControls(g_camp_page_tabs, 2);
}

// FUNCTION: WIZ8 0x005b9ef0
void InvalidateCampPanel(void)
{
    g_camp_secondary_panel->Invalidate(0);
}

// FUNCTION: WIZ8 0x005b9f00
void EnableCampSecondaryPanel(void)
{
    int index;

    g_camp_secondary_panel->EnableRegionSet(true);
    for (index = 0; index < 2; ++index) {
        g_camp_page_tabs[index]->SetActive(true);
        g_camp_page_tabs[index]->SetEnabled(true);
    }
    if (g_camp_screen->info_page == W8_CAMP_INFO_PAGE_ITEMS) {
        SetCampInfoPageMode(W8_CAMP_INFO_PAGE_ITEMS);
        return;
    }
    SetCampInfoPageMode(W8_CAMP_INFO_PAGE_CHARACTER);
}

// FUNCTION: WIZ8 0x005b9f60
void DisableCampSecondaryPanel(void)
{
    int index;

    g_camp_secondary_panel->EnableRegionSet(false);
    for (index = 0; index < 2; ++index) {
        g_camp_page_tabs[index]->SetActive(false);
    }
}

// FUNCTION: WIZ8 0x005b9f90
void RefreshCampSecondaryPanel(bool invalidate)
{
    if (invalidate) {
        InvalidateCampPanel();
    }
    g_camp_secondary_panel->Redraw();
}
