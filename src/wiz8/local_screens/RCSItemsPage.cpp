#include "wiz8/local_screens/RCSItemsPage.h"

#include "wiz8/character.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/combat_state.h"
#include "wiz8/cursor.h"
#include "wiz8/dialog_code/AssayDialog.h"
#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/StatInfoDialogs.h"
#include "wiz8/game_status.h"
#include "wiz8/layouts/item_tables.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Controls.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/TextControl.h"
#include "wiz8/local_code/Widget.h"
#include "wiz8/magic.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/RCSCommon.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/sr_api.h"
#include "wiz8/targeting.h"
#include "wiz8/utility.h"
#include "wiz8/xstatus.h"

#include "input.h"
#include "soundman.h"

#include <new>
#include <wchar.h>

/* Retail Local Screens\RCSItemsPage.cpp - the items page of the camp
   review screen: page-mode toggling, the item-info/split/stat dialogs,
   use-item and targeting entry points, and the five region callbacks the
   camp region table registers for the backpack, equipment, item-pool,
   realm-tab and panel-tab areas. */

// GLOBAL: WIZ8 0x0069C0EC
int giCasterCharSlot;

// GLOBAL: WIZ8 0x0069C424
W8ItemInstance* g_split_item_source_0069c424;

// GLOBAL: WIZ8 0x005EFB44
int g_split_result_kind_005efb44 = 1;
// GLOBAL: WIZ8 0x005EFB4C
int g_split_dialog_x_005efb4c = 0xa1;
// GLOBAL: WIZ8 0x005EFB50
int g_split_dialog_y_005efb50 = 0x94;
// GLOBAL: WIZ8 0x005EFB64
int g_split_dialog_kind_005efb64 = 0;
// GLOBAL: WIZ8 0x005EF958
int g_info_dialog_x_005ef958 = 0x80;
// GLOBAL: WIZ8 0x005EF95C
int g_info_dialog_y_005ef95c = 0x61;
// GLOBAL: WIZ8 0x005EE65C
int g_character_event_kind_005ee65c = 0x35;

/* 0x0061E7C4: the twelve equip-slot label message ids indexed by region
   callback_id. */
// GLOBAL: WIZ8 0x0061E7C4
const unsigned short g_equip_slot_label_ids_61e7c4[12] = {
    0x433, 0x434, 0x435, 0x436, 0x437, 0x438, 0x439, 0x43a, 0x43b, 0x43c, 0x43d, 0x43e,
};

/* The items page swaps two control panels in and out: mode zero shows the
   item page, mode one the character page. */
// FUNCTION: WIZ8 0x005B9FD0
void SetItemPageMode005B9FD0(char mode)
{
    int index;

    if (mode == 0) {
        g_panel_controls_69c43c[0]->EnableSecondaryState(1);
        g_panel_controls_69c43c[1]->DisableSecondaryState(1);
        g_panel_control_69c444->SetActive(true);
        g_camp_screen_0069c0f4->character_info->SetEnabled(0);
        for (index = 0; index < 7; ++index) {
            g_panel_controls_69c448[index]->SetActive(true);
        }
        for (index = 0; index < 4; ++index) {
            g_panel_controls_69c42c[index]->SetActive(true);
        }
    } else if (mode == 1) {
        g_panel_controls_69c43c[0]->DisableSecondaryState(1);
        g_panel_controls_69c43c[1]->EnableSecondaryState(1);
        g_panel_control_69c444->SetActive(false);
        g_camp_screen_0069c0f4->character_info->SetEnabled(1);
        for (index = 0; index < 7; ++index) {
            g_panel_controls_69c448[index]->SetActive(false);
        }
        for (index = 0; index < 4; ++index) {
            g_panel_controls_69c42c[index]->SetActive(false);
        }
    } else {
        srAssertFail("FALSE", "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x544,
                     "Info toggle error");
    }
    g_camp_screen_0069c0f4->item_mode = mode;
    g_camp_screen_0069c0f4->redraw_flags |= 0x2000;
}

// FUNCTION: WIZ8 0x005BA110
void OpenItemInfoDialog005BA110(W8ItemInstance* item, W8DialogDestroyCallback destroy_callback)
{
    W8DialogBase* dialog;

    if (IsItemWornByCharacter(g_value_0069c0f8, item) == 0 &&
        IsItemCarriedByCharacter(g_value_0069c0f8, item) == 0) {
        dialog = new W8AssayDialog(item, 0);
    } else {
        dialog = new W8AssayDialog(item, g_value_0069c0f8);
    }
    dialog->SetText(&g_wchar_00689b34);
    dialog->SetOrigin(g_info_dialog_x_005ef958, g_info_dialog_y_005ef95c);
    dialog->m_destroy_callback = destroy_callback;
    DisplayCampDialog(dialog);
}

// FUNCTION: WIZ8 0x005BA2B0
void OpenStatInfoDialog005BA2B0(unsigned int uiIndex)
{
    DisplayCampDialog(new W8StatInfoDialog005DFC70(uiIndex));
}

// FUNCTION: WIZ8 0x005BA310
void OpenSecondaryStatInfoDialog005BA310(unsigned int uiIndex)
{
    DisplayCampDialog(new W8StatInfoDialog005E0180(uiIndex));
}

// FUNCTION: WIZ8 0x005BA370
void IdentifyAndOpenItemInfo005BA370(W8ItemInstance* item)
{
    if (CanItemLeaveItsSlot(item) != 0) {
        if (PartyAttemptsToIdentifyItem(item, 0) != 0 &&
            g_camp_screen_0069c0f4->realm_flags[1] != 0) {
            Function5A4A00();
            g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
        }
    }
    OpenItemInfoDialog005BA110(item, 0);
    Function5B59B0(0);
}

// FUNCTION: WIZ8 0x005BA3D0
void DropHeldItem005BA3D0(void)
{
    if (Function5A5F30(1) != 0) {
        if (DropItemInHand(0) != 0) {
            Function5B59B0(0);
        }
    }
}

// FUNCTION: WIZ8 0x005BA400
void OpenSplitStackDialog005BA400(W8ItemInstance* item)
{
    W8DialogBase* dialog;

    g_split_item_source_0069c424 = 0;
    if (item->item_id != -1 && item->stack_count > 1 &&
        (g_item_records[item->item_id].flags_041 & 2) == 0 &&
        g_item_records[item->item_id].quantity_kind == 1) {
        g_split_item_source_0069c424 = item;
        dialog = new W8Dialog005DCED0(g_split_dialog_kind_005efb64, item, -1);
        dialog->SetText(&g_wchar_00689b34);
        dialog->SetOrigin(g_split_dialog_x_005efb4c, g_split_dialog_y_005efb50);
        dialog->m_destroy_callback = SplitStackDialogResult005BAA80;
        DisplayCampDialog(dialog);
        Function5B59B0(0);
    }
}

// FUNCTION: WIZ8 0x005BA4F0
void UseItem005BA4F0(W8ItemInstance* item)
{
    unsigned short slot;

    if (Function522B80(g_rcs_mode_0064cbe8, item, 0) != 0) {
        Function5B59B0(0);
        return;
    }
    if (CanCastFromItem(g_value_0069c0f8, item) != 0) {
        LearnSpellFromItem(g_value_0069c0f8, item);
    } else {
        if (Function522A00(item) == 0 || Function4DA0F0(item) != 0) {
            g_flag_00685071 = 1;
            g_value_00685072 = (int)item;
            g_value_00685077 = (char)g_rcs_mode_0064cbe8;
            GetOriginOfCharacterItem(g_rcs_mode_0064cbe8, item, &g_flag_00685076, &slot);
        } else {
            if (g_status_685170.item_in_cursor == 0) {
                Function5A6020(item);
                CopyItemInstance(&g_status_685170.item_in_hand_235b, item, g_value_0069c0f8, 1);
            }
        }
        DismissSelectedPartyCharacter();
    }
    Function5A4A00();
    g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
    Function5B59B0(0);
}

// FUNCTION: WIZ8 0x005BA5D0
void MergeItemStacksWithHeld005BA5D0(W8ItemInstance* item)
{
    if (item->item_id != -1) {
        if (MergeItems(g_value_0069c0f8, item) != 0) {
            Function5A4A00();
            g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
            Function5B59B0(0);
        }
    }
}

// FUNCTION: WIZ8 0x005BA620
void ReportCastResult005BA620(int party_slot)
{
    const wchar_t* text;
    W8Character* character;
    int result;

    Function52FE80(g_rcs_mode_0064cbe8, 0);
    result = Function548E20(g_rcs_mode_0064cbe8, party_slot);
    character = g_status_685170.buffers.characters + party_slot;
    if (result == 0) {
        SoundPlay("Data\\Sound\\Misc\\Spell Fizzle 01.wav", 0);
        text = FormatWideString(gppStringList[0x6f8 / 4], character->name, 0, 1, 0);
        Function5A4C00(text, 0, 1, 0);
    } else if (result == 1) {
        SoundPlay("Data\\Sound\\Misc\\GeneralMagic.wav", 0);
        text = FormatWideString(gppStringList[0x6f4 / 4], character->name, 0, 1, 0);
        Function5A4C00(text, 0, 1, 0);
    } else if (result == 2) {
        SoundPlay("Data\\Sound\\Misc\\GeneralMagic.wav", 0);
        text = FormatWideString(gppStringList[0x6f0 / 4], character->name, 0, 1, 0);
        Function5A4C00(text, 0, 1, 0);
    }
    g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
    Function5B59B0(0);
}

// FUNCTION: WIZ8 0x005BA740
void UseHeldItemOnItem005BA740(W8ItemInstance* item)
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
    if (g_status_685170.buffers.party_rows[giCasterCharSlot].occupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(giCasterCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x66a, 0);
    }
    if (item->item_id != -1) {
        if (CanItemLeaveItsSlot(item) != 0) {
            ResetCombatSlot(&target);
            target.iType = W8_TARGET_KIND_ITEM;
            if (Function5A6340(giCasterCharSlot, 0x17, 8, &target) == 1) {
                OpenItemInfoDialog005BA110(item, 0);
                if (item->identified == 0) {
                    QueueCharacterEvent(g_status_685170.buffers.characters + giCasterCharSlot,
                                        g_character_event_kind_005ee65c, 0,
                                        g_effect_argument_005ed8c8, g_effect_argument_005ed914);
                }
            }
            Function5A4A00();
            g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
            Function5B59B0(0);
            giCasterCharSlot = -1;
            return;
        }
        QueueCharacterEvent(g_status_685170.buffers.characters + giCasterCharSlot,
                            g_character_event_kind_005ee65c, 0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
}

// FUNCTION: WIZ8 0x005BA8E0
void TargetCharacterWithHeldItem005BA8E0(unsigned int uiTargetChar)
{
    W8CombatSlot target;

    if (uiTargetChar > 7) {
        srAssertFail("uiTargetChar < MAX_CHARS",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x692, 0);
    }
    if (g_status_685170.buffers.party_rows[uiTargetChar].occupied == 0) {
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
    if (g_status_685170.buffers.party_rows[giCasterCharSlot].occupied == 0) {
        srAssertFail("fCHAR_OCCUPIED(giCasterCharSlot)",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0x696, 0);
    }
    ResetCombatSlot(&target);
    target.iType = W8_TARGET_KIND_CHARACTER;
    target.iChar = uiTargetChar;
    Function5A6340(giCasterCharSlot, 0x3a, 8, &target);
    Function5A4A00();
    g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
    Function5B59B0(0);
    giCasterCharSlot = -1;
}

// FUNCTION: WIZ8 0x005BAA10
unsigned char CanCharacterUseItemEntry005BAA10(W8Character* character, W8ItemInstance* item)
{
    if (CanCharacterActivateItem(character, item) == 0) {
        if (CanCastFromItem(character, item) == 0) {
            if (Function522A00(item) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x005BAA50
unsigned char CanSplitItemStack005BAA50(const W8ItemInstance* item)
{
    const W8ItemDatabaseRecord* record = g_item_records + item->item_id;
    if ((record->flags_041 & 2) != 0) {
        return 0;
    }
    return record->quantity_kind == 1;
}

/* The split-stack dialog's layout is unresolved (ctor 0x005DCED0, size 0xd8);
   the destroy callback reads the entered count at +0xc0 and the kind marker
   at +0xc8 by offset. reinterpret-ok: unresolved gap class fields. */
// FUNCTION: WIZ8 0x005BAA80
void SplitStackDialogResult005BAA80(W8DialogBase* dialog)
{
    W8ItemInstance split;
    W8ItemInstance* destination;
    W8Character* character;
    unsigned int count;
    unsigned char remaining;
    unsigned char carried;

    if (static_cast<W8Dialog005DCED0*>(dialog)->result_0c8 != g_split_result_kind_005efb44) {
        return;
    }
    count = static_cast<W8Dialog005DCED0*>(dialog)->split_count_0c0;
    remaining = g_split_item_source_0069c424->stack_count - (unsigned char)count;
    carried = (unsigned char)count;
    if (count == 0) {
        return;
    }
    if (g_status_685170.item_in_cursor == 0) {
        if (count == g_split_item_source_0069c424->stack_count) {
            CopyItemInstance(&g_status_685170.item_in_hand_235b, g_split_item_source_0069c424,
                             g_value_0069c0f8, 1);
            carried = g_status_685170.item_in_hand_235b.stack_count;
        } else {
            split.item_id = g_split_item_source_0069c424->item_id;
            split.uses_or_charges = g_split_item_source_0069c424->uses_or_charges;
            split.identified = g_split_item_source_0069c424->identified;
            split.unknown_07[0] = g_split_item_source_0069c424->unknown_07[0];
            *reinterpret_cast<unsigned int*>(
                &split.unknown_07
                     [1]) = // reinterpret-ok: two-item layout word shared with the held item
                *reinterpret_cast<const unsigned int*>(
                    &g_split_item_source_0069c424->unknown_07[1]);
            split.stack_count = (unsigned char)count;
            g_split_item_source_0069c424->stack_count = remaining;
            CopyItemInstance(&g_status_685170.item_in_hand_235b, &split, 0, 1);
            g_held_item_source_006840c0 = g_rcs_mode_0064cbe8;
            GetOriginOfCharacterItem(g_rcs_mode_0064cbe8, g_split_item_source_0069c424,
                                     &g_held_item_origin_006840c4, &g_held_item_slot_006840c5);
            carried = g_status_685170.item_in_hand_235b.stack_count;
        }
    } else {
        if (count == g_split_item_source_0069c424->stack_count) {
            return;
        }
        split.item_id = g_split_item_source_0069c424->item_id;
        split.uses_or_charges = g_split_item_source_0069c424->uses_or_charges;
        split.identified = g_split_item_source_0069c424->identified;
        split.unknown_07[0] = g_split_item_source_0069c424->unknown_07[0];
        *reinterpret_cast<unsigned int*>(
            &split.unknown_07[1]) = // reinterpret-ok: unresolved layout word copied verbatim
            *reinterpret_cast<const unsigned int*>(&g_split_item_source_0069c424->unknown_07[1]);
        split.stack_count = remaining;
        if (g_held_item_source_006840c0 == -1) {
            goto add_to_pool;
        }
        character = g_status_685170.buffers.characters + g_held_item_source_006840c0;
        if (g_held_item_origin_006840c4 == 1) {
            if (character->equipment[(short)g_held_item_slot_006840c5].item_id != -1 ||
                CanEquipItemInSlot(character, g_status_685170.item_in_hand_235b.item_id,
                                   (unsigned char)g_held_item_slot_006840c5, 0) == 0 ||
                CanCharacterUseItem(character, g_status_685170.item_in_hand_235b.item_id) == 0) {
                goto add_to_character;
            }
            destination = character->equipment + (short)g_held_item_slot_006840c5;
        } else {
            if (g_held_item_origin_006840c4 != 0) {
                goto add_to_pool;
            }
            destination = character->backpack + (short)g_held_item_slot_006840c5;
            if (character->backpack[(short)g_held_item_slot_006840c5].item_id != -1) {
                goto add_to_character;
            }
        }
        goto store;
    add_to_character:
        if (AddItemToCharacter(character, &split, 0, 0, 0) != 0) {
            goto applied;
        }
    add_to_pool:
        if (AddItemToParty(&split, 0, 0) != 0) {
            goto applied;
        }
        Function5A4C00(gppStringList[0x2454 / 4], 0, 1, 0);
        g_status_685170.item_in_hand_235b.stack_count = remaining;
        if (Function5A5F30(1) != 0 && DropItemInHand(0) != 0) {
            Function5B59B0(0);
        }
        destination = &g_status_685170.item_in_hand_235b;
    store:
        CopyItemInstance(destination, &split, 0, 1);
    }
applied:
    g_status_685170.item_in_hand_235b.stack_count = carried;
    Function50E5C0(g_rcs_mode_0064cbe8);
    Function5A4A00();
    RecalculateCharacterDerivedStats(g_status_685170.buffers.characters + g_rcs_mode_0064cbe8);
    RecalculateCarriedWeight(g_value_0069c0f8);
    RedistributePartyEncumbrance();
    Function5B59B0(0);
    g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
}

/* Mode/tail bits the update pass keys on: the camp entry_mode byte at +0xd3f
   selects which use paths are live, and iTargetingMode selects what the item
   cursor is allowed to target. */
// FUNCTION: WIZ8 0x005BAD20
void UpdateItemCursorForState005BAD20(int flag, W8ItemInstance* item, int slot)
{
    int cursor;
    unsigned char allowed;

    if (g_camp_screen_0069c0f4->entry_mode == 0) {
        return;
    }
    if (gXStatus.iTargetingMode == 1) {
        if ((g_camp_screen_0069c0f4->entry_mode == 7 || g_camp_screen_0069c0f4->entry_mode == 9) &&
            g_status_685170.buffers.party_rows[slot].occupied != 0 &&
            g_status_685170.buffers.characters[slot].condition_turns[0x13] == 0) {
        set_cursor:
            cursor = GetTargetingCursorForState(flag);
            SetTargetCursor(cursor);
            return;
        }
        allowed = CanPartySlotParticipate(slot);
    } else {
        if (gXStatus.iTargetingMode != 6) {
            if (gXStatus.iTargetingMode != 7) {
                return;
            }
            if (Function53C2C0(slot) == 0) {
                return;
            }
            goto set_cursor;
        }
        if (item == 0) {
            srAssertFail("pItem", "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp",
                         0x763, 0);
        }
        if (item->item_id == -1) {
            return;
        }
        if ((g_camp_screen_0069c0f4->entry_mode == 2 || g_camp_screen_0069c0f4->entry_mode == 8) &&
            CanItemLeaveItsSlot(item) != 0) {
            goto set_cursor_item;
        }
        if (g_camp_screen_0069c0f4->entry_mode == 3) {
            goto set_cursor;
        }
        if (g_camp_screen_0069c0f4->entry_mode == 6) {
            goto set_cursor_item;
        }
        if (g_camp_screen_0069c0f4->entry_mode == 4) {
            const W8ItemDatabaseRecord* record = g_item_records + item->item_id;
            if ((record->flags_041 & 2) != 0) {
                return;
            }
            if (record->quantity_kind != 1) {
                return;
            }
            if (gXStatus.fCombatMode != 0) {
                if (Function5A6310(item) == 0) {
                    if (g_combat_state->flag_a50 == 0) {
                        return;
                    }
                    if (g_status_685170.buffers.party_rows[g_rcs_mode_0064cbe8].pending_action !=
                        9) {
                        return;
                    }
                }
                goto set_cursor;
            }
            goto set_cursor;
        }
        if (g_camp_screen_0069c0f4->entry_mode != 5) {
            if (g_camp_screen_0069c0f4->entry_mode != 1) {
                return;
            }
            if (item->item_id == -1) {
                return;
            }
            if ((char)flag == 0) {
                if (g_status_685170.item_in_cursor == 0) {
                    SetTargetCursor(3);
                    return;
                }
                SetTargetCursor(0xf);
                SetItemCursor(0xe);
                return;
            }
            if (g_status_685170.item_in_cursor == 0) {
                SetTargetCursor(4);
                return;
            }
            SetTargetCursor(0xe);
            SetItemCursor(0xf);
            return;
        }
        if (CanCharacterActivateItem(g_value_0069c0f8, item) == 0 &&
            CanCastFromItem(g_value_0069c0f8, item) == 0) {
            allowed = Function522A00(item);
        } else {
            goto set_cursor_item;
        }
    }
    if (allowed == 0) {
        return;
    }
set_cursor_item:
    cursor = GetTargetingCursorForState(flag);
    SetTargetCursor(cursor);
}

// FUNCTION: WIZ8 0x005BAFC0
void SetHandCursors005BAFC0(char mode)
{
    if (mode == 0) {
        if (g_status_685170.item_in_cursor != 0) {
            SetTargetCursor(0xf);
            SetItemCursor(0xe);
            return;
        }
        SetTargetCursor(3);
        return;
    }
    if (g_status_685170.item_in_cursor != 0) {
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
void UnequipBothHands005BB010(void)
{
    W8Character* character;

    if (gXStatus.fCombatMode != 0 && g_combat_state->flag_001 == 0 &&
        gXStatus.fPartyMovementMode == 0 && g_combat_state->flag_a50 == 0) {
        Function5A4C00(gppStringList[0x240c / 4], 0, 1, 0);
        return;
    }
    if (IsPartySlotEligible00524A10(g_rcs_mode_0064cbe8) == 0) {
        Function5A4C00(gppStringList[0x245c / 4], 0, 1, 0);
        return;
    }
    character = g_status_685170.buffers.characters + g_rcs_mode_0064cbe8;
    BindEquippedItem(character, 6);
    BindEquippedItem(character, 7);
    if (CanUnequipSlotItem(character, 6) != 0 && CanUnequipSlotItem(character, 7) != 0) {
        Function51D3B0(g_rcs_mode_0064cbe8, 0, 1);
        g_camp_screen_0069c0f4->item_redraw_flags |= 0x3ffe00;
        g_camp_screen_0069c0f4->redraw_flags |= 0x100;
        g_camp_screen_0069c0f4->redraw_flags |= 0x2000;
        return;
    }
    Function5A4C00(gppStringList[0x2458 / 4], 0, 1, 0);
}

// FUNCTION: WIZ8 0x005BB140
void TogglePartyRowFlag005BB140(void)
{
    if ((g_panel_controls_69c468[1]->m_stateFlags & g_W8TextControlMask005ED570) != 0) {
        g_status_685170.buffers.party_rows[g_rcs_mode_0064cbe8].flag_0f5 = 1;
        g_panel_controls_69c468[0]->SetEnabled(0);
        g_panel_controls_69c468[0]->Invalidate(0);
        return;
    }
    g_status_685170.buffers.party_rows[g_rcs_mode_0064cbe8].flag_0f5 = 0;
    g_panel_controls_69c468[0]->SetEnabled(1);
    g_panel_controls_69c468[0]->Invalidate(0);
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
    g_camp_screen_0069c0f4->item_scroll = 0;
    Function5A4A00();
    g_camp_screen_0069c0f4->item_redraw_flags |= 0x7fc00000;
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
    if ((g_panel_controls_69c470[tab]->m_stateFlags & g_W8TextControlMask005ED570) == 0) {
        g_camp_screen_0069c0f4->realm_flags[realm] = 0;
    } else {
        g_camp_screen_0069c0f4->realm_flags[realm] = 1;
        if (realm != 0 && realm != 1) {
            Function5A49D0(realm);
            index = 0;
            do {
                if (index != tab && (g_panel_controls_69c470[index]->m_stateFlags &
                                     g_W8TextControlMask005ED570) != 0) {
                    g_panel_controls_69c470[index]->DisableSecondaryState(1);
                }
                ++index;
            } while (index < 4);
        }
    }
    g_camp_screen_0069c0f4->item_scroll = 0;
    Function5A4A00();
    if (gfKeyState[0x11] == 0) {
        g_camp_screen_0069c0f4->item_redraw_flags |= 0x7fc00000;
        return;
    }
    g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
}

/* Region reason values are bit codes set by the region manager: 0x8 left
   release, 0x10 left down, 0x40 right press, 0x80 right release, 0x100
   double-click/activate, 0x400 enter/leave transition, 0x800 mouse wheel. */
// FUNCTION: WIZ8 0x005BB350
unsigned char BackpackRegionHandler005BB350(const W8RegionEvent* event, W8Region* region)
{
    int slot;
    W8ItemInstance* item;

    slot = region->callback_id;
    item = g_value_0069c0f8->backpack + slot;
    if (item->item_id == -1 && g_status_685170.item_in_cursor == 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }
    if (event->reason < 0x81) {
        if (event->reason == 0x80) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (event->reason == 8) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        } else {
            if (event->reason != 0x10) {
                return 0;
            }
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
                return 1;
            }
            Function5A4C70(item, slot, 0);
            if (item->item_id != -1) {
                SetItemTooltip005BBD30(item, region);
                Function4F27C0(1);
                return 1;
            }
        }
        Function4F27E0(region);
        return 1;
    }
    if (event->reason == 0x100) {
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && item->item_id != -1 &&
            g_camp_screen_0069c0f4->entry_mode != 3) {
            g_camp_entry_parameter_0069c0fc = (int)g_value_0069c0f8;
            if (CanItemLeaveItsSlot(item) != 0 && PartyAttemptsToIdentifyItem(item, 0) != 0 &&
                g_camp_screen_0069c0f4->realm_flags[1] != 0) {
                Function5A4A00();
                g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
            }
            OpenItemInfoDialog005BA110(item, 0);
            Function5B59B0(0);
        }
        return 1;
    }
    if (event->reason == 0x400) {
        if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
            if (item->item_id != -1 || g_status_685170.item_in_cursor != 0) {
                g_camp_screen_0069c0f4->item_redraw_flags |= 2 << (slot & 0x1f);
            }
            UpdateItemCursorForState005BAD20((region->flags & W8_REGION_MOUSE_ENTER) != 0, item, 0);
            if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
                if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
                    return 0;
                }
            } else if (item->item_id != -1) {
                SetItemTooltip005BBD30(item, region);
                Function4F27C0(1);
                return 0;
            }
            Function4F27E0(region);
        }
        return 0;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005BB560
unsigned char EquipSlotRegionHandler005BB560(const W8RegionEvent* event, W8Region* region)
{
    int slot;
    int delay;
    W8ItemInstance* item;
    wchar_t* name;

    slot = region->callback_id;
    item = g_value_0069c0f8->equipment + slot;
    if (item->item_id == -1 &&
        (g_status_685170.item_in_cursor == 0 || g_camp_screen_0069c0f4->entry_mode == 1 ||
         CanEquipItemInSlot(g_value_0069c0f8, g_status_685170.item_in_hand_235b.item_id,
                            (unsigned char)slot, 1) == 0 ||
         CanCharacterUseItem(g_value_0069c0f8, g_status_685170.item_in_hand_235b.item_id) == 0)) {
        PushButtonSoundScheme005587C0(0, 1);
    }
    if (event->reason < 0x81) {
        if (event->reason == 0x80) {
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if (event->reason == 8) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            Function4F27E0(region);
            return 1;
        }
        if (event->reason != 0x10) {
            return 0;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
            Function5A4C70(item, slot, 1);
            delay = g_settings_6850c8.tooltip_delay_ms;
            if ((unsigned int)delay > 300) {
                delay = 300;
            }
            SetRegionHelpDelay(delay);
            Function4F27C0(1);
            Function4F27D0(region);
            if (item->item_id != -1) {
                name = FormatItemDisplayName(item, 0);
                swprintf(g_camp_screen_0069c0f4->caption, L"%s (%s)", name,
                         gppStringList[g_equip_slot_label_ids_61e7c4[slot]]);
                SetRegionHelpText(g_camp_screen_0069c0f4->caption);
                return 1;
            }
            swprintf(g_camp_screen_0069c0f4->caption, g_format_s_006068e4,
                     gppStringList[g_equip_slot_label_ids_61e7c4[slot]]);
            SetRegionHelpText(g_camp_screen_0069c0f4->caption);
            return 1;
        }
    } else {
        if (event->reason != 0x100) {
            if (event->reason != 0x400) {
                return 0;
            }
            if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
                if (item->item_id != -1 || g_status_685170.item_in_cursor != 0) {
                    g_camp_screen_0069c0f4->item_redraw_flags |= 0x400 << (slot & 0x1f);
                }
                UpdateItemCursorForState005BAD20(1, item, 0);
                Function4F27D0(region);
                delay = g_settings_6850c8.tooltip_delay_ms;
                if ((unsigned int)delay > 300) {
                    delay = 300;
                }
                SetRegionHelpDelay(delay);
                Function4F27C0(1);
                if (item->item_id == -1) {
                    swprintf(g_camp_screen_0069c0f4->caption, g_format_s_006068e4,
                             gppStringList[g_equip_slot_label_ids_61e7c4[slot]]);
                } else {
                    name = FormatItemDisplayName(item, 0);
                    swprintf(g_camp_screen_0069c0f4->caption, L"%s (%s)", name,
                             gppStringList[g_equip_slot_label_ids_61e7c4[slot]]);
                }
                SetRegionHelpText(g_camp_screen_0069c0f4->caption);
            }
            if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
                if (item->item_id != -1 || g_status_685170.item_in_cursor != 0) {
                    g_camp_screen_0069c0f4->item_redraw_flags |= 0x400 << (slot & 0x1f);
                }
                UpdateItemCursorForState005BAD20(0, item, 0);
                Function4F27E0(region);
            }
            return 0;
        }
        if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 && item->item_id != -1 &&
            g_camp_screen_0069c0f4->entry_mode != 3) {
            g_camp_entry_parameter_0069c0fc = (int)g_value_0069c0f8;
            if (CanItemLeaveItsSlot(item) != 0 && PartyAttemptsToIdentifyItem(item, 0) != 0 &&
                g_camp_screen_0069c0f4->realm_flags[1] != 0) {
                Function5A4A00();
                g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
            }
            OpenItemInfoDialog005BA110(item, 0);
            Function5B59B0(0);
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x005BB900
unsigned char ItemPoolRegionHandler005BB900(const W8RegionEvent* event, W8Region* region)
{
    unsigned int slot;
    unsigned int pool_index;
    int delta;
    int count;
    W8ItemInstance* item;

    slot = region->callback_id;
    pool_index = g_camp_screen_0069c0f4->item_scroll + slot;
    if ((unsigned int)g_camp_screen_0069c0f4->item_list_count <= pool_index ||
        g_status_685170.party_item_pool_0021[g_camp_screen_0069c0f4->item_list_4ec[pool_index]]
                .item_id == -1) {
        pool_index = g_status_685170.party_item_count_1791;
    } else {
        pool_index = g_camp_screen_0069c0f4->item_list_4ec[pool_index];
    }
    item = g_status_685170.party_item_pool_0021 + pool_index;
    if (item->item_id == -1 && g_status_685170.item_in_cursor == 0) {
        PushButtonSoundScheme005587C0(0, 1);
    }
    if (event->reason < 0x101) {
        if (event->reason == 0x100) {
            if ((region->flags & W8_REGION_RIGHT_BUTTON_HELD) != 0 &&
                pool_index < (unsigned int)g_status_685170.party_item_count_1791 &&
                g_camp_screen_0069c0f4->entry_mode != 3) {
                g_camp_entry_parameter_0069c0fc = (int)g_value_0069c0f8;
                if (CanItemLeaveItsSlot(item) != 0 && PartyAttemptsToIdentifyItem(item, 0) != 0 &&
                    g_camp_screen_0069c0f4->realm_flags[1] != 0) {
                    Function5A4A00();
                    g_camp_screen_0069c0f4->redraw_flags |= 0xfffffff;
                }
                OpenItemInfoDialog005BA110(item, 0);
                Function5B59B0(0);
            }
            return 1;
        }
        if (event->reason == 8) {
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            Function4F27E0(region);
            return 1;
        }
        if (event->reason != 0x10) {
            if (event->reason != 0x80) {
                return 0;
            }
            region->flags |= W8_REGION_RIGHT_BUTTON_HELD;
            return 1;
        }
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) == 0) {
            return 1;
        }
        Function5A4C70(item, pool_index, 2);
        if (item->item_id != -1) {
            SetItemTooltip005BBD30(item, region);
            return 1;
        }
    } else {
        if (event->reason == 0x400) {
            if ((region->flags & W8_REGION_MOUSE_TRANSITION_MASK) != 0) {
                if (item->item_id != -1 || g_status_685170.item_in_cursor != 0) {
                    g_camp_screen_0069c0f4->item_redraw_flags |= 0x800000 << (slot & 0x1f);
                }
                UpdateItemCursorForState005BAD20((region->flags & W8_REGION_MOUSE_ENTER) != 0, item,
                                                 0);
                if ((region->flags & W8_REGION_MOUSE_ENTER) == 0) {
                    if ((region->flags & W8_REGION_MOUSE_LEAVE) == 0) {
                        return 0;
                    }
                } else if (item->item_id != -1) {
                    SetItemTooltip005BBD30(item, region);
                    return 0;
                }
                Function4F27E0(region);
            }
            return 0;
        }
        if (event->reason != 0x800) {
            return 0;
        }
        delta = GetMouseWheelDeltaValue(
            reinterpret_cast<const W8RegionMouseEvent*>(event)
                ->mouse_position); // reinterpret-ok: reason 0x800 carries the mouse event payload
        count = delta;
        if (delta > 0) {
            do {
                g_camp_screen_0069c0f4->item_range->m_range->Decrement();
                --count;
            } while (count != 0);
            delta = 0;
        }
        if (delta < 0) {
            count = -delta;
            do {
                g_camp_screen_0069c0f4->item_range->m_range->Increment();
                --count;
            } while (count != 0);
        }
        pool_index = g_camp_screen_0069c0f4->item_scroll + slot;
        if ((unsigned int)g_camp_screen_0069c0f4->item_list_count <= pool_index) {
            pool_index = g_status_685170.party_item_count_1791;
        } else {
            pool_index = g_camp_screen_0069c0f4->item_list_4ec[pool_index];
        }
        if (g_status_685170.party_item_pool_0021[pool_index].item_id != -1) {
            SetItemTooltip005BBD30(g_status_685170.party_item_pool_0021 + pool_index, region);
            return 1;
        }
    }
    Function4F27E0(region);
    return 1;
}

/* Reason 0x40/0x8 press the tab control, 0x10 releases it, and the 0x400
   enter/leave transition forwards the matching mouse events. */
// FUNCTION: WIZ8 0x005BBBB0
unsigned char RealmTabRegionHandler005BBBB0(const W8RegionEvent* event, W8Region* region)
{
    if (event->reason < 0x41) {
        if (event->reason == 0x40 || event->reason == 8) {
            g_panel_controls_69c470[region->callback_id]->OnLeftButtonDown(0);
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (event->reason == 0x10) {
            g_panel_controls_69c470[region->callback_id]->OnLeftButtonUp(0);
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            }
            return 1;
        }
    } else if (event->reason == 0x400) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_panel_controls_69c470[region->callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_panel_controls_69c470[region->callback_id]->OnMouseEnter(0);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005BBC70
unsigned char PanelTabRegionHandler005BBC70(const W8RegionEvent* event, W8Region* region)
{
    if (event->reason < 0x41) {
        if (event->reason == 0x40 || event->reason == 8) {
            g_panel_controls_69c468[region->callback_id]->OnLeftButtonDown(0);
            region->flags |= W8_REGION_LEFT_BUTTON_HELD;
            return 1;
        }
        if (event->reason == 0x10) {
            g_panel_controls_69c468[region->callback_id]->OnLeftButtonUp(0);
            if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) != 0) {
                region->flags &= ~W8_REGION_LEFT_BUTTON_HELD;
            }
            return 1;
        }
    } else if (event->reason == 0x400) {
        if ((region->flags & W8_REGION_MOUSE_LEAVE) != 0) {
            g_panel_controls_69c468[region->callback_id]->OnMouseLeave(0);
            return 1;
        }
        if ((region->flags & W8_REGION_MOUSE_ENTER) != 0) {
            g_panel_controls_69c468[region->callback_id]->OnMouseEnter(0);
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005BBD30
void SetItemTooltip005BBD30(W8ItemInstance* item, W8Region* region)
{
    int delay;
    wchar_t* name;
    const W8ItemDatabaseRecord* record;

    delay = g_settings_6850c8.tooltip_delay_ms;
    if ((unsigned int)delay > 300) {
        delay = 300;
    }
    SetRegionHelpDelay(delay);
    Function4F27C0(1);
    Function4F27D0(region);
    name = FormatItemDisplayName(item, 0);
    wcscpy(g_camp_screen_0069c0f4->caption, name);
    record = g_item_records + item->item_id;
    if (record->category == 3) {
        if (record->spell_id == 0) {
            srAssertFail("ubSpell != SPELL_NONE",
                         "C:\\Projects\\Wizardry 8\\Local Screens\\RCSItemsPage.cpp", 0xa7a, 0);
        }
        if (g_value_0069c0f8->spell_learned[record->spell_id] == 1 &&
            (item->identified != 0 || item->unknown_07[0] != 0)) {
            wcscat(g_camp_screen_0069c0f4->caption, L" (");
            wcscat(g_camp_screen_0069c0f4->caption, gppStringList[0x24c0 / 4]);
            wcscat(g_camp_screen_0069c0f4->caption, L")");
        }
    }
    SetRegionHelpText(g_camp_screen_0069c0f4->caption);
}
