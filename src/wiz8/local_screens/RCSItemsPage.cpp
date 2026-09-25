#include "wiz8/local_screens/RCSItemsPage.h"

#include "wiz8/layouts/character.h"
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

// GLOBAL: WIZ8 0x005EFB44
int g_split_result_kind = 1;
// GLOBAL: WIZ8 0x005EFB4C
int g_split_dialog_x = 0xa1;
// GLOBAL: WIZ8 0x005EFB50
int g_split_dialog_y = 0x94;
// GLOBAL: WIZ8 0x005EFB64
#pragma bss_seg(".data")
int g_split_dialog_kind = 0;
#pragma bss_seg()
// GLOBAL: WIZ8 0x005EF958
int g_info_dialog_x = 0x80;
// GLOBAL: WIZ8 0x005EF95C
int g_info_dialog_y = 0x61;
// GLOBAL: WIZ8 0x005EE65C
int g_character_event_kind_005ee65c = 0x35;

/* 0x0061E7C4: the twelve equip-slot label message ids indexed by region
   callback_id. */
// GLOBAL: WIZ8 0x0061E7C4
const unsigned short g_equip_slot_label_ids[12] = {
    0x433, 0x434, 0x435, 0x436, 0x437, 0x438, 0x439, 0x43a, 0x43b, 0x43c, 0x43d, 0x43e,
};

/* The two page-tab primary callbacks: the Items and Character info buttons
   created by CreateCampSecondaryPanel. */
// FUNCTION: WIZ8 0x005B9FB0
void SetItemPageMode005B9FB0(void)
{
    SetItemPageMode005B9FD0(0);
}

// FUNCTION: WIZ8 0x005B9FC0
void SetItemPageMode005B9FC0(void)
{
    SetItemPageMode005B9FD0(1);
}

/* The items page swaps two control panels in and out: mode zero shows the
   item page, mode one the character page. */
// FUNCTION: WIZ8 0x005B9FD0
void SetItemPageMode005B9FD0(char mode)
{
    int index;

    switch (mode) {
    default:
        srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x544,
                     "Info toggle error");
        break;
    case 1:
        g_camp_page_tabs[0]->DisableSecondaryState(1);
        g_camp_page_tabs[1]->EnableSecondaryState(1);
        g_camp_help_text->SetActive(false);
        g_camp_screen->character_info->SetEnabled(1);
        for (index = 0; index < 7; ++index) {
            g_camp_stat_labels[index]->SetActive(false);
        }
        for (index = 0; index < 4; ++index) {
            g_camp_info_labels[index]->SetActive(false);
        }
        break;
    case 0:
        g_camp_page_tabs[0]->EnableSecondaryState(1);
        g_camp_page_tabs[1]->DisableSecondaryState(1);
        g_camp_help_text->SetActive(true);
        g_camp_screen->character_info->SetEnabled(0);
        for (index = 0; index < 7; ++index) {
            g_camp_stat_labels[index]->SetActive(true);
        }
        for (index = 0; index < 4; ++index) {
            g_camp_info_labels[index]->SetActive(true);
        }
        break;
    }
    g_camp_screen->item_mode = mode;
    g_camp_screen->redraw_flags |= 0x2000;
}

// FUNCTION: WIZ8 0x005BA110
void OpenItemInfoDialog(W8ItemInstance* item, W8DialogDestroyCallback destroy_callback)
{
    W8DialogBase* dialog;

    if (IsItemWornByCharacter(g_review_character, item) == 0 &&
        IsItemCarriedByCharacter(g_review_character, item) == 0) {
        dialog = new W8AssayDialog(item, 0);
    } else {
        dialog = new W8AssayDialog(item, g_review_character);
    }
    dialog->SetText(&g_wchar_00689b34);
    dialog->SetOrigin(g_info_dialog_x, g_info_dialog_y);
    dialog->m_destroy_callback = destroy_callback;
    DisplayCampDialog(dialog);
}

/* The secondary-activation callbacks the seven attribute labels and four
   secondary labels on the camp secondary panel carry: each opens the stat
   info dialog at its own index. Note retail wires both
   g_camp_info_labels[2] and [3] to the index-3 thunk. */
// FUNCTION: WIZ8 0x005BA200
void OpenStatInfoDialog005BA200(void)
{
    OpenStatInfoDialog005BA2B0(0);
}

// FUNCTION: WIZ8 0x005BA210
void OpenStatInfoDialog005BA210(void)
{
    OpenStatInfoDialog005BA2B0(1);
}

// FUNCTION: WIZ8 0x005BA220
void OpenStatInfoDialog005BA220(void)
{
    OpenStatInfoDialog005BA2B0(2);
}

// FUNCTION: WIZ8 0x005BA230
void OpenStatInfoDialog005BA230(void)
{
    OpenStatInfoDialog005BA2B0(3);
}

// FUNCTION: WIZ8 0x005BA240
void OpenStatInfoDialog005BA240(void)
{
    OpenStatInfoDialog005BA2B0(4);
}

// FUNCTION: WIZ8 0x005BA250
void OpenStatInfoDialog005BA250(void)
{
    OpenStatInfoDialog005BA2B0(5);
}

// FUNCTION: WIZ8 0x005BA260
void OpenStatInfoDialog005BA260(void)
{
    OpenStatInfoDialog005BA2B0(6);
}

// FUNCTION: WIZ8 0x005BA270
void OpenSecondaryStatInfoDialog005BA270(void)
{
    OpenSecondaryStatInfoDialog005BA310(0);
}

// FUNCTION: WIZ8 0x005BA280
void OpenSecondaryStatInfoDialog005BA280(void)
{
    OpenSecondaryStatInfoDialog005BA310(1);
}

// FUNCTION: WIZ8 0x005BA290
void OpenSecondaryStatInfoDialog005BA290(void)
{
    OpenSecondaryStatInfoDialog005BA310(3);
}

// FUNCTION: WIZ8 0x005BA2A0
void OpenSecondaryStatInfoDialog005BA2A0(void)
{
    OpenSecondaryStatInfoDialog005BA310(4);
}

// FUNCTION: WIZ8 0x005BA2B0
void OpenStatInfoDialog005BA2B0(unsigned int uiIndex)
{
    DisplayCampDialog(new W8AttributeInfoDialog(uiIndex));
}

// FUNCTION: WIZ8 0x005BA310
void OpenSecondaryStatInfoDialog005BA310(unsigned int uiIndex)
{
    DisplayCampDialog(new W8SecondaryAttributeInfoDialog(uiIndex));
}

// FUNCTION: WIZ8 0x005BA370
void IdentifyAndOpenItemInfo(W8ItemInstance* item)
{
    if (CanItemLeaveItsSlot(item) != 0) {
        if (PartyAttemptsToIdentifyItem(item, 0) != 0 && g_camp_screen->realm_flags[1] != 0) {
            RebuildCampItemList();
            g_camp_screen->redraw_flags |= 0xfffffff;
        }
    }
    OpenItemInfoDialog(item, 0);
    SetCampItemActionMode(0);
}

// FUNCTION: WIZ8 0x005BA3D0
void DropHeldItem005BA3D0(void)
{
    if (ResolvePendingCampCharacter(1) != 0) {
        if (DropItemInHand(0) != 0) {
            SetCampItemActionMode(0);
        }
    }
}

// FUNCTION: WIZ8 0x005BA400
void OpenSplitStackDialog(W8ItemInstance* item)
{
    W8DialogBase* dialog;

    g_split_item_source = 0;
    if (item->iItemNo != -1 && item->stack_count > 1 &&
        (g_item_records[item->iItemNo].flags_041 & 2) == 0 &&
        g_item_records[item->iItemNo].quantity_kind == 1) {
        g_split_item_source = item;
        dialog = new W8SplitItemDialog(g_split_dialog_kind, item, -1);
        dialog->SetText(&g_wchar_00689b34);
        dialog->SetOrigin(g_split_dialog_x, g_split_dialog_y);
        dialog->m_destroy_callback = SplitStackDialogResult;
        DisplayCampDialog(dialog);
        SetCampItemActionMode(0);
    }
}

// FUNCTION: WIZ8 0x005BA4F0
void UseItem005BA4F0(W8ItemInstance* item)
{
    unsigned short slot;

    if (ValidateItemSpellUse(giReviewCharSlot, item, 0) != 0) {
        SetCampItemActionMode(0);
        return;
    }
    if (CanCastFromItem(g_review_character, item) != 0) {
        LearnSpellFromItem(g_review_character, item);
    } else {
        if (IsUsableItemClass(item) == 0 || IsSpecialItemId(item) != 0) {
            gXStatus.item_drag_active = 1;
            gXStatus.dragged_item = item;
            gXStatus.dragged_character_slot = static_cast<char>(giReviewCharSlot);
            GetOriginOfCharacterItem(giReviewCharSlot, item, &gXStatus.dragged_item_origin, &slot);
        } else {
            if (g_status.item_in_cursor == 0) {
                MarkCampCharacterPending(item);
                CopyItemInstance(&g_status.item_in_hand_235b, item, g_review_character, 1);
            }
        }
        DismissSelectedPartyCharacter();
    }
    RebuildCampItemList();
    g_camp_screen->redraw_flags |= 0xfffffff;
    SetCampItemActionMode(0);
}

// FUNCTION: WIZ8 0x005BA5D0
void MergeItemStacksWithHeld(W8ItemInstance* item)
{
    if (item->iItemNo != -1) {
        if (MergeItems(g_review_character, item) != 0) {
            RebuildCampItemList();
            g_camp_screen->redraw_flags |= 0xfffffff;
            SetCampItemActionMode(0);
        }
    }
}

// FUNCTION: WIZ8 0x005BA620
void ReportCastResult(int party_slot)
{
    wchar_t* text;
    W8Character* character;
    int result;

    StartBreathCycle(giReviewCharSlot, 0);
    result = RevealCharacterItemBindingsByProfession(giReviewCharSlot, party_slot);
    character = g_status.buffers.Char + party_slot;
    if (result == 0) {
        SoundPlay("Data\\Sound\\Misc\\Spell Fizzle 01.wav", 0);
        text = FormatWideString(gppStringList[0x6f8 / 4], character->name, 0, 1, 0);
        ShowCampNoticeLine(text, 0, 1, 0);
    } else if (result == 1) {
        SoundPlay("Data\\Sound\\Misc\\GeneralMagic.wav", 0);
        text = FormatWideString(gppStringList[0x6f4 / 4], character->name, 0, 1, 0);
        ShowCampNoticeLine(text, 0, 1, 0);
    } else if (result == 2) {
        SoundPlay("Data\\Sound\\Misc\\GeneralMagic.wav", 0);
        text = FormatWideString(gppStringList[0x6f0 / 4], character->name, 0, 1, 0);
        ShowCampNoticeLine(text, 0, 1, 0);
    }
    g_camp_screen->redraw_flags |= 0xfffffff;
    SetCampItemActionMode(0);
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
    if (g_status.buffers.XChar[giCasterCharSlot].fOccupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(giCasterCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x66a, 0);
    }
    if (item->iItemNo != -1) {
        if (CanItemLeaveItsSlot(item) != 0) {
            ResetCombatSlot(&target);
            target.iType = W8_TARGET_KIND_ITEM;
            if (CommitPartySlotSpell(giCasterCharSlot, 0x17, 8, &target) == 1) {
                OpenItemInfoDialog(item, 0);
                if (item->identified == 0) {
                    QueueCharacterEvent(g_status.buffers.Char + giCasterCharSlot,
                                        g_character_event_kind_005ee65c, 0,
                                        g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                }
            }
            RebuildCampItemList();
            g_camp_screen->redraw_flags |= 0xfffffff;
            SetCampItemActionMode(0);
            giCasterCharSlot = -1;
            return;
        }
        QueueCharacterEvent(g_status.buffers.Char + giCasterCharSlot,
                            g_character_event_kind_005ee65c, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
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
    if (g_status.buffers.XChar[uiTargetChar].fOccupied == 0) {
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
    if (g_status.buffers.XChar[giCasterCharSlot].fOccupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(giCasterCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x696, 0);
    }
    ResetCombatSlot(&target);
    target.iType = W8_TARGET_KIND_CHARACTER;
    target.iChar = uiTargetChar;
    CommitPartySlotSpell(giCasterCharSlot, 0x3a, 8, &target);
    RebuildCampItemList();
    g_camp_screen->redraw_flags |= 0xfffffff;
    SetCampItemActionMode(0);
    giCasterCharSlot = -1;
}

// FUNCTION: WIZ8 0x005BAA10
bool CanCharacterUseItemEntry(W8Character* character, W8ItemInstance* item)
{
    if (CanCharacterActivateItem(character, item) == 0) {
        if (CanCastFromItem(character, item) == 0) {
            if (IsUsableItemClass(item) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x005BAA50
bool CanSplitItemStack(const W8ItemInstance* item)
{
    const W8ItemDatabaseRecord* record = g_item_records + item->iItemNo;
    if ((record->flags_041 & 2) != 0) {
        return 0;
    }
    return record->quantity_kind == 1;
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

    if (static_cast<W8SplitItemDialog*>(dialog)->split_result_0c8 != g_split_result_kind) {
        return;
    }
    count = static_cast<W8SplitItemDialog*>(dialog)->split_count_0c0;
    remaining = g_split_item_source->stack_count - static_cast<unsigned char>(count);
    carried = (unsigned char)count;
    if (count == 0) {
        return;
    }
    if (g_status.item_in_cursor != 0) {
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
            (gXStatus.held_item_origin == 0 || gXStatus.held_item_origin == 1)) {
            character = g_status.buffers.Char + gXStatus.held_item_source;
            if (gXStatus.held_item_origin == 1) {
                if (character->EquippedItem[static_cast<short>(gXStatus.held_item_slot)].iItemNo ==
                        -1 &&
                    CanEquipItemInSlot(character, g_status.item_in_hand_235b.iItemNo,
                                       static_cast<unsigned char>(gXStatus.held_item_slot),
                                       0) != 0 &&
                    CanCharacterUseItem(character, g_status.item_in_hand_235b.iItemNo) != 0) {
                    destination =
                        character->EquippedItem + static_cast<short>(gXStatus.held_item_slot);
                }
            } else if (character->backpack[static_cast<short>(gXStatus.held_item_slot)].iItemNo ==
                       -1) {
                destination = character->backpack + static_cast<short>(gXStatus.held_item_slot);
            }
        }
        if (destination == 0 &&
            (character == 0 || AddItemToCharacter(character, &split, 0, 0, 0) == 0) &&
            AddItemToParty(&split, 0, 0) == 0) {
            ShowCampNoticeLine(gppStringList[0x2454 / 4], 0, 1, 0);
            g_status.item_in_hand_235b.stack_count = remaining;
            if (ResolvePendingCampCharacter(1) != 0 && DropItemInHand(0) != 0) {
                SetCampItemActionMode(0);
            }
            destination = &g_status.item_in_hand_235b;
        }
        if (destination != 0) {
            CopyItemInstance(destination, &split, 0, 1);
        }
    } else {
        if (count == g_split_item_source->stack_count) {
            CopyItemInstance(&g_status.item_in_hand_235b, g_split_item_source, g_review_character,
                             1);
            carried = g_status.item_in_hand_235b.stack_count;
        } else {
            split = *g_split_item_source;
            split.stack_count = (unsigned char)count;
            g_split_item_source->stack_count = remaining;
            CopyItemInstance(&g_status.item_in_hand_235b, &split, 0, 1);
            gXStatus.held_item_source = giReviewCharSlot;
            GetOriginOfCharacterItem(giReviewCharSlot, g_split_item_source,
                                     &gXStatus.held_item_origin, &gXStatus.held_item_slot);
            carried = g_status.item_in_hand_235b.stack_count;
        }
    }
    g_status.item_in_hand_235b.stack_count = carried;
    RebuildEquipmentAndDerivedStatsForSlot(giReviewCharSlot);
    RebuildCampItemList();
    RecalculateCharacterDerivedStats(g_status.buffers.Char + giReviewCharSlot);
    RecalculateCarriedWeight(g_review_character);
    RedistributePartyEncumbrance();
    SetCampItemActionMode(0);
    g_camp_screen->redraw_flags |= 0xfffffff;
}

/* Mode/tail bits the update pass keys on: the camp entry_mode byte at +0xd3f
   selects which use paths are live, and iTargetingMode selects what the item
   cursor is allowed to target. */
// FUNCTION: WIZ8 0x005BAD20
void UpdateItemCursorForState(int flag, W8ItemInstance* item, int slot)
{
    if (g_camp_screen->entry_mode == 0) {
        return;
    }
    switch (gXStatus.iTargetingMode) {
    case 1:
        if ((g_camp_screen->entry_mode == 7 || g_camp_screen->entry_mode == 9) &&
            g_status.buffers.XChar[slot].fOccupied != 0 &&
            g_status.buffers.Char[slot].uiCondition[0x13] == 0) {
            SetTargetCursor(GetTargetingCursorForState(flag));
            return;
        }
        if (CanPartySlotParticipate(slot) != 0) {
            SetTargetCursor(GetTargetingCursorForState(flag));
        }
        return;
    case 6:
        if (item == 0) {
            srAssertFail("pItem", "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp",
                         0x763, 0);
        }
        if (item->iItemNo == -1) {
            return;
        }
        if ((g_camp_screen->entry_mode == 2 || g_camp_screen->entry_mode == 8) &&
            CanItemLeaveItsSlot(item) != 0) {
            SetTargetCursor(GetTargetingCursorForState(flag));
            return;
        }
        if (g_camp_screen->entry_mode == 3 || g_camp_screen->entry_mode == 6) {
            SetTargetCursor(GetTargetingCursorForState(flag));
            return;
        }
        if (g_camp_screen->entry_mode == 4) {
            const W8ItemDatabaseRecord* record = g_item_records + item->iItemNo;
            if ((record->flags_041 & 2) != 0 || record->quantity_kind != 1) {
                return;
            }
            if (gXStatus.fCombatMode != 0 && IsEquippableItemClass(item) == 0 &&
                (g_combat_state->equip_phase_a50 == 0 ||
                 g_status.buffers.XChar[giReviewCharSlot].pending_action != 9)) {
                return;
            }
            SetTargetCursor(GetTargetingCursorForState(flag));
            return;
        }
        if (g_camp_screen->entry_mode == 5) {
            if (CanCharacterActivateItem(g_review_character, item) != 0 ||
                CanCastFromItem(g_review_character, item) != 0 || IsUsableItemClass(item) != 0) {
                SetTargetCursor(GetTargetingCursorForState(flag));
            }
            return;
        }
        if (g_camp_screen->entry_mode != 1 || item->iItemNo == -1) {
            return;
        }
        if (static_cast<char>(flag) == 0) {
            if (g_status.item_in_cursor == 0) {
                SetTargetCursor(3);
                return;
            }
            SetTargetCursor(0xf);
            SetItemCursor(0xe);
            return;
        }
        if (g_status.item_in_cursor == 0) {
            SetTargetCursor(4);
            return;
        }
        SetTargetCursor(0xe);
        SetItemCursor(0xf);
        return;
    case 7:
        if (IsDeadCharacterTargetable(slot) != 0) {
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
        if (g_status.item_in_cursor != 0) {
            SetTargetCursor(0xf);
            SetItemCursor(0xe);
            return;
        }
        SetTargetCursor(3);
        return;
    }
    if (g_status.item_in_cursor != 0) {
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

    if (gXStatus.fCombatMode != 0 && g_combat_state->round_active_001 == 0 &&
        gXStatus.fPartyMovementMode == 0 && g_combat_state->equip_phase_a50 == 0) {
        ShowCampNoticeLine(gppStringList[0x240c / 4], 0, 1, 0);
        return;
    }
    if (IsPartySlotEligible(giReviewCharSlot) == 0) {
        ShowCampNoticeLine(gppStringList[0x245c / 4], 0, 1, 0);
        return;
    }
    character = g_status.buffers.Char + giReviewCharSlot;
    BindEquippedItem(character, 6);
    BindEquippedItem(character, 7);
    if (CanUnequipSlotItem(character, 6) != 0 && CanUnequipSlotItem(character, 7) != 0) {
        SwapWeaponSetSlots(giReviewCharSlot, 0, 1);
        g_camp_screen->item_redraw_flags |= 0x3ffe00;
        g_camp_screen->redraw_flags |= 0x100;
        g_camp_screen->redraw_flags |= 0x2000;
        return;
    }
    ShowCampNoticeLine(gppStringList[0x2458 / 4], 0, 1, 0);
}

// FUNCTION: WIZ8 0x005BB140
void TogglePartyRowFlag(void)
{
    if ((g_camp_action_buttons[1]->m_stateFlags & g_W8TextControlMask005ED570) != 0) {
        g_status.buffers.XChar[giReviewCharSlot].item_action_pending_0f5 = 1;
        g_camp_action_buttons[0]->SetEnabled(0);
        g_camp_action_buttons[0]->Invalidate(0);
        return;
    }
    g_status.buffers.XChar[giReviewCharSlot].item_action_pending_0f5 = 0;
    g_camp_action_buttons[0]->SetEnabled(1);
    g_camp_action_buttons[0]->Invalidate(0);
}

// FUNCTION: WIZ8 0x005BB1C0
void SelectItemsRealmTab005BB1C0(void)
{
    SelectItemsRealmTab005BB250(0);
}

// FUNCTION: WIZ8 0x005BB1D0
void SelectItemsRealmTab005BB1D0(void)
{
    SelectItemsRealmTab005BB250(1);
}

// FUNCTION: WIZ8 0x005BB1E0
void SelectItemsRealmTab005BB1E0(void)
{
    SelectItemsRealmTab005BB250(2);
}

// FUNCTION: WIZ8 0x005BB1F0
void SelectItemsRealmTab005BB1F0(void)
{
    SelectItemsRealmTab005BB250(3);
}

// FUNCTION: WIZ8 0x005BB200
void SelectItemsRealmTab005BB200(void)
{
    SelectItemsRealmTab005BB250(4);
}

// FUNCTION: WIZ8 0x005BB210
void SelectItemsRealmTab005BB210(void)
{
    SelectItemsRealmTab005BB250(5);
}

// FUNCTION: WIZ8 0x005BB220
void SortPartyItemPool005BB220(void)
{
    SortPartyItemPool();
    g_camp_screen->item_scroll = 0;
    RebuildCampItemList();
    g_camp_screen->item_redraw_flags |= 0x7fc00000;
}

/* Tab index to realm_flags index: the UI tab order is not the realm order. */
// FUNCTION: WIZ8 0x005BB250
void SelectItemsRealmTab005BB250(int tab)
{
    int realm;
    int index;

    switch (tab) {
    case 0:
        realm = 2;
        break;
    case 1:
        realm = 3;
        break;
    case 2:
        realm = 5;
        break;
    case 3:
        realm = 4;
        break;
    case 4:
        realm = 0;
        break;
    case 5:
        realm = 1;
        break;
    default:
        realm = tab;
    }
    if ((g_camp_realm_tabs[tab]->m_stateFlags & g_W8TextControlMask005ED570) == 0) {
        g_camp_screen->realm_flags[realm] = 0;
    } else {
        g_camp_screen->realm_flags[realm] = 1;
        if (realm != 0 && realm != 1) {
            ClearOtherRealmFilters(realm);
            index = 0;
            do {
                if (index != tab &&
                    (g_camp_realm_tabs[index]->m_stateFlags & g_W8TextControlMask005ED570) != 0) {
                    g_camp_realm_tabs[index]->DisableSecondaryState(1);
                }
                ++index;
            } while (index < 4);
        }
    }
    g_camp_screen->item_scroll = 0;
    RebuildCampItemList();
    if (gfKeyState[0x11] == 0) {
        g_camp_screen->item_redraw_flags |= 0x7fc00000;
        return;
    }
    g_camp_screen->redraw_flags |= 0xfffffff;
}

/* Region reason values are bit codes set by the region manager: 0x8 left
   release, 0x10 left down, 0x40 right press, 0x80 right release, 0x100
   double-click/activate, 0x400 enter/leave transition, 0x800 mouse wheel. */
// FUNCTION: WIZ8 0x005BB350
unsigned char BackpackRegionHandler(const InputAtom* event, W8Region* region)
{
    int slot;
    W8ItemInstance* item;

    slot = region->callback_id;
    item = g_review_character->backpack + slot;
    if (item->iItemNo == -1 && g_status.item_in_cursor == 0) {
        PushButtonSoundScheme(0, 1);
    }
    if (event->usEvent < 0x81) {
        if (event->usEvent == 0x80) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (event->usEvent == 8) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        } else {
            if (event->usEvent != 0x10) {
                return 0;
            }
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
                return 1;
            }
            HandleCampItemClick(item, slot, 0);
            if (item->iItemNo != -1) {
                SetItemTooltip(item, region);
                SetRegionHelpForceEnabled(1);
                return 1;
            }
        }
        DisableRegionHelpFlag(region);
        return 1;
    }
    if (event->usEvent == 0x100) {
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && item->iItemNo != -1 &&
            g_camp_screen->entry_mode != 3) {
            g_camp_entry_parameter = g_review_character;
            if (CanItemLeaveItsSlot(item) != 0 && PartyAttemptsToIdentifyItem(item, 0) != 0 &&
                g_camp_screen->realm_flags[1] != 0) {
                RebuildCampItemList();
                g_camp_screen->redraw_flags |= 0xfffffff;
            }
            OpenItemInfoDialog(item, 0);
            SetCampItemActionMode(0);
        }
        return 1;
    }
    if (event->usEvent == 0x400) {
        if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
            if (item->iItemNo != -1 || g_status.item_in_cursor != 0) {
                g_camp_screen->item_redraw_flags |= 2 << (slot & 0x1f);
            }
            UpdateItemCursorForState((region->flags & W8_REGION_MOUSE_ENTER) != 0, item, 0);
            if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
                if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
                    return 0;
                }
            } else if (item->iItemNo != -1) {
                SetItemTooltip(item, region);
                SetRegionHelpForceEnabled(1);
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
        (g_status.item_in_cursor == 0 || g_camp_screen->entry_mode == 1 ||
         CanEquipItemInSlot(g_review_character, g_status.item_in_hand_235b.iItemNo,
                            (unsigned char)slot, 1) == 0 ||
         CanCharacterUseItem(g_review_character, g_status.item_in_hand_235b.iItemNo) == 0)) {
        PushButtonSoundScheme(0, 1);
    }
    if (event->usEvent < 0x81) {
        if (event->usEvent == 0x80) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (event->usEvent == 8) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            DisableRegionHelpFlag(region);
            return 1;
        }
        if (event->usEvent != 0x10) {
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            HandleCampItemClick(item, slot, 1);
            delay = g_settings.tooltip_delay_ms;
            if ((unsigned int)delay > 300) {
                delay = 300;
            }
            SetRegionHelpDelay(delay);
            SetRegionHelpForceEnabled(1);
            EnableRegionHelpFlag(region);
            if (item->iItemNo != -1) {
                name = FormatItemDisplayName(item, 0);
                swprintf(g_camp_screen->caption, L"%s (%s)", name,
                         gppStringList[g_equip_slot_label_ids[slot]]);
                SetRegionHelpText(g_camp_screen->caption);
                return 1;
            }
            swprintf(g_camp_screen->caption, g_format_s_006068e4,
                     gppStringList[g_equip_slot_label_ids[slot]]);
            SetRegionHelpText(g_camp_screen->caption);
            return 1;
        }
    } else {
        if (event->usEvent != 0x100) {
            if (event->usEvent != 0x400) {
                return 0;
            }
            if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                if (item->iItemNo != -1 || g_status.item_in_cursor != 0) {
                    g_camp_screen->item_redraw_flags |= 0x400 << (slot & 0x1f);
                }
                UpdateItemCursorForState(1, item, 0);
                EnableRegionHelpFlag(region);
                delay = g_settings.tooltip_delay_ms;
                if ((unsigned int)delay > 300) {
                    delay = 300;
                }
                SetRegionHelpDelay(delay);
                SetRegionHelpForceEnabled(1);
                if (item->iItemNo == -1) {
                    swprintf(g_camp_screen->caption, g_format_s_006068e4,
                             gppStringList[g_equip_slot_label_ids[slot]]);
                } else {
                    name = FormatItemDisplayName(item, 0);
                    swprintf(g_camp_screen->caption, L"%s (%s)", name,
                             gppStringList[g_equip_slot_label_ids[slot]]);
                }
                SetRegionHelpText(g_camp_screen->caption);
            }
            if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
                if (item->iItemNo != -1 || g_status.item_in_cursor != 0) {
                    g_camp_screen->item_redraw_flags |= 0x400 << (slot & 0x1f);
                }
                UpdateItemCursorForState(0, item, 0);
                DisableRegionHelpFlag(region);
            }
            return 0;
        }
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && item->iItemNo != -1 &&
            g_camp_screen->entry_mode != 3) {
            g_camp_entry_parameter = g_review_character;
            if (CanItemLeaveItsSlot(item) != 0 && PartyAttemptsToIdentifyItem(item, 0) != 0 &&
                g_camp_screen->realm_flags[1] != 0) {
                RebuildCampItemList();
                g_camp_screen->redraw_flags |= 0xfffffff;
            }
            OpenItemInfoDialog(item, 0);
            SetCampItemActionMode(0);
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
        g_status.party_item_pool_0021[g_camp_screen->item_list_4ec[pool_index]].iItemNo == -1) {
        pool_index = g_status.party_item_count_1791;
    } else {
        pool_index = g_camp_screen->item_list_4ec[pool_index];
    }
    item = g_status.party_item_pool_0021 + pool_index;
    if (item->iItemNo == -1 && g_status.item_in_cursor == 0) {
        PushButtonSoundScheme(0, 1);
    }
    if (event->usEvent < 0x101) {
        if (event->usEvent == 0x100) {
            if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 &&
                pool_index < g_status.party_item_count_1791 &&
                g_camp_screen->entry_mode != 3) {
                g_camp_entry_parameter = g_review_character;
                if (CanItemLeaveItsSlot(item) != 0 && PartyAttemptsToIdentifyItem(item, 0) != 0 &&
                    g_camp_screen->realm_flags[1] != 0) {
                    RebuildCampItemList();
                    g_camp_screen->redraw_flags |= 0xfffffff;
                }
                OpenItemInfoDialog(item, 0);
                SetCampItemActionMode(0);
            }
            return 1;
        }
        if (event->usEvent == 8) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            DisableRegionHelpFlag(region);
            return 1;
        }
        if (event->usEvent != 0x10) {
            if (event->usEvent != 0x80) {
                return 0;
            }
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        HandleCampItemClick(item, pool_index, 2);
        if (item->iItemNo != -1) {
            SetItemTooltip(item, region);
            return 1;
        }
    } else {
        if (event->usEvent == 0x400) {
            if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
                if (item->iItemNo != -1 || g_status.item_in_cursor != 0) {
                    g_camp_screen->item_redraw_flags |= 0x800000 << (slot & 0x1f);
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
        if (event->usEvent != 0x800) {
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
            pool_index = g_status.party_item_count_1791;
        } else {
            pool_index = g_camp_screen->item_list_4ec[pool_index];
        }
        if (g_status.party_item_pool_0021[pool_index].iItemNo != -1) {
            SetItemTooltip(g_status.party_item_pool_0021 + pool_index, region);
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
    if (event->usEvent < 0x41) {
        if (event->usEvent == 0x40 || event->usEvent == 8) {
            g_camp_realm_tabs[region->callback_id]->OnLeftButtonDown(0);
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (event->usEvent == 0x10) {
            g_camp_realm_tabs[region->callback_id]->OnLeftButtonUp(0);
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            }
            return 1;
        }
    } else if (event->usEvent == 0x400) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_camp_realm_tabs[region->callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_camp_realm_tabs[region->callback_id]->OnMouseEnter(0);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005BBC70
unsigned char PanelTabRegionHandler(const InputAtom* event, W8Region* region)
{
    if (event->usEvent < 0x41) {
        if (event->usEvent == 0x40 || event->usEvent == 8) {
            g_camp_action_buttons[region->callback_id]->OnLeftButtonDown(0);
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (event->usEvent == 0x10) {
            g_camp_action_buttons[region->callback_id]->OnLeftButtonUp(0);
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            }
            return 1;
        }
    } else if (event->usEvent == 0x400) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_camp_action_buttons[region->callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_camp_action_buttons[region->callback_id]->OnMouseEnter(0);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005BBD30
void SetItemTooltip(W8ItemInstance* item, W8Region* region)
{
    int delay;
    wchar_t* name;
    const W8ItemDatabaseRecord* record;

    delay = g_settings.tooltip_delay_ms;
    if ((unsigned int)delay > 300) {
        delay = 300;
    }
    SetRegionHelpDelay(delay);
    SetRegionHelpForceEnabled(1);
    EnableRegionHelpFlag(region);
    name = FormatItemDisplayName(item, 0);
    wcscpy(g_camp_screen->caption, name);
    record = g_item_records + item->iItemNo;
    if (record->category == 3) {
        if (record->spell_id == 0) {
            srAssertFail("ubSpell != SPELL_NONE",
                         "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0xa7a, 0);
        }
        if (g_review_character->spell_learned[record->spell_id] == 1 &&
            (item->identified != 0 || item->spell_hint_07 != 0)) {
            wcscat(g_camp_screen->caption, L" (");
            wcscat(g_camp_screen->caption, gppStringList[0x24c0 / 4]);
            wcscat(g_camp_screen->caption, L")");
        }
    }
    SetRegionHelpText(g_camp_screen->caption);
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
        DrawCampItemLabel(
            &g_status.party_item_pool_0021[state->item_list_4ec[state->item_scroll + index]],
            (index & 1) * 0x31 + 0x22c, y, 1);
        ++index;
    }
    region = g_camp_screen_regions;
    for (index = 0; index < 12; ++index) {
        item = &character->EquippedItem[index];
        if (item->iItemNo != -1) {
            DrawCampItemLabel(item, region->label_x_18, region->label_y_1c,
                              static_cast<char>(region->label_flag_20));
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

    name = FormatItemDisplayName(item, 0);
    wcscpy(g_camp_screen->caption, name);
    bounds.left = left;
    bounds.top = top;
    bounds.right = left + 0xfa;
    bounds.bottom = top + 0xfa;
    text = new W8TextBuffer(&bounds, g_camp_screen->caption, g_font10arial,
                            g_W8TextBufferLayoutMask005ED558 | g_W8TextBufferLayoutMask005ED548, 4);
    if (text != 0) {
        height = text->m_lineCount;
        width = text->m_maxLineWidth + 4;
        height = GetFontHeight(g_font10arial) * height + 2;
        if (flag != 0) {
            bounds.right -= width;
            bounds.left -= width;
            text->SetLayoutBounds(&bounds, 1, 1);
        }
        ColorFillVideoSurfaceArea(0xfffffff2, bounds.left, bounds.top, bounds.left + width,
                                  bounds.top + height, 0x8000);
        buffer = static_cast<char*>(LockPrimarySurface(&pitch));
        if (buffer != 0) {
            // reinterpret-ok: SGP frame-buffer bytes to W8TextBuffer's byte view
            text->RenderText(reinterpret_cast<unsigned char*>(buffer), pitch, 2, 1, 1);
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
